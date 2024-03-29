/*
 * Copyright (c) 2008 open80211s Ltd.
 * Author:     Luis Carlos Cobo <luisca@cozybit.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/unaligned.h>
#include "mesh.h"

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

static inline u32 u32_field_get(u8 *preq_elem, int offset, bool ae)
{
	if (ae)
		offset += 6;
	return le32_to_cpu(get_unaligned((__le32 *) (preq_elem + offset)));
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

#define net_traversal_jiffies(s) \
	msecs_to_jiffies(s->u.sta.mshcfg.dot11MeshHWMPnetDiameterTraversalTime)
#define default_lifetime(s) \
	MSEC_TO_TU(s->u.sta.mshcfg.dot11MeshHWMPactivePathTimeout)
#define min_preq_int_jiff(s) \
	(msecs_to_jiffies(s->u.sta.mshcfg.dot11MeshHWMPpreqMinInterval))
#define max_preq_retries(s) (s->u.sta.mshcfg.dot11MeshHWMPmaxPREQretries)
#define disc_timeout_jiff(s) \
	msecs_to_jiffies(sdata->u.sta.mshcfg.min_discovery_timeout)

enum mpath_frame_type {
	MPATH_PREQ = 0,
	MPATH_PREP,
	MPATH_PERR
};

static int mesh_path_sel_frame_tx(enum mpath_frame_type action, u8 flags,
		u8 *orig_addr, __le32 orig_dsn, u8 dst_flags, u8 *dst,
		__le32 dst_dsn, u8 *da, u8 hop_count, u8 ttl, __le32 lifetime,
		__le32 metric, __le32 preq_id, struct net_device *dev)
{
	struct ieee80211_local *local = wdev_priv(dev->ieee80211_ptr);
	struct sk_buff *skb = dev_alloc_skb(local->hw.extra_tx_headroom + 400);
	struct ieee80211_mgmt *mgmt;
	u8 *pos;
	int ie_len;

	if (!skb)
		return -1;
	skb_reserve(skb, local->hw.extra_tx_headroom);
	/* 25 is the size of the common mgmt part (24) plus the size of the
	 * common action part (1)
	 */
	mgmt = (struct ieee80211_mgmt *)
		skb_put(skb, 25 + sizeof(mgmt->u.action.u.mesh_action));
	memset(mgmt, 0, 25 + sizeof(mgmt->u.action.u.mesh_action));
	mgmt->frame_control = IEEE80211_FC(IEEE80211_FTYPE_MGMT,
					   IEEE80211_STYPE_ACTION);

	memcpy(mgmt->da, da, ETH_ALEN);
	memcpy(mgmt->sa, dev->dev_addr, ETH_ALEN);
	/* BSSID is left zeroed, wildcard value */
	mgmt->u.action.category = MESH_PATH_SEL_CATEGORY;
	mgmt->u.action.u.mesh_action.action_code = action;

	switch (action) {
	case MPATH_PREQ:
		ie_len = 37;
		pos = skb_put(skb, 2 + ie_len);
		*pos++ = WLAN_EID_PREQ;
		break;
	case MPATH_PREP:
		ie_len = 31;
		pos = skb_put(skb, 2 + ie_len);
		*pos++ = WLAN_EID_PREP;
		break;
	default:
		kfree(skb);
		return -ENOTSUPP;
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

	ieee80211_sta_tx(dev, skb, 0);
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
		struct net_device *dev)
{
	struct ieee80211_local *local = wdev_priv(dev->ieee80211_ptr);
	struct sk_buff *skb = dev_alloc_skb(local->hw.extra_tx_headroom + 400);
	struct ieee80211_mgmt *mgmt;
	u8 *pos;
	int ie_len;

	if (!skb)
		return -1;
	skb_reserve(skb, local->hw.extra_tx_headroom);
	/* 25 is the size of the common mgmt part (24) plus the size of the
	 * common action part (1)
	 */
	mgmt = (struct ieee80211_mgmt *)
		skb_put(skb, 25 + sizeof(mgmt->u.action.u.mesh_action));
	memset(mgmt, 0, 25 + sizeof(mgmt->u.action.u.mesh_action));
	mgmt->frame_control = IEEE80211_FC(IEEE80211_FTYPE_MGMT,
					   IEEE80211_STYPE_ACTION);

	memcpy(mgmt->da, ra, ETH_ALEN);
	memcpy(mgmt->sa, dev->dev_addr, ETH_ALEN);
	/* BSSID is left zeroed, wildcard value */
	mgmt->u.action.category = MESH_PATH_SEL_CATEGORY;
	mgmt->u.action.u.mesh_action.action_code = MPATH_PERR;
	ie_len = 12;
	pos = skb_put(skb, 2 + ie_len);
	*pos++ = WLAN_EID_PERR;
	*pos++ = ie_len;
	/* mode flags, reserved */
	*pos++ = 0;
	/* number of destinations */
	*pos++ = 1;
	memcpy(pos, dst, ETH_ALEN);
	pos += ETH_ALEN;
	memcpy(pos, &dst_dsn, 4);

	ieee80211_sta_tx(dev, skb, 0);
	return 0;
}

static u32 airtime_link_metric_get(struct ieee80211_local *local,
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

	sband = local->hw.wiphy->bands[local->hw.conf.channel->band];

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
static u32 hwmp_route_info_get(struct net_device *dev,
			    struct ieee80211_mgmt *mgmt,
			    u8 *hwmp_ie)
{
	struct ieee80211_local *local = wdev_priv(dev->ieee80211_ptr);
	struct mesh_path *mpath;
	struct sta_info *sta;
	bool fresh_info;
	u8 *orig_addr, *ta;
	u32 orig_dsn, orig_metric;
	unsigned long orig_lifetime, exp_time;
	u32 last_hop_metric, new_metric;
	bool process = true;
	u8 action = mgmt->u.action.u.mesh_action.action_code;

	rcu_read_lock();
	sta = sta_info_get(local, mgmt->sa);
	if (!sta) {
		rcu_read_unlock();
		return 0;
	}

	last_hop_metric = airtime_link_metric_get(local, sta);
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
		rcu_read_unlock();
		return 0;
	}
	new_metric = orig_metric + last_hop_metric;
	if (new_metric < orig_metric)
		new_metric = MAX_METRIC;
	exp_time = TU_TO_EXP_TIME(orig_lifetime);

	if (memcmp(orig_addr, dev->dev_addr, ETH_ALEN) == 0) {
		/* This MP is the originator, we are not interested in this
		 * frame, except for updating transmitter's path info.
		 */
		process = false;
		fresh_info = false;
	} else {
		mpath = mesh_path_lookup(orig_addr, dev);
		if (mpath) {
			spin_lock_bh(&mpath->state_lock);
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
			mesh_path_add(orig_addr, dev);
			mpath = mesh_path_lookup(orig_addr, dev);
			if (!mpath) {
				rcu_read_unlock();
				return 0;
			}
			spin_lock_bh(&mpath->state_lock);
		}

		if (fresh_info) {
			mesh_path_assign_nexthop(mpath, sta);
			mpath->flags |= MESH_PATH_DSN_VALID;
			mpath->metric = new_metric;
			mpath->dsn = orig_dsn;
			mpath->exp_time = time_after(mpath->exp_time, exp_time)
					  ?  mpath->exp_time : exp_time;
			mesh_path_activate(mpath);
			spin_unlock_bh(&mpath->state_lock);
			mesh_path_tx_pending(mpath);
			/* draft says preq_id should be saved to, but there does
			 * not seem to be any use for it, skipping by now
			 */
		} else
			spin_unlock_bh(&mpath->state_lock);
	}

	/* Update and check transmitter routing info */
	ta = mgmt->sa;
	if (memcmp(orig_addr, ta, ETH_ALEN) == 0)
		fresh_info = false;
	else {
		fresh_info = true;

		mpath = mesh_path_lookup(ta, dev);
		if (mpath) {
			spin_lock_bh(&mpath->state_lock);
			if ((mpath->flags & MESH_PATH_FIXED) ||
				((mpath->flags & MESH_PATH_ACTIVE) &&
					(last_hop_metric > mpath->metric)))
				fresh_info = false;
		} else {
			mesh_path_add(ta, dev);
			mpath = mesh_path_lookup(ta, dev);
			if (!mpath) {
				rcu_read_unlock();
				return 0;
			}
			spin_lock_bh(&mpath->state_lock);
		}

		if (fresh_info) {
			mesh_path_assign_nexthop(mpath, sta);
			mpath->flags &= ~MESH_PATH_DSN_VALID;
			mpath->metric = last_hop_metric;
			mpath->exp_time = time_after(mpath->exp_time, exp_time)
					  ?  mpath->exp_time : exp_time;
			mesh_path_activate(mpath);
			spin_unlock_bh(&mpath->state_lock);
			mesh_path_tx_pending(mpath);
		} else
			spin_unlock_bh(&mpath->state_lock);
	}

	rcu_read_unlock();

	return process ? new_metric : 0;
}

static void hwmp_preq_frame_process(struct net_device *dev,
				    struct ieee80211_mgmt *mgmt,
				    u8 *preq_elem, u32 metric) {
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_if_sta *ifsta = &sdata->u.sta;
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

	if (memcmp(dst_addr, dev->dev_addr, ETH_ALEN) == 0) {
		forward = false;
		reply = true;
		metric = 0;
		if (time_after(jiffies, ifsta->last_dsn_update +
					net_traversal_jiffies(sdata)) ||
		    time_before(jiffies, ifsta->last_dsn_update)) {
			dst_dsn = ++ifsta->dsn;
			ifsta->last_dsn_update = jiffies;
		}
	} else {
		rcu_read_lock();
		mpath = mesh_path_lookup(dst_addr, dev);
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
		rcu_read_unlock();
	}

	if (reply) {
		lifetime = PREQ_IE_LIFETIME(preq_elem);
		ttl = ifsta->mshcfg.dot11MeshTTL;
		if (ttl != 0)
			mesh_path_sel_frame_tx(MPATH_PREP, 0, dst_addr,
				cpu_to_le32(dst_dsn), 0, orig_addr,
				cpu_to_le32(orig_dsn), mgmt->sa, 0, ttl,
				cpu_to_le32(lifetime), cpu_to_le32(metric),
				0, dev);
		else
			ifsta->mshstats.dropped_frames_ttl++;
	}

	if (forward) {
		u32 preq_id;
		u8 hopcount, flags;

		ttl = PREQ_IE_TTL(preq_elem);
		lifetime = PREQ_IE_LIFETIME(preq_elem);
		if (ttl <= 1) {
			ifsta->mshstats.dropped_frames_ttl++;
			return;
		}
		--ttl;
		flags = PREQ_IE_FLAGS(preq_elem);
		preq_id = PREQ_IE_PREQ_ID(preq_elem);
		hopcount = PREQ_IE_HOPCOUNT(preq_elem) + 1;
		mesh_path_sel_frame_tx(MPATH_PREQ, flags, orig_addr,
				cpu_to_le32(orig_dsn), dst_flags, dst_addr,
				cpu_to_le32(dst_dsn), dev->broadcast,
				hopcount, ttl, cpu_to_le32(lifetime),
				cpu_to_le32(metric), cpu_to_le32(preq_id),
				dev);
		ifsta->mshstats.fwded_frames++;
	}
}


static void hwmp_prep_frame_process(struct net_device *dev,
				    struct ieee80211_mgmt *mgmt,
				    u8 *prep_elem, u32 metric)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
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
	if (memcmp(dst_addr, dev->dev_addr, ETH_ALEN) == 0)
		/* destination, no forwarding required */
		return;

	ttl = PREP_IE_TTL(prep_elem);
	if (ttl <= 1) {
		sdata->u.sta.mshstats.dropped_frames_ttl++;
		return;
	}

	rcu_read_lock();
	mpath = mesh_path_lookup(dst_addr, dev);
	if (mpath)
		spin_lock_bh(&mpath->state_lock);
	else
		goto fail;
	if (!(mpath->flags & MESH_PATH_ACTIVE)) {
		spin_unlock_bh(&mpath->state_lock);
		goto fail;
	}
	memcpy(next_hop, mpath->next_hop->addr, ETH_ALEN);
	spin_unlock_bh(&mpath->state_lock);
	--ttl;
	flags = PREP_IE_FLAGS(prep_elem);
	lifetime = PREP_IE_LIFETIME(prep_elem);
	hopcount = PREP_IE_HOPCOUNT(prep_elem) + 1;
	orig_addr = PREP_IE_ORIG_ADDR(prep_elem);
	dst_dsn = PREP_IE_DST_DSN(prep_elem);
	orig_dsn = PREP_IE_ORIG_DSN(prep_elem);

	mesh_path_sel_frame_tx(MPATH_PREP, flags, orig_addr,
		cpu_to_le32(orig_dsn), 0, dst_addr,
		cpu_to_le32(dst_dsn), mpath->next_hop->addr, hopcount, ttl,
		cpu_to_le32(lifetime), cpu_to_le32(metric),
		0, dev);
	rcu_read_unlock();
	sdata->u.sta.mshstats.fwded_frames++;
	return;

fail:
	rcu_read_unlock();
	sdata->u.sta.mshstats.dropped_frames_no_route++;
	return;
}

static void hwmp_perr_frame_process(struct net_device *dev,
			     struct ieee80211_mgmt *mgmt, u8 *perr_elem)
{
	struct mesh_path *mpath;
	u8 *ta, *dst_addr;
	u32 dst_dsn;

	ta = mgmt->sa;
	dst_addr = PERR_IE_DST_ADDR(perr_elem);
	dst_dsn = PERR_IE_DST_DSN(perr_elem);
	rcu_read_lock();
	mpath = mesh_path_lookup(dst_addr, dev);
	if (mpath) {
		spin_lock_bh(&mpath->state_lock);
		if (mpath->flags & MESH_PATH_ACTIVE &&
		    memcmp(ta, mpath->next_hop->addr, ETH_ALEN) == 0 &&
		    (!(mpath->flags & MESH_PATH_DSN_VALID) ||
		    DSN_GT(dst_dsn, mpath->dsn))) {
			mpath->flags &= ~MESH_PATH_ACTIVE;
			mpath->dsn = dst_dsn;
			spin_unlock_bh(&mpath->state_lock);
			mesh_path_error_tx(dst_addr, cpu_to_le32(dst_dsn),
					   dev->broadcast, dev);
		} else
			spin_unlock_bh(&mpath->state_lock);
	}
	rcu_read_unlock();
}



void mesh_rx_path_sel_frame(struct net_device *dev,
			    struct ieee80211_mgmt *mgmt,
			    size_t len)
{
	struct ieee802_11_elems elems;
	size_t baselen;
	u32 last_hop_metric;

	baselen = (u8 *) mgmt->u.action.u.mesh_action.variable - (u8 *) mgmt;
	ieee802_11_parse_elems(mgmt->u.action.u.mesh_action.variable,
			len - baselen, &elems);

	switch (mgmt->u.action.u.mesh_action.action_code) {
	case MPATH_PREQ:
		if (!elems.preq || elems.preq_len != 37)
			/* Right now we support just 1 destination and no AE */
			return;
		last_hop_metric = hwmp_route_info_get(dev, mgmt, elems.preq);
		if (!last_hop_metric)
			return;
		hwmp_preq_frame_process(dev, mgmt, elems.preq, last_hop_metric);
		break;
	case MPATH_PREP:
		if (!elems.prep || elems.prep_len != 31)
			/* Right now we support no AE */
			return;
		last_hop_metric = hwmp_route_info_get(dev, mgmt, elems.prep);
		if (!last_hop_metric)
			return;
		hwmp_prep_frame_process(dev, mgmt, elems.prep, last_hop_metric);
		break;
	case MPATH_PERR:
		if (!elems.perr || elems.perr_len != 12)
			/* Right now we support only one destination per PERR */
			return;
		hwmp_perr_frame_process(dev, mgmt, elems.perr);
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
static void mesh_queue_preq(struct mesh_path *mpath, u8 flags)
{
	struct ieee80211_sub_if_data *sdata =
		IEEE80211_DEV_TO_SUB_IF(mpath->dev);
	struct ieee80211_if_sta *ifsta = &sdata->u.sta;
	struct mesh_preq_queue *preq_node;

	preq_node = kmalloc(sizeof(struct mesh_preq_queue), GFP_KERNEL);
	if (!preq_node) {
		printk(KERN_DEBUG "Mesh HWMP: could not allocate PREQ node\n");
		return;
	}

	spin_lock(&ifsta->mesh_preq_queue_lock);
	if (ifsta->preq_queue_len == MAX_PREQ_QUEUE_LEN) {
		spin_unlock(&ifsta->mesh_preq_queue_lock);
		kfree(preq_node);
		if (printk_ratelimit())
			printk(KERN_DEBUG "Mesh HWMP: PREQ node queue full\n");
		return;
	}

	memcpy(preq_node->dst, mpath->dst, ETH_ALEN);
	preq_node->flags = flags;

	list_add_tail(&preq_node->list, &ifsta->preq_queue.list);
	++ifsta->preq_queue_len;
	spin_unlock(&ifsta->mesh_preq_queue_lock);

	if (time_after(jiffies, ifsta->last_preq + min_preq_int_jiff(sdata)))
		queue_work(sdata->local->hw.workqueue, &ifsta->work);

	else if (time_before(jiffies, ifsta->last_preq)) {
		/* avoid long wait if did not send preqs for a long time
		 * and jiffies wrapped around
		 */
		ifsta->last_preq = jiffies - min_preq_int_jiff(sdata) - 1;
		queue_work(sdata->local->hw.workqueue, &ifsta->work);
	} else
		mod_timer(&ifsta->mesh_path_timer, ifsta->last_preq +
						min_preq_int_jiff(sdata));
}

/**
 * mesh_path_start_discovery - launch a path discovery from the PREQ queue
 *
 * @dev: local mesh interface
 */
void mesh_path_start_discovery(struct net_device *dev)
{
	struct ieee80211_sub_if_data *sdata =
		IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_if_sta *ifsta = &sdata->u.sta;
	struct mesh_preq_queue *preq_node;
	struct mesh_path *mpath;
	u8 ttl, dst_flags;
	u32 lifetime;

	spin_lock(&ifsta->mesh_preq_queue_lock);
	if (!ifsta->preq_queue_len ||
		time_before(jiffies, ifsta->last_preq +
				min_preq_int_jiff(sdata))) {
		spin_unlock(&ifsta->mesh_preq_queue_lock);
		return;
	}

	preq_node = list_first_entry(&ifsta->preq_queue.list,
			struct mesh_preq_queue, list);
	list_del(&preq_node->list);
	--ifsta->preq_queue_len;
	spin_unlock(&ifsta->mesh_preq_queue_lock);

	rcu_read_lock();
	mpath = mesh_path_lookup(preq_node->dst, dev);
	if (!mpath)
		goto enddiscovery;

	spin_lock_bh(&mpath->state_lock);
	if (preq_node->flags & PREQ_Q_F_START) {
		if (mpath->flags & MESH_PATH_RESOLVING) {
			spin_unlock_bh(&mpath->state_lock);
			goto enddiscovery;
		} else {
			mpath->flags &= ~MESH_PATH_RESOLVED;
			mpath->flags |= MESH_PATH_RESOLVING;
			mpath->discovery_retries = 0;
			mpath->discovery_timeout = disc_timeout_jiff(sdata);
		}
	} else if (!(mpath->flags & MESH_PATH_RESOLVING) ||
			mpath->flags & MESH_PATH_RESOLVED) {
		mpath->flags &= ~MESH_PATH_RESOLVING;
		spin_unlock_bh(&mpath->state_lock);
		goto enddiscovery;
	}

	ifsta->last_preq = jiffies;

	if (time_after(jiffies, ifsta->last_dsn_update +
				net_traversal_jiffies(sdata)) ||
	    time_before(jiffies, ifsta->last_dsn_update)) {
		++ifsta->dsn;
		sdata->u.sta.last_dsn_update = jiffies;
	}
	lifetime = default_lifetime(sdata);
	ttl = sdata->u.sta.mshcfg.dot11MeshTTL;
	if (ttl == 0) {
		sdata->u.sta.mshstats.dropped_frames_ttl++;
		spin_unlock_bh(&mpath->state_lock);
		goto enddiscovery;
	}

	if (preq_node->flags & PREQ_Q_F_REFRESH)
		dst_flags = MP_F_DO;
	else
		dst_flags = MP_F_RF;

	spin_unlock_bh(&mpath->state_lock);
	mesh_path_sel_frame_tx(MPATH_PREQ, 0, dev->dev_addr,
			cpu_to_le32(ifsta->dsn), dst_flags, mpath->dst,
			cpu_to_le32(mpath->dsn), dev->broadcast, 0,
			ttl, cpu_to_le32(lifetime), 0,
			cpu_to_le32(ifsta->preq_id++), dev);
	mod_timer(&mpath->timer, jiffies + mpath->discovery_timeout);

enddiscovery:
	rcu_read_unlock();
	kfree(preq_node);
}

/**
 * ieee80211s_lookup_nexthop - put the appropriate next hop on a mesh frame
 *
 * @next_hop: output argument for next hop address
 * @skb: frame to be sent
 * @dev: network device the frame will be sent through
 *
 * Returns: 0 if the next hop was found. Nonzero otherwise. If no next hop is
 * found, the function will start a path discovery and queue the frame so it is
 * sent when the path is resolved. This means the caller must not free the skb
 * in this case.
 */
int mesh_nexthop_lookup(u8 *next_hop, struct sk_buff *skb,
		struct net_device *dev)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct sk_buff *skb_to_free = NULL;
	struct mesh_path *mpath;
	int err = 0;

	rcu_read_lock();
	mpath = mesh_path_lookup(skb->data, dev);

	if (!mpath) {
		mesh_path_add(skb->data, dev);
		mpath = mesh_path_lookup(skb->data, dev);
		if (!mpath) {
			dev_kfree_skb(skb);
			sdata->u.sta.mshstats.dropped_frames_no_route++;
			err = -ENOSPC;
			goto endlookup;
		}
	}

	if (mpath->flags & MESH_PATH_ACTIVE) {
		if (time_after(jiffies, mpath->exp_time -
			msecs_to_jiffies(sdata->u.sta.mshcfg.path_refresh_time))
				&& skb->pkt_type != PACKET_OTHERHOST
				&& !(mpath->flags & MESH_PATH_RESOLVING)
				&& !(mpath->flags & MESH_PATH_FIXED)) {
			mesh_queue_preq(mpath,
					PREQ_Q_F_START | PREQ_Q_F_REFRESH);
		}
		memcpy(next_hop, mpath->next_hop->addr,
				ETH_ALEN);
	} else {
		if (!(mpath->flags & MESH_PATH_RESOLVING)) {
			/* Start discovery only if it is not running yet */
			mesh_queue_preq(mpath, PREQ_Q_F_START);
		}

		if (skb_queue_len(&mpath->frame_queue) >=
				MESH_FRAME_QUEUE_LEN) {
			skb_to_free = mpath->frame_queue.next;
			skb_unlink(skb_to_free, &mpath->frame_queue);
		}

		skb_queue_tail(&mpath->frame_queue, skb);
		if (skb_to_free)
			mesh_path_discard_frame(skb_to_free, dev);
		err = -ENOENT;
	}

endlookup:
	rcu_read_unlock();
	return err;
}

void mesh_path_timer(unsigned long data)
{
	struct ieee80211_sub_if_data *sdata;
	struct mesh_path *mpath;

	rcu_read_lock();
	mpath = (struct mesh_path *) data;
	mpath = rcu_dereference(mpath);
	if (!mpath)
		goto endmpathtimer;
	spin_lock_bh(&mpath->state_lock);
	sdata = IEEE80211_DEV_TO_SUB_IF(mpath->dev);
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

	spin_unlock_bh(&mpath->state_lock);
endmpathtimer:
	rcu_read_unlock();
}
