#include <includes.h>
#include <stdio.h>

#include "hostapd.h"
#include "sta_info.h"
#include "mesh.h"
#include "eloop.h"
#include "ieee802_11.h"
#include "driver.h"

/* Peer link cancel reasons, all subject to ANA approval */
#define MESH_LINK_CANCELLED			2
#define MESH_MAX_NEIGHBORS			3
#define MESH_CAPABILITY_POLICY_VIOLATION	4
#define MESH_CLOSE_RCVD				5
#define MESH_MAX_RETRIES			6
#define MESH_CONFIRM_TIMEOUT			7
#define MESH_SECURITY_ROLE_NEGOTIATION_DIFFERS	8
#define MESH_SECURITY_AUTHENTICATION_IMPOSSIBLE	9
#define MESH_SECURITY_FAILED_VERIFICATION	10

enum plink_frame_type {
	PLINK_OPEN = 0,
	PLINK_CONFIRM,
	PLINK_CLOSE
};

enum plink_event {
	PLINK_UNDEFINED,
	OPN_ACPT,
	OPN_RJCT,
	OPN_IGNR,
	CNF_ACPT,
	CNF_RJCT,
	CNF_IGNR,
	CLS_ACPT,
	CLS_IGNR
};

/**
 * mesh_plink_fsm_restart - restart a mesh peer link finite state machine
 *
 * @sta: mes peer link to restart
 *
 * Locking: this function must be called holding sta->plink_lock
 */
static inline void mesh_plink_fsm_restart(struct sta_info *sta)
{
	sta->plink_state = PLINK_LISTEN;
	sta->llid = sta->plid = sta->reason = 0;
	sta->plink_retries = 0;
}

/*
 * NOTE: This is just an alias for ap_sta_add
 */
static struct sta_info *mesh_plink_alloc(struct hostapd_data *hapd,
					 u8 *hw_addr, u64 rates)
{
	struct sta_info *sta;

	if (hapd->num_sta >= MESH_MAX_PLINKS)
		return NULL;

	sta = ap_sta_add(hapd, hw_addr);
	if (!sta)
		return NULL;

	sta->flags |= WLAN_STA_AUTHORIZED;
	sta->plink_state = PLINK_LISTEN;

	return sta;
}

static int mesh_plink_frame_tx(struct hostapd_data * hapd,
		enum plink_frame_type action, u8 *da, __le16 llid, __le16 plid,
		__le16 reason) 
{
//	struct ieee80211_local *local = wdev_priv(dev->ieee80211_ptr);
//	struct sk_buff *skb = dev_alloc_skb(local->hw.extra_tx_headroom + 400);
#define MAX_PLINK_LEN 400
	struct ieee80211_mgmt *mgmt = wpa_zalloc(MAX_PLINK_LEN);
	u8 include_plid = 0;
	u8 *pos;
	int ie_len;

	if (!mgmt)
		return -1;
	/* 25 is the size of the common mgmt part (24) plus the size of the
	 * common action part (1)
	 */
	memset(mgmt, 0, 25 + sizeof(mgmt->u.action.u.plink_action));
	mgmt->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
					   WLAN_FC_STYPE_ACTION);
	memcpy(mgmt->da, da, ETH_ALEN);
	memcpy(mgmt->sa, hapd->own_addr, ETH_ALEN);
	/* BSSID is left zeroed, wildcard value */
	mgmt->u.action.category = PLINK_CATEGORY;
	mgmt->u.action.u.plink_action.action_code = action;

	pos = ((u8*)mgmt) + sizeof(struct ieee80211_mgmt);
	if (action == PLINK_CLOSE)
		mgmt->u.action.u.plink_action.aux = reason;
	else {
		mgmt->u.action.u.plink_action.aux = host_to_le16(0x0);
		if (action == PLINK_CONFIRM) {
			//pos = skb_put(skb, 4);
			/* two-byte status code followed by two-byte AID */
			memset(pos, 0, 4);
			pos += 4;
		}
		mesh_mgmt_ies_add(pos, hapd);
	}

	/* Add Peer Link Management element */
	switch (action) {
	case PLINK_OPEN:
		ie_len = 3;
		break;
	case PLINK_CONFIRM:
		ie_len = 5;
		include_plid = 1;
		break;
	case PLINK_CLOSE:
	default:
		if (!plid)
			ie_len = 5;
		else {
			ie_len = 7;
			include_plid = 1;
		}
		break;
	}

	*pos++ = WLAN_EID_PEER_LINK;
	*pos++ = ie_len;
	*pos++ = action;
	memcpy(pos, &llid, 2);
	if (include_plid) {
		pos += 2;
		memcpy(pos, &plid, 2);
	}
	if (action == PLINK_CLOSE) {
		pos += 2;
		memcpy(pos, &reason, 2);
	}

	if (hostapd_send_mgmt_frame(hapd, mgmt, pos - (u8 *) mgmt, 0) < 0)
		perror("mesh_plink_frame_tx: send");

	free(mgmt);

	return 0;
}
void mesh_neighbour_update(struct hostapd_data *hapd, u8 *hw_addr, u64 rates)
{
	struct sta_info *sta;
	sta = ap_get_sta(hapd, hw_addr);
	if(sta == NULL) {
		printf("%s: add a new sta: " MACSTR "\n", __func__, MAC2STR(hw_addr));
		sta = mesh_plink_alloc(hapd, hw_addr, rates);
	}

	if(sta->plink_state == PLINK_LISTEN) {
		mesh_plink_open(hapd, sta);
	}
}

static void mesh_plink_timer(void *eloop_ctx, void * timeout_ctx)
{
	struct sta_info *sta;
	struct hostapd_data *hapd;
	__le16 llid, plid, reason;
	//struct net_device *dev = NULL;
	//struct ieee80211_sub_if_data *sdata;

	/*
	 * This STA is valid because sta_info_destroy() will
	 * del_timer_sync() this timer after having made sure
	 * it cannot be readded (by deleting the plink.)
	 */
	hapd = (struct hostapd_data *) eloop_ctx;
	sta = (struct sta_info *) timeout_ctx;

	reason = 0;
	llid = sta->llid;
	plid = sta->plid;

	switch (sta->plink_state) {
	case PLINK_OPN_RCVD:
	case PLINK_OPN_SNT:
		/* retry timer */
		if (sta->plink_retries < hapd->mconf->dot11MeshMaxRetries) {
			u32 rand_int;
			rand_int = rand();
			sta->plink_timeout = sta->plink_timeout +
					     rand_int % sta->plink_timeout;
			++sta->plink_retries;
			eloop_register_timeout(sta->plink_timeout, 0, mesh_plink_timer, hapd, sta);
			mesh_plink_frame_tx(hapd, PLINK_OPEN, sta->addr, llid,
					    0, 0);
			break;
		}
		reason = MESH_MAX_RETRIES;
		/* fall through on else */
	case PLINK_CNF_RCVD:
		/* confirm timer */
		if (!reason)
			reason = MESH_CONFIRM_TIMEOUT;
		sta->plink_state = PLINK_HOLDING;
		eloop_register_timeout(hapd->mconf->dot11MeshHoldingTimeout, 0, mesh_plink_timer, hapd, sta);
		mesh_plink_frame_tx(hapd, PLINK_CLOSE, sta->addr, llid, plid,
				    reason);
		break;
	case PLINK_HOLDING:
		/* holding timer */
		eloop_cancel_timeout(mesh_plink_timer, hapd, sta);
		mesh_plink_fsm_restart(sta);
		break;
	default:
		break;
	}
}

static inline void mesh_plink_timer_set(struct hostapd_data *hapd, struct sta_info *sta, int timeout)
{
	eloop_register_timeout(timeout, 0, mesh_plink_timer, hapd, sta);
	sta->plink_timeout = timeout;
}

int mesh_plink_open(struct hostapd_data *hapd, struct sta_info *sta)
{
	__le16 llid;

	llid = rand();
	sta->llid = llid;
	if (sta->plink_state != PLINK_LISTEN) {
		return -EBUSY;
	}
	sta->plink_state = PLINK_OPN_SNT;
	mesh_plink_timer_set(hapd, sta, hapd->mconf->dot11MeshRetryTimeout);

	return mesh_plink_frame_tx(hapd, PLINK_OPEN,
				   sta->addr, llid, 0, 0);
}

