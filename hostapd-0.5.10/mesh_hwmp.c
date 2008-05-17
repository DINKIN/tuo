#include <includes.h>
#include <stdio.h>

#include "hostapd.h"
#include "sta_info.h"
#include "mesh.h"
#include "list.h"
#include "common.h"
#include "ieee802_11.h"
#include "driver.h"
#include "wireless_copy.h"
#include "eloop.h"

#define TEST_FRAME_LEN	8192
#define MAX_METRIC	0xffffffff
#define ARITH_SHIFT	8

/* Number of frames buffered per destination for unresolved destinations */
#define MESH_FRAME_QUEUE_LEN	10
#define MAX_PREQ_QUEUE_LEN	64

/* Destination only */
#define MP_F_DO	0x1
/* Reply and forward */
#define MP_F_RF	0x2

static inline u32 u32_field_get(u8 *preq_elem, int offset, u8 ae)
{
	if (ae)
		offset += 6;
	return le_to_host32(*(__le32 *) (preq_elem + offset));
}

/* HWMP IE processing macros */
#define AE_F			(1<<6)
#define AE_F_SET(x)		(*x & AE_F)
#define PREQ_IE_FLAGS(x)	(*(x))
#define PREQ_IE_HOPCOUNT(x)	(*(x + 1))
#define PREQ_IE_TTL(x)		(*(x + 2))
#define PREQ_IE_PREQ_ID(x)	u32_field_get(x, 3, 0)
#define PREQ_IE_ORIG_ADDR(x)	(x + 7)
#define PREQ_IE_ORIG_DSN(x)	u32_field_get(x, 13, 0);
#define PREQ_IE_LIFETIME(x)	u32_field_get(x, 17, AE_F_SET(x));
#define PREQ_IE_METRIC(x) 	u32_field_get(x, 21, AE_F_SET(x));
#define PREQ_IE_DST_F(x)	(*(AE_F_SET(x) ? x + 32 : x + 26))
#define PREQ_IE_DST_ADDR(x) 	(AE_F_SET(x) ? x + 33 : x + 27)
#define PREQ_IE_DST_DSN(x) 	u32_field_get(x, 33, AE_F_SET(x));


#define PREP_IE_FLAGS(x)	PREQ_IE_FLAGS(x)
#define PREP_IE_HOPCOUNT(x)	PREQ_IE_HOPCOUNT(x)
#define PREP_IE_TTL(x)		PREQ_IE_TTL(x)
#define PREP_IE_ORIG_ADDR(x)	(x + 3)
#define PREP_IE_ORIG_DSN(x)	u32_field_get(x, 9, 0);
#define PREP_IE_LIFETIME(x)	u32_field_get(x, 13, AE_F_SET(x));
#define PREP_IE_METRIC(x)	u32_field_get(x, 17, AE_F_SET(x));
#define PREP_IE_DST_ADDR(x)	(AE_F_SET(x) ? x + 27 : x + 21)
#define PREP_IE_DST_DSN(x)	u32_field_get(x, 27, AE_F_SET(x));

#define PERR_IE_DST_ADDR(x)	(x + 2)
#define PERR_IE_DST_DSN(x)	u32_field_get(x, 8, 0);

#define TU_TO_EXP_TIME(x) (jiffies + msecs_to_jiffies(x * 1024 / 1000))
#define MSEC_TO_TU(x) (x*1000/1024)
#define DSN_GT(x, y) ((long) (y) - (long) (x) < 0)
#define DSN_LT(x, y) ((long) (x) - (long) (y) < 0)

enum mpath_frame_type {
	MPATH_PREQ = 0,
	MPATH_PREP,
	MPATH_PERR
};
static int mesh_path_sel_frame_tx(enum mpath_frame_type action, u8 flags,
		u8 *orig_addr, __le32 orig_dsn, u8 dst_flags, u8 *dst,
		__le32 dst_dsn, u8 *da, u8 hop_count, u8 ttl, __le32 lifetime,
		__le32 metric, __le32 preq_id, struct hostapd_data *hapd)
{
#define MAX_PATH_SEL_LEN 400
	struct ieee80211_mgmt *mgmt = wpa_zalloc(MAX_PATH_SEL_LEN);
	u8 *pos;
	int ie_len;

	if (!mgmt)
		return -1;
	/* 25 is the size of the common mgmt part (24) plus the size of the
	 * common action part (1) category
	 */
	memset(mgmt, 0, IEEE80211_HDRLEN + 1 + sizeof(mgmt->u.action.u.mesh_action));
	mgmt->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
					   WLAN_FC_STYPE_ACTION);

	memcpy(mgmt->da, da, ETH_ALEN);
	memcpy(mgmt->sa, hapd->own_addr, ETH_ALEN);
	/* BSSID is left zeroed, wildcard value 
	 * BSSID set to broadcast, because the madwifi driver only receive the broadcast bssid frame or matched bssid frame.
	 */
	memset(mgmt->bssid, 0xff, ETH_ALEN);

	mgmt->u.action.category = MESH_PATH_SEL_CATEGORY;
	mgmt->u.action.u.mesh_action.action_code = action;

	pos = (u8*)mgmt + 25 + sizeof(mgmt->u.action.u.mesh_action);
	switch (action) {
	case MPATH_PREQ:
		ie_len = 37;
		*pos++ = WLAN_EID_PREQ;
		break;
	case MPATH_PREP:
		ie_len = 31;
		*pos++ = WLAN_EID_PREP;
		break;
	default:
		free(mgmt);
		return -1;
		break;
	}
	*pos++ = ie_len;
	*pos++ = flags;
	*pos++ = hop_count;
	*pos++ = ttl;
	if (action == MPATH_PREQ) {
		memcpy(pos, &preq_id, 4);
		pos += 4;
	}
	memcpy(pos, orig_addr, ETH_ALEN);
	pos += ETH_ALEN;
	memcpy(pos, &orig_dsn, 4);
	pos += 4;
	memcpy(pos, &lifetime, 4);
	pos += 4;
	memcpy(pos, &metric, 4);
	pos += 4;
	if (action == MPATH_PREQ) {
		/* destination count */
		*pos++ = 1;
		*pos++ = dst_flags;
	}
	memcpy(pos, dst, ETH_ALEN);
	pos += ETH_ALEN;
	memcpy(pos, &dst_dsn, 4);
	pos += 4;

	if (hostapd_send_mgmt_frame(hapd, mgmt, pos - (u8 *) mgmt, 0) < 0)
		perror("mesh_plink_frame_tx: send");

	free(mgmt);
	return 0;
}

/**
 * mesh_send_path error - Sends a PERR mesh management frame
 *
 * @dst: broken destination
 * @dst_dsn: dsn of the broken destination
 * @ra: node this frame is addressed to
 */
int mesh_path_error_tx(u8 *dst, __le32 dst_dsn, u8 *ra,
		struct hostapd_data *hapd)
{
#define MAX_PATH_SEL_LEN 400
	struct ieee80211_mgmt *mgmt = wpa_zalloc(MAX_PATH_SEL_LEN);
	u8 *pos;
	int ie_len;

	if (!mgmt)
		return -1;
	/* 25 is the size of the common mgmt part (24) plus the size of the
	 * common action part (1)
	 */
	memset(mgmt, 0, 25 + sizeof(mgmt->u.action.u.mesh_action));
	mgmt->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
					   WLAN_FC_STYPE_ACTION);

	memcpy(mgmt->da, ra, ETH_ALEN);
	memcpy(mgmt->sa, hapd->own_addr, ETH_ALEN);
	/* BSSID is left zeroed, wildcard value */
	mgmt->u.action.category = MESH_PATH_SEL_CATEGORY;
	mgmt->u.action.u.mesh_action.action_code = MPATH_PERR;
	ie_len = 12;

	pos = (u8*)mgmt + 25 + sizeof(mgmt->u.action.u.mesh_action);
	*pos++ = WLAN_EID_PERR;
	*pos++ = ie_len;
	/* mode flags, reserved */
	*pos++ = 0;
	/* number of destinations */
	*pos++ = 1;
	memcpy(pos, dst, ETH_ALEN);
	pos += ETH_ALEN;
	memcpy(pos, &dst_dsn, 4);
	pos += 4;

	if (hostapd_send_mgmt_frame(hapd, mgmt, pos - (u8 *) mgmt, 0) < 0)
		perror("mesh_plink_frame_tx: send");

	free(mgmt);
	return 0;
}


static u32 airtime_link_metric_get(struct hostapd_data *hapd,
				   struct sta_info *sta)
{
	struct ieee80211_supported_band *sband;
	/* This should be adjusted for each device */
	int device_constant = 1 << ARITH_SHIFT;
	int test_frame_len = TEST_FRAME_LEN << ARITH_SHIFT;
	int s_unit = 1 << ARITH_SHIFT;
	int rate, err;
	u32 tx_time, estimated_retx;
	u64 result;

//	sband = local->hw.wiphy->bands[local->hw.conf.channel->band];

	if (sta->fail_avg >= 100)
		return MAX_METRIC;
	err = (sta->fail_avg << ARITH_SHIFT) / 100;

	/* bitrate is in units of 100 Kbps, while we need rate in units of
	 * 1Mbps. This will be corrected on tx_time computation.
	 */
	rate = sband->bitrates[sta->txrate_idx].bitrate;
	tx_time = (device_constant + 10 * test_frame_len / rate);
	estimated_retx = ((1 << (2 * ARITH_SHIFT)) / (s_unit - err));
	result = (tx_time * estimated_retx) >> (2 * ARITH_SHIFT) ;
	return (u32)result;
}

/**
 * hwmp_route_info_get - Update routing info to originator and transmitter
 *
 * @dev: local mesh interface
 * @mgmt: mesh management frame
 * @hwmp_ie: hwmp information element (PREP or PREQ)
 *
 * This function updates the path routing information to the originator and the
 * transmitter of a HWMP PREQ or PREP fram.
 *
 * Returns: metric to frame originator or 0 if the frame should not be further
 * processed
 *
 * Notes: this function is the only place (besides user-provided info) where
 * path routing information is updated.
 */
static u32 hwmp_route_info_get(struct hostapd_data *hapd,
			    struct ieee80211_mgmt *mgmt,
			    u8 *hwmp_ie)
{
	struct mesh_path *mpath;
	struct sta_info *sta;
	bool fresh_info;
	u8 *orig_addr, *ta;
	u32 orig_dsn, orig_metric;
	unsigned long orig_lifetime, exp_time;
	u32 last_hop_metric, new_metric;
	bool process = true;
	u8 action = mgmt->u.action.u.mesh_action.action_code;

	sta = ap_get_sta(hapd, mgmt->sa);
	if (!sta) {
		return 0;
	}

	last_hop_metric = airtime_link_metric_get(hapd, sta);
	/* Update and check originator routing info */
	fresh_info = true;

	switch (action) {
	case MPATH_PREQ:
		orig_addr = PREQ_IE_ORIG_ADDR(hwmp_ie);
		orig_dsn = PREQ_IE_ORIG_DSN(hwmp_ie);
		orig_lifetime = PREQ_IE_LIFETIME(hwmp_ie);
		orig_metric = PREQ_IE_METRIC(hwmp_ie);
		break;
	case MPATH_PREP:
		/* Originator here refers to the MP that was the destination in
		 * the Path Request. The draft refers to that MP as the
		 * destination address, even though usually it is the origin of
		 * the PREP frame. We divert from the nomenclature in the draft
		 * so that we can easily use a single function to gather path
		 * information from both PREQ and PREP frames.
		 */
		orig_addr = PREP_IE_ORIG_ADDR(hwmp_ie);
		orig_dsn = PREP_IE_ORIG_DSN(hwmp_ie);
		orig_lifetime = PREP_IE_LIFETIME(hwmp_ie);
		orig_metric = PREP_IE_METRIC(hwmp_ie);
		break;
	default:
		return 0;
	}
	new_metric = orig_metric + last_hop_metric;
	if (new_metric < orig_metric)
		new_metric = MAX_METRIC;
//	exp_time = TU_TO_EXP_TIME(orig_lifetime);

	if (memcmp(orig_addr, hapd->own_addr, ETH_ALEN) == 0) {
		/* This MP is the originator, we are not interested in this
		 * frame, except for updating transmitter's path info.
		 */
		process = false;
		fresh_info = false;
	} else {
		mpath = mesh_path_lookup(orig_addr);
		if (mpath) {
			if (mpath->flags & MESH_PATH_FIXED)
				fresh_info = false;
			else if ((mpath->flags & MESH_PATH_ACTIVE) &&
			    (mpath->flags & MESH_PATH_DSN_VALID)) {
				if (DSN_GT(mpath->dsn, orig_dsn) ||
				    (mpath->dsn == orig_dsn &&
				     action == MPATH_PREQ &&
				     new_metric > mpath->metric)) {
					process = false;
					fresh_info = false;
				}
			}
		} else {
			mesh_path_add(orig_addr, hapd);
			mpath = mesh_path_lookup(orig_addr);
			if (!mpath) {
				return 0;
			}
		}

		if (fresh_info) {
			mesh_path_assign_nexthop(mpath, sta);
			mpath->flags |= MESH_PATH_DSN_VALID;
			mpath->metric = new_metric;
			mpath->dsn = orig_dsn;
//			mpath->exp_time = time_after(mpath->exp_time, exp_time)
//					  ?  mpath->exp_time : exp_time;
			mesh_path_activate(mpath);
			mesh_path_tx_pending(mpath);
			/* draft says preq_id should be saved to, but there does
			 * not seem to be any use for it, skipping by now
			 */
		} else {
		}
	}

	/* Update and check transmitter routing info */
	ta = mgmt->sa;
	if (memcmp(orig_addr, ta, ETH_ALEN) == 0)
		fresh_info = false;
	else {
		fresh_info = true;

		mpath = mesh_path_lookup(ta);
		if (mpath) {
			if ((mpath->flags & MESH_PATH_FIXED) ||
				((mpath->flags & MESH_PATH_ACTIVE) &&
					(last_hop_metric > mpath->metric)))
				fresh_info = false;
		} else {
			mesh_path_add(ta, hapd);
			mpath = mesh_path_lookup(ta);
			if (!mpath) {
				return 0;
			}
		}

		if (fresh_info) {
			mesh_path_assign_nexthop(mpath, sta);
			mpath->flags &= ~MESH_PATH_DSN_VALID;
			mpath->metric = last_hop_metric;
//			mpath->exp_time = time_after(mpath->exp_time, exp_time)
//					  ?  mpath->exp_time : exp_time;
			mesh_path_activate(mpath);
			mesh_path_tx_pending(mpath);
		} else {
		}
	}


	return process ? new_metric : 0;
}


static void hwmp_preq_frame_process(struct hostapd_data *hapd,
				    struct ieee80211_mgmt *mgmt,
				    u8 *preq_elem, u32 metric) {
	struct mesh_path *mpath;
	u8 *dst_addr, *orig_addr;
	u8 dst_flags, ttl;
	u32 orig_dsn, dst_dsn, lifetime;
	bool reply = false;
	bool forward = true;

	/* Update destination DSN, if present */
	dst_addr = PREQ_IE_DST_ADDR(preq_elem);
	orig_addr = PREQ_IE_ORIG_ADDR(preq_elem);
	dst_dsn = PREQ_IE_DST_DSN(preq_elem);
	orig_dsn = PREQ_IE_ORIG_DSN(preq_elem);
	dst_flags = PREQ_IE_DST_F(preq_elem);

	if (memcmp(dst_addr, hapd->own_addr, ETH_ALEN) == 0) {
		forward = false;
		reply = true;
		metric = 0;
/*
		if (time_after(jiffies, ifsta->last_dsn_update +
					net_traversal_jiffies(sdata)) ||
		    time_before(jiffies, ifsta->last_dsn_update)) {
			dst_dsn = ++ifsta->dsn;
			ifsta->last_dsn_update = jiffies;
		}
*/
	} else {
		mpath = mesh_path_lookup(dst_addr);
		if (mpath) {
			if ((!(mpath->flags & MESH_PATH_DSN_VALID)) ||
					DSN_LT(mpath->dsn, dst_dsn)) {
				mpath->dsn = dst_dsn;
				mpath->flags &= MESH_PATH_DSN_VALID;
			} else if ((!(dst_flags & MP_F_DO)) &&
					(mpath->flags & MESH_PATH_ACTIVE)) {
				reply = true;
				metric = mpath->metric;
				dst_dsn = mpath->dsn;
				if (dst_flags & MP_F_RF)
					dst_flags |= MP_F_DO;
				else
					forward = false;
			}
		}
	}

	if (reply) {
		lifetime = PREQ_IE_LIFETIME(preq_elem);
		ttl = hapd->mconf->dot11MeshTTL;
		if (ttl != 0)
			mesh_path_sel_frame_tx(MPATH_PREP, 0, dst_addr,
				(dst_dsn), 0, orig_addr,
				(orig_dsn), mgmt->sa, 0, ttl,
				(lifetime), (metric),
				0, hapd);
		else
			hapd->mshstats.dropped_frames_ttl++;
	}

	if (forward) {
		u32 preq_id;
		u8 hopcount, flags;
		u8 broadcast[ETH_ALEN];

		ttl = PREQ_IE_TTL(preq_elem);
		lifetime = PREQ_IE_LIFETIME(preq_elem);
		if (ttl <= 1) {
			hapd->mshstats.dropped_frames_ttl++;
			return;
		}
		--ttl;
		memset(broadcast, 0xff, ETH_ALEN);
		flags = PREQ_IE_FLAGS(preq_elem);
		preq_id = PREQ_IE_PREQ_ID(preq_elem);
		hopcount = PREQ_IE_HOPCOUNT(preq_elem) + 1;
		mesh_path_sel_frame_tx(MPATH_PREQ, flags, orig_addr,
				(orig_dsn), dst_flags, dst_addr,
				(dst_dsn), broadcast,
				hopcount, ttl, (lifetime),
				(metric), (preq_id),
				hapd);
		hapd->mshstats.fwded_frames++;
	}
}


static void hwmp_prep_frame_process(struct hostapd_data *hapd,
				    struct ieee80211_mgmt *mgmt,
				    u8 *prep_elem, u32 metric)
{
	struct mesh_path *mpath;
	u8 *dst_addr, *orig_addr;
	u8 ttl, hopcount, flags;
	u8 next_hop[ETH_ALEN];
	u32 dst_dsn, orig_dsn, lifetime;

	/* Note that we divert from the draft nomenclature and denominate
	 * destination to what the draft refers to as origininator. So in this
	 * function destnation refers to the final destination of the PREP,
	 * which corresponds with the originator of the PREQ which this PREP
	 * replies
	 */
	dst_addr = PREP_IE_DST_ADDR(prep_elem);
	if (memcmp(dst_addr, hapd->own_addr, ETH_ALEN) == 0)
		/* destination, no forwarding required */
		return;

	ttl = PREP_IE_TTL(prep_elem);
	if (ttl <= 1) {
		hapd->mshstats.dropped_frames_ttl++;
		return;
	}

	mpath = mesh_path_lookup(dst_addr);
	if (mpath){
	}
	else
		goto fail;
	if (!(mpath->flags & MESH_PATH_ACTIVE)) {
		goto fail;
	}
	memcpy(next_hop, mpath->next_hop->addr, ETH_ALEN);
	--ttl;
	flags = PREP_IE_FLAGS(prep_elem);
	lifetime = PREP_IE_LIFETIME(prep_elem);
	hopcount = PREP_IE_HOPCOUNT(prep_elem) + 1;
	orig_addr = PREP_IE_ORIG_ADDR(prep_elem);
	dst_dsn = PREP_IE_DST_DSN(prep_elem);
	orig_dsn = PREP_IE_ORIG_DSN(prep_elem);

	mesh_path_sel_frame_tx(MPATH_PREP, flags, orig_addr,
		(orig_dsn), 0, dst_addr,
		(dst_dsn), mpath->next_hop->addr, hopcount, ttl,
		(lifetime), (metric),
		0, hapd);
	hapd->mshstats.fwded_frames++;
	return;

fail:
	hapd->mshstats.dropped_frames_no_route++;
	return;
}


static void hwmp_perr_frame_process(struct hostapd_data *hapd,
			     struct ieee80211_mgmt *mgmt, u8 *perr_elem)
{
	struct mesh_path *mpath;
	u8 *ta, *dst_addr;
	u32 dst_dsn;

	ta = mgmt->sa;
	dst_addr = PERR_IE_DST_ADDR(perr_elem);
	dst_dsn = PERR_IE_DST_DSN(perr_elem);
	mpath = mesh_path_lookup(dst_addr);
	if (mpath) {
		if (mpath->flags & MESH_PATH_ACTIVE &&
		    memcmp(ta, mpath->next_hop->addr, ETH_ALEN) == 0 &&
		    (!(mpath->flags & MESH_PATH_DSN_VALID) ||
		    DSN_GT(dst_dsn, mpath->dsn))) {
			mpath->flags &= ~MESH_PATH_ACTIVE;
			mpath->dsn = dst_dsn;
			mesh_path_error_tx(dst_addr, (dst_dsn),
					   hapd->broadcast, hapd);
		} else {
		}
	}
}


void mesh_rx_path_sel_frame(struct hostapd_data *hapd,
			    struct ieee80211_mgmt *mgmt,
			    size_t len)
{
	struct ieee802_11_elems elems;
	size_t baselen;
	u32 last_hop_metric;

	baselen = (u8 *) mgmt->u.action.u.mesh_action.variable - (u8 *) mgmt;
	if(ParseFailed == ieee802_11_parse_elems(hapd, mgmt->u.action.u.mesh_action.variable,
			len - baselen, &elems, 0) ) {
		return;
	}

	switch (mgmt->u.action.u.mesh_action.action_code) {
	case MPATH_PREQ:
		if (!elems.preq || elems.preq_len != 37)
			/* Right now we support just 1 destination and no AE */
			return;
		last_hop_metric = hwmp_route_info_get(hapd, mgmt, elems.preq);
		if (!last_hop_metric)
			return;
		hwmp_preq_frame_process(hapd, mgmt, elems.preq, last_hop_metric);
		break;
	case MPATH_PREP:
		if (!elems.prep || elems.prep_len != 31)
			/* Right now we support no AE */
			return;
		last_hop_metric = hwmp_route_info_get(hapd, mgmt, elems.prep);
		if (!last_hop_metric)
			return;
		hwmp_prep_frame_process(hapd, mgmt, elems.prep, last_hop_metric);
		break;
	case MPATH_PERR:
		if (!elems.perr || elems.perr_len != 12)
			/* Right now we support only one destination per PERR */
			return;
		hwmp_perr_frame_process(hapd, mgmt, elems.perr);
	default:
		return;
	}

}


/**
 * mesh_queue_preq - queue a PREQ to a given destination
 *
 * @mpath: mesh path to discover
 * @flags: special attributes of the PREQ to be sent
 *
 * Locking: the function must be called from within a rcu read lock block.
 *
 */
static void mesh_queue_preq(struct hostapd_data *hapd, struct mesh_path *mpath, u8 flags)
{
	struct mesh_preq_queue *preq_node;

	preq_node = wpa_zalloc(sizeof(struct mesh_preq_queue));
	if (!preq_node) {
		printf("Mesh HWMP: could not allocate PREQ node\n");
		return;
	}

	if (hapd->preq_queue_len == MAX_PREQ_QUEUE_LEN) {
		free(preq_node);
		printf("Mesh HWMP: PREQ node queue full\n");
		return;
	}

	memcpy(preq_node->dst, mpath->dst, ETH_ALEN);
	preq_node->flags = flags;

	list_add_tail(&preq_node->list, &hapd->preq_queue.list);
	++hapd->preq_queue_len;

/*
	if (time_after(jiffies, ifsta->last_preq + min_preq_int_jiff(sdata)))
		queue_work(sdata->local->hw.workqueue, &ifsta->work);

	else if (time_before(jiffies, ifsta->last_preq)) {
		// avoid long wait if did not send preqs for a long time
		// and jiffies wrapped around
		//
		ifsta->last_preq = jiffies - min_preq_int_jiff(sdata) - 1;
		queue_work(sdata->local->hw.workqueue, &ifsta->work);
	} else
		mod_timer(&ifsta->mesh_path_timer, ifsta->last_preq +

						min_preq_int_jiff(sdata));
*/
}

void hwmp_proactive_preq(void *eloop_ctx, void *timeout_ctx)
{
	struct hostapd_data *hapd = eloop_ctx;
	eloop_register_timeout(2, 0, hwmp_proactive_preq, hapd, NULL);

	printf("%s:\n", __func__);

	mesh_path_sel_frame_tx(MPATH_PREQ, MP_F_RF, hapd->own_addr,
			hapd->dsn, MP_F_RF, hapd->broadcast,
			0, hapd->broadcast, 0,
			hapd->mconf->dot11MeshTTL, 0, 0,
			hapd->preq_id++, hapd);
}

/**
 * mesh_path_start_discovery - launch a path discovery from the PREQ queue
 *
 * @dev: local mesh interface
 */
void mesh_path_start_discovery(struct hostapd_data *hapd)
{
	struct mesh_preq_queue *preq_node;
	struct mesh_path *mpath;
	u8 ttl, dst_flags;
	u32 lifetime;
	u8 broadcast[ETH_ALEN];

	if (!hapd->preq_queue_len) {
		return;
	}

	preq_node = list_first_entry(&hapd->preq_queue.list,
			struct mesh_preq_queue, list);
	list_del(&preq_node->list);
	--hapd->preq_queue_len;

	mpath = mesh_path_lookup(preq_node->dst);
	if (!mpath)
		goto enddiscovery;

	if (preq_node->flags & PREQ_Q_F_START) {
		if (mpath->flags & MESH_PATH_RESOLVING) {
			goto enddiscovery;
		} else {
			mpath->flags &= ~MESH_PATH_RESOLVED;
			mpath->flags |= MESH_PATH_RESOLVING;
			mpath->discovery_retries = 0;
			mpath->discovery_timeout = hapd->mconf->min_discovery_timeout;
		}
	} else if (!(mpath->flags & MESH_PATH_RESOLVING) ||
			mpath->flags & MESH_PATH_RESOLVED) {
		mpath->flags &= ~MESH_PATH_RESOLVING;
		goto enddiscovery;
	}
/*
	ifsta->last_preq = jiffies;

	if (time_after(jiffies, ifsta->last_dsn_update +
				net_traversal_jiffies(sdata)) ||
	    time_before(jiffies, ifsta->last_dsn_update)) {
		++ifsta->dsn;
		sdata->u.sta.last_dsn_update = jiffies;
	}
*/
	lifetime = hapd->mconf->dot11MeshHWMPactivePathTimeout;
	ttl = hapd->mconf->dot11MeshTTL;
	if (ttl == 0) {
		hapd->mshstats.dropped_frames_ttl++;
		goto enddiscovery;
	}

	if (preq_node->flags & PREQ_Q_F_REFRESH)
		dst_flags = MP_F_DO;
	else
		dst_flags = MP_F_RF;

	memset(broadcast, 0xff, ETH_ALEN);
	mesh_path_sel_frame_tx(MPATH_PREQ, 0, hapd->own_addr,
			hapd->dsn, dst_flags, mpath->dst,
			mpath->dsn, broadcast, 0,
			ttl, lifetime, 0,
			hapd->preq_id++, hapd);
//	mod_timer(&mpath->timer, jiffies + mpath->discovery_timeout);

enddiscovery:
	free(preq_node);
}


void mesh_path_timer(unsigned long data)
{
	struct mesh_path *mpath;
/*
	mpath = (struct mesh_path *) data;
	if (!mpath)
		goto endmpathtimer;
	if (mpath->flags & MESH_PATH_RESOLVED ||
			(!(mpath->flags & MESH_PATH_RESOLVING)))
		mpath->flags &= ~(MESH_PATH_RESOLVING | MESH_PATH_RESOLVED);
	else if (mpath->discovery_retries < max_preq_retries(sdata)) {
		++mpath->discovery_retries;
		mpath->discovery_timeout *= 2;
		mesh_queue_preq(mpath, 0);
	} else {
		mpath->flags = 0;
		mpath->exp_time = jiffies;
		mesh_path_flush_pending(mpath);
	}
endmpathtimer:
*/
}

