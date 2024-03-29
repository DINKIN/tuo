/*
 * Copyright 2002-2005, Instant802 Networks, Inc.
 * Copyright 2005, Devicescape Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <linux/module.h>

#include <net/mac80211.h>
#include "ieee80211_i.h"
#include "ieee80211_rate.h"
#include "debugfs.h"


/* This is a minimal implementation of TX rate controlling that can be used
 * as the default when no improved mechanisms are available. */

#define RATE_CONTROL_NUM_DOWN 20
#define RATE_CONTROL_NUM_UP   15

#define RATE_CONTROL_EMERG_DEC 2
#define RATE_CONTROL_INTERVAL (HZ / 20)
#define RATE_CONTROL_MIN_TX 10

static void rate_control_rate_inc(struct ieee80211_local *local,
				  struct sta_info *sta)
{
	struct ieee80211_sub_if_data *sdata;
	struct ieee80211_supported_band *sband;
	int i = sta->txrate_idx;
	int maxrate;

	sdata = sta->sdata;
	if (sdata->bss && sdata->bss->force_unicast_rateidx > -1) {
		/* forced unicast rate - do not change STA rate */
		return;
	}

	sband = local->hw.wiphy->bands[local->hw.conf.channel->band];
	maxrate = sdata->bss ? sdata->bss->max_ratectrl_rateidx : -1;

	if (i > sband->n_bitrates)
		i = sband->n_bitrates - 2;

	while (i + 1 < sband->n_bitrates) {
		i++;
		if (rate_supported(sta, sband->band, i) &&
		    (maxrate < 0 || i <= maxrate)) {
			sta->txrate_idx = i;
			break;
		}
	}
}


static void rate_control_rate_dec(struct ieee80211_local *local,
				  struct sta_info *sta)
{
	struct ieee80211_sub_if_data *sdata;
	struct ieee80211_supported_band *sband;
	int i = sta->txrate_idx;

	sdata = sta->sdata;
	if (sdata->bss && sdata->bss->force_unicast_rateidx > -1) {
		/* forced unicast rate - do not change STA rate */
		return;
	}

	sband = local->hw.wiphy->bands[local->hw.conf.channel->band];
	if (i > sband->n_bitrates)
		i = sband->n_bitrates;

	while (i > 0) {
		i--;
		if (rate_supported(sta, sband->band, i)) {
			sta->txrate_idx = i;
			break;
		}
	}
}

struct global_rate_control {
	int dummy;
};

struct sta_rate_control {
	unsigned long last_rate_change;
	u32 tx_num_failures;
	u32 tx_num_xmit;

	unsigned long avg_rate_update;
	u32 tx_avg_rate_sum;
	u32 tx_avg_rate_num;

#ifdef CONFIG_MAC80211_DEBUGFS
	struct dentry *tx_avg_rate_sum_dentry;
	struct dentry *tx_avg_rate_num_dentry;
#endif
};


static void rate_control_simple_tx_status(void *priv, struct net_device *dev,
					  struct sk_buff *skb,
					  struct ieee80211_tx_status *status)
{
	struct ieee80211_local *local = wdev_priv(dev->ieee80211_ptr);
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
	struct sta_info *sta;
	struct sta_rate_control *srctrl;

	rcu_read_lock();

	sta = sta_info_get(local, hdr->addr1);

	if (!sta)
		goto unlock;

	srctrl = sta->rate_ctrl_priv;
	srctrl->tx_num_xmit++;
	if (status->excessive_retries) {
		srctrl->tx_num_failures++;
		sta->tx_retry_failed++;
		sta->tx_num_consecutive_failures++;
		sta->tx_num_mpdu_fail++;
	} else {
		sta->tx_num_consecutive_failures = 0;
		sta->tx_num_mpdu_ok++;
	}
	sta->tx_retry_count += status->retry_count;
	sta->tx_num_mpdu_fail += status->retry_count;

	if (time_after(jiffies,
		       srctrl->last_rate_change + RATE_CONTROL_INTERVAL) &&
		srctrl->tx_num_xmit > RATE_CONTROL_MIN_TX) {
		u32 per_failed;
		srctrl->last_rate_change = jiffies;

		per_failed = (100 * sta->tx_num_mpdu_fail) /
			(sta->tx_num_mpdu_fail + sta->tx_num_mpdu_ok);
		/* TODO: calculate average per_failed to make adjusting
		 * parameters easier */
#if 0
		if (net_ratelimit()) {
			printk(KERN_DEBUG "MPDU fail=%d ok=%d per_failed=%d\n",
			       sta->tx_num_mpdu_fail, sta->tx_num_mpdu_ok,
			       per_failed);
		}
#endif

		/*
		 * XXX: Make these configurable once we have an
		 * interface to the rate control algorithms
		 */
		if (per_failed > RATE_CONTROL_NUM_DOWN) {
			rate_control_rate_dec(local, sta);
		} else if (per_failed < RATE_CONTROL_NUM_UP) {
			rate_control_rate_inc(local, sta);
		}
		srctrl->tx_avg_rate_sum += status->control.tx_rate->bitrate;
		srctrl->tx_avg_rate_num++;
		srctrl->tx_num_failures = 0;
		srctrl->tx_num_xmit = 0;
	} else if (sta->tx_num_consecutive_failures >=
		   RATE_CONTROL_EMERG_DEC) {
		rate_control_rate_dec(local, sta);
	}

	if (time_after(jiffies, srctrl->avg_rate_update + 60 * HZ)) {
		srctrl->avg_rate_update = jiffies;
		if (srctrl->tx_avg_rate_num > 0) {
#ifdef CONFIG_MAC80211_VERBOSE_DEBUG
			DECLARE_MAC_BUF(mac);
			printk(KERN_DEBUG "%s: STA %s Average rate: "
			       "%d (%d/%d)\n",
			       dev->name, print_mac(mac, sta->addr),
			       srctrl->tx_avg_rate_sum /
			       srctrl->tx_avg_rate_num,
			       srctrl->tx_avg_rate_sum,
			       srctrl->tx_avg_rate_num);
#endif /* CONFIG_MAC80211_VERBOSE_DEBUG */
			srctrl->tx_avg_rate_sum = 0;
			srctrl->tx_avg_rate_num = 0;
		}
	}

 unlock:
	rcu_read_unlock();
}


static void
rate_control_simple_get_rate(void *priv, struct net_device *dev,
			     struct ieee80211_supported_band *sband,
			     struct sk_buff *skb,
			     struct rate_selection *sel)
{
	struct ieee80211_local *local = wdev_priv(dev->ieee80211_ptr);
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
	struct ieee80211_sub_if_data *sdata;
	struct sta_info *sta;
	int rateidx;
	u16 fc;

	rcu_read_lock();

	sta = sta_info_get(local, hdr->addr1);

	/* Send management frames and broadcast/multicast data using lowest
	 * rate. */
	fc = le16_to_cpu(hdr->frame_control);
	if ((fc & IEEE80211_FCTL_FTYPE) != IEEE80211_FTYPE_DATA ||
	    is_multicast_ether_addr(hdr->addr1) || !sta) {
		sel->rate = rate_lowest(local, sband, sta);
		rcu_read_unlock();
		return;
	}

	/* If a forced rate is in effect, select it. */
	sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	if (sdata->bss && sdata->bss->force_unicast_rateidx > -1)
		sta->txrate_idx = sdata->bss->force_unicast_rateidx;

	rateidx = sta->txrate_idx;

	if (rateidx >= sband->n_bitrates)
		rateidx = sband->n_bitrates - 1;

	sta->last_txrate_idx = rateidx;

	rcu_read_unlock();

	sel->rate = &sband->bitrates[rateidx];
}


static void rate_control_simple_rate_init(void *priv, void *priv_sta,
					  struct ieee80211_local *local,
					  struct sta_info *sta)
{
	struct ieee80211_supported_band *sband;

	sband = local->hw.wiphy->bands[local->hw.conf.channel->band];

	/* TODO: This routine should consider using RSSI from previous packets
	 * as we need to have IEEE 802.1X auth succeed immediately after assoc..
	 * Until that method is implemented, we will use the lowest supported rate
	 * as a workaround, */
	sta->txrate_idx = rate_lowest_index(local, sband, sta);
}


static void * rate_control_simple_alloc(struct ieee80211_local *local)
{
	struct global_rate_control *rctrl;

	rctrl = kzalloc(sizeof(*rctrl), GFP_ATOMIC);

	return rctrl;
}


static void rate_control_simple_free(void *priv)
{
	struct global_rate_control *rctrl = priv;
	kfree(rctrl);
}


static void rate_control_simple_clear(void *priv)
{
}


static void * rate_control_simple_alloc_sta(void *priv, gfp_t gfp)
{
	struct sta_rate_control *rctrl;

	rctrl = kzalloc(sizeof(*rctrl), gfp);

	return rctrl;
}


static void rate_control_simple_free_sta(void *priv, void *priv_sta)
{
	struct sta_rate_control *rctrl = priv_sta;
	kfree(rctrl);
}

#ifdef CONFIG_MAC80211_DEBUGFS

static int open_file_generic(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t sta_tx_avg_rate_sum_read(struct file *file,
					char __user *userbuf,
					size_t count, loff_t *ppos)
{
	struct sta_rate_control *srctrl = file->private_data;
	char buf[20];

	sprintf(buf, "%d\n", srctrl->tx_avg_rate_sum);
	return simple_read_from_buffer(userbuf, count, ppos, buf, strlen(buf));
}

static const struct file_operations sta_tx_avg_rate_sum_ops = {
	.read = sta_tx_avg_rate_sum_read,
	.open = open_file_generic,
};

static ssize_t sta_tx_avg_rate_num_read(struct file *file,
					char __user *userbuf,
					size_t count, loff_t *ppos)
{
	struct sta_rate_control *srctrl = file->private_data;
	char buf[20];

	sprintf(buf, "%d\n", srctrl->tx_avg_rate_num);
	return simple_read_from_buffer(userbuf, count, ppos, buf, strlen(buf));
}

static const struct file_operations sta_tx_avg_rate_num_ops = {
	.read = sta_tx_avg_rate_num_read,
	.open = open_file_generic,
};

static void rate_control_simple_add_sta_debugfs(void *priv, void *priv_sta,
						struct dentry *dir)
{
	struct sta_rate_control *srctrl = priv_sta;

	srctrl->tx_avg_rate_num_dentry =
		debugfs_create_file("rc_simple_sta_tx_avg_rate_num", 0400,
				    dir, srctrl, &sta_tx_avg_rate_num_ops);
	srctrl->tx_avg_rate_sum_dentry =
		debugfs_create_file("rc_simple_sta_tx_avg_rate_sum", 0400,
				    dir, srctrl, &sta_tx_avg_rate_sum_ops);
}

static void rate_control_simple_remove_sta_debugfs(void *priv, void *priv_sta)
{
	struct sta_rate_control *srctrl = priv_sta;

	debugfs_remove(srctrl->tx_avg_rate_sum_dentry);
	debugfs_remove(srctrl->tx_avg_rate_num_dentry);
}
#endif

static struct rate_control_ops mac80211_rcsimple = {
	.name = "simple",
	.tx_status = rate_control_simple_tx_status,
	.get_rate = rate_control_simple_get_rate,
	.rate_init = rate_control_simple_rate_init,
	.clear = rate_control_simple_clear,
	.alloc = rate_control_simple_alloc,
	.free = rate_control_simple_free,
	.alloc_sta = rate_control_simple_alloc_sta,
	.free_sta = rate_control_simple_free_sta,
#ifdef CONFIG_MAC80211_DEBUGFS
	.add_sta_debugfs = rate_control_simple_add_sta_debugfs,
	.remove_sta_debugfs = rate_control_simple_remove_sta_debugfs,
#endif
};

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple rate control algorithm");

int __init rc80211_simple_init(void)
{
	return ieee80211_rate_control_register(&mac80211_rcsimple);
}

void rc80211_simple_exit(void)
{
	ieee80211_rate_control_unregister(&mac80211_rcsimple);
}

#ifdef CONFIG_MAC80211_RC_SIMPLE_MODULE
module_init(rc80211_simple_init);
module_exit(rc80211_simple_exit);
#endif
