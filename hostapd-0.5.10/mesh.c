#include <includes.h>
#include <stdio.h>

#include "hostapd.h"
#include "sta_info.h"
#include "mesh.h"
#include "ieee802_11.h"

#define ACCEPT_PLINKS 0x80

int mesh_allocated = 0;

void ieee80211s_init(void)
{
	mesh_pathtbl_init();
	mesh_allocated = 1;
}

void ieee80211s_stop(void)
{
	mesh_allocated = 0;
	mesh_pathtbl_unregister();
}

void mesh_mgmt_ies_add(u8 *pos, struct hostapd_data *hapd)
{
//	struct ieee80211_local *local = wdev_priv(dev->ieee80211_ptr);
//	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
//	struct ieee80211_supported_band *sband;
//	int len, i, rate;

//	sband = local->hw.wiphy->bands[local->hw.conf.channel->band];
//	len = sband->n_bitrates;
//	if (len > 8)
//		len = 8;
/*
	*pos++ = WLAN_EID_SUPP_RATES;
	*pos++ = len;
	for (i = 0; i < len; i++) {
		rate = sband->bitrates[i].bitrate;
		*pos++ = (u8) (rate / 5);
	}

	if (sband->n_bitrates > len) {
		pos = skb_put(skb, sband->n_bitrates - len + 2);
		*pos++ = WLAN_EID_EXT_SUPP_RATES;
		*pos++ = sband->n_bitrates - len;
		for (i = len; i < sband->n_bitrates; i++) {
			rate = sband->bitrates[i].bitrate;
			*pos++ = (u8) (rate / 5);
		}
	}
*/
//	pos = skb_put(skb, 2 + sdata->u.sta.mesh_id_len);
	*pos++ = WLAN_EID_MESH_ID;
	*pos++ = hapd->mesh_id_len;
	if (hapd->mesh_id_len) {
		memcpy(pos, hapd->mesh_id, hapd->mesh_id_len);
		pos += hapd->mesh_id_len;
	}

	*pos++ = WLAN_EID_MESH_CONFIG;
	*pos++ = MESH_CFG_LEN;
	/* Version */
	*pos++ = 1;

	/* Active path selection protocol ID */
	memcpy(pos, hapd->mesh_pp_id, 4);
	pos += 4;

	/* Active path selection metric ID   */
	memcpy(pos, hapd->mesh_pm_id, 4);
	pos += 4;

	/* Congestion control mode identifier */
	memcpy(pos, hapd->mesh_cc_id, 4);
	pos += 4;

	/* Channel precedence:
	 * Not running simple channel unification protocol
	 */
	memset(pos, 0x00, 4);
	pos += 4;

	/* Mesh capability */
	*pos++ = ACCEPT_PLINKS;
	*pos++ = 0x00;

	return;
}

struct mesh_table *mesh_table_alloc(int size)
{
	struct mesh_table *newtbl;

	newtbl = wpa_zalloc(sizeof(struct mesh_table));
	if (!newtbl)
		return NULL;

	newtbl->hash_buckets = wpa_zalloc(sizeof(struct hlist_head) * size);

	if (!newtbl->hash_buckets) {
		free(newtbl);
		return NULL;
	}

	newtbl->size = size;
	newtbl->entries = 0;
	newtbl->hash_rnd = rand();

	return newtbl;
}

void mesh_table_free(struct mesh_table *tbl, bool free_leafs)
{
	struct hlist_head *mesh_hash;
	struct hlist_node *p, *q;
	int i;

	mesh_hash = tbl->hash_buckets;
	for (i = 0; i <= tbl->size; i++) {
		hlist_for_each_safe(p, q, &mesh_hash[i]) {
			tbl->free_node(p, free_leafs);
			tbl->entries--;
		}
	}
	free(tbl->hash_buckets);
	free(tbl);
}

