/*
 * Copyright 2003-2005	Devicescape Software, Inc.
 * Copyright (c) 2006	Jiri Benc <jbenc@suse.cz>
 * Copyright 2007	Johannes Berg <johannes@sipsolutions.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kobject.h>
#include "ieee80211_i.h"
#include "key.h"
#include "debugfs.h"
#include "debugfs_key.h"

#define KEY_READ(name, prop, buflen, format_string)			\
static ssize_t key_##name##_read(struct file *file,			\
				 char __user *userbuf,			\
				 size_t count, loff_t *ppos)		\
{									\
	char buf[buflen];						\
	struct ieee80211_key *key = file->private_data;			\
	int res = scnprintf(buf, buflen, format_string, key->prop);	\
	return simple_read_from_buffer(userbuf, count, ppos, buf, res);	\
}
#define KEY_READ_D(name) KEY_READ(name, name, 20, "%d\n")
#define KEY_READ_X(name) KEY_READ(name, name, 20, "0x%x\n")

#define KEY_OPS(name)							\
static const struct file_operations key_ ##name## _ops = {		\
	.read = key_##name##_read,					\
	.open = mac80211_open_file_generic,				\
}

#define KEY_FILE(name, format)						\
		 KEY_READ_##format(name)				\
		 KEY_OPS(name)

#define KEY_CONF_READ(name, buflen, format_string)			\
	KEY_READ(conf_##name, conf.name, buflen, format_string)
#define KEY_CONF_READ_D(name) KEY_CONF_READ(name, 20, "%d\n")

#define KEY_CONF_OPS(name)						\
static const struct file_operations key_ ##name## _ops = {		\
	.read = key_conf_##name##_read,					\
	.open = mac80211_open_file_generic,				\
}

#define KEY_CONF_FILE(name, format)					\
		 KEY_CONF_READ_##format(name)				\
		 KEY_CONF_OPS(name)

KEY_CONF_FILE(keylen, D);
KEY_CONF_FILE(keyidx, D);
KEY_CONF_FILE(hw_key_idx, D);
KEY_FILE(flags, X);
KEY_FILE(tx_rx_count, D);
KEY_READ(ifindex, sdata->dev->ifindex, 20, "%d\n");
KEY_OPS(ifindex);

static ssize_t key_algorithm_read(struct file *file,
				  char __user *userbuf,
				  size_t count, loff_t *ppos)
{
	char *alg;
	struct ieee80211_key *key = file->private_data;

	switch (key->conf.alg) {
	case ALG_WEP:
		alg = "WEP\n";
		break;
	case ALG_TKIP:
		alg = "TKIP\n";
		break;
	case ALG_CCMP:
		alg = "CCMP\n";
		break;
	default:
		return 0;
	}
	return simple_read_from_buffer(userbuf, count, ppos, alg, strlen(alg));
}
KEY_OPS(algorithm);

static ssize_t key_tx_spec_read(struct file *file, char __user *userbuf,
				size_t count, loff_t *ppos)
{
	const u8 *tpn;
	char buf[20];
	int len;
	struct ieee80211_key *key = file->private_data;

	switch (key->conf.alg) {
	case ALG_WEP:
		len = scnprintf(buf, sizeof(buf), "\n");
		break;
	case ALG_TKIP:
		len = scnprintf(buf, sizeof(buf), "%08x %04x\n",
				key->u.tkip.iv32,
				key->u.tkip.iv16);
		break;
	case ALG_CCMP:
		tpn = key->u.ccmp.tx_pn;
		len = scnprintf(buf, sizeof(buf), "%02x%02x%02x%02x%02x%02x\n",
				tpn[0], tpn[1], tpn[2], tpn[3], tpn[4], tpn[5]);
		break;
	default:
		return 0;
	}
	return simple_read_from_buffer(userbuf, count, ppos, buf, len);
}
KEY_OPS(tx_spec);

static ssize_t key_rx_spec_read(struct file *file, char __user *userbuf,
				size_t count, loff_t *ppos)
{
	struct ieee80211_key *key = file->private_data;
	char buf[14*NUM_RX_DATA_QUEUES+1], *p = buf;
	int i, len;
	const u8 *rpn;

	switch (key->conf.alg) {
	case ALG_WEP:
		len = scnprintf(buf, sizeof(buf), "\n");
		break;
	case ALG_TKIP:
		for (i = 0; i < NUM_RX_DATA_QUEUES; i++)
			p += scnprintf(p, sizeof(buf)+buf-p,
				       "%08x %04x\n",
				       key->u.tkip.iv32_rx[i],
				       key->u.tkip.iv16_rx[i]);
		len = p - buf;
		break;
	case ALG_CCMP:
		for (i = 0; i < NUM_RX_DATA_QUEUES; i++) {
			rpn = key->u.ccmp.rx_pn[i];
			p += scnprintf(p, sizeof(buf)+buf-p,
				       "%02x%02x%02x%02x%02x%02x\n",
				       rpn[0], rpn[1], rpn[2],
				       rpn[3], rpn[4], rpn[5]);
		}
		len = p - buf;
		break;
	default:
		return 0;
	}
	return simple_read_from_buffer(userbuf, count, ppos, buf, len);
}
KEY_OPS(rx_spec);

static ssize_t key_replays_read(struct file *file, char __user *userbuf,
				size_t count, loff_t *ppos)
{
	struct ieee80211_key *key = file->private_data;
	char buf[20];
	int len;

	if (key->conf.alg != ALG_CCMP)
		return 0;
	len = scnprintf(buf, sizeof(buf), "%u\n", key->u.ccmp.replays);
	return simple_read_from_buffer(userbuf, count, ppos, buf, len);
}
KEY_OPS(replays);

static ssize_t key_key_read(struct file *file, char __user *userbuf,
			    size_t count, loff_t *ppos)
{
	struct ieee80211_key *key = file->private_data;
	int i, res, bufsize = 2 * key->conf.keylen + 2;
	char *buf = kmalloc(bufsize, GFP_KERNEL);
	char *p = buf;

	for (i = 0; i < key->conf.keylen; i++)
		p += scnprintf(p, bufsize + buf - p, "%02x", key->conf.key[i]);
	p += scnprintf(p, bufsize+buf-p, "\n");
	res = simple_read_from_buffer(userbuf, count, ppos, buf, p - buf);
	kfree(buf);
	return res;
}
KEY_OPS(key);

#define DEBUGFS_ADD(name) \
	key->debugfs.name = debugfs_create_file(#name, 0400,\
				key->debugfs.dir, key, &key_##name##_ops);

void ieee80211_debugfs_key_add(struct ieee80211_key *key)
  {
	static int keycount;
	char buf[50];
	DECLARE_MAC_BUF(mac);
	struct sta_info *sta;

	if (!key->local->debugfs.keys)
		return;

	sprintf(buf, "%d", keycount);
	key->debugfs.cnt = keycount;
	keycount++;
	key->debugfs.dir = debugfs_create_dir(buf,
					key->local->debugfs.keys);

	if (!key->debugfs.dir)
		return;

	rcu_read_lock();
	sta = rcu_dereference(key->sta);
	if (sta)
		sprintf(buf, "../../stations/%s", print_mac(mac, sta->addr));
	rcu_read_unlock();

	/* using sta as a boolean is fine outside RCU lock */
	if (sta)
		key->debugfs.stalink =
			debugfs_create_symlink("station", key->debugfs.dir, buf);

	DEBUGFS_ADD(keylen);
	DEBUGFS_ADD(flags);
	DEBUGFS_ADD(keyidx);
	DEBUGFS_ADD(hw_key_idx);
	DEBUGFS_ADD(tx_rx_count);
	DEBUGFS_ADD(algorithm);
	DEBUGFS_ADD(tx_spec);
	DEBUGFS_ADD(rx_spec);
	DEBUGFS_ADD(replays);
	DEBUGFS_ADD(key);
	DEBUGFS_ADD(ifindex);
};

#define DEBUGFS_DEL(name) \
	debugfs_remove(key->debugfs.name); key->debugfs.name = NULL;

void ieee80211_debugfs_key_remove(struct ieee80211_key *key)
{
	if (!key)
		return;

	DEBUGFS_DEL(keylen);
	DEBUGFS_DEL(flags);
	DEBUGFS_DEL(keyidx);
	DEBUGFS_DEL(hw_key_idx);
	DEBUGFS_DEL(tx_rx_count);
	DEBUGFS_DEL(algorithm);
	DEBUGFS_DEL(tx_spec);
	DEBUGFS_DEL(rx_spec);
	DEBUGFS_DEL(replays);
	DEBUGFS_DEL(key);
	DEBUGFS_DEL(ifindex);

	debugfs_remove(key->debugfs.stalink);
	key->debugfs.stalink = NULL;
	debugfs_remove(key->debugfs.dir);
	key->debugfs.dir = NULL;
}
void ieee80211_debugfs_key_add_default(struct ieee80211_sub_if_data *sdata)
{
	char buf[50];

	if (!sdata->debugfsdir)
		return;

	sprintf(buf, "../keys/%d", sdata->default_key->debugfs.cnt);
	sdata->debugfs.default_key =
		debugfs_create_symlink("default_key", sdata->debugfsdir, buf);
}
void ieee80211_debugfs_key_remove_default(struct ieee80211_sub_if_data *sdata)
{
	if (!sdata)
		return;

	debugfs_remove(sdata->debugfs.default_key);
	sdata->debugfs.default_key = NULL;
}

void ieee80211_debugfs_key_sta_del(struct ieee80211_key *key,
				   struct sta_info *sta)
{
	debugfs_remove(key->debugfs.stalink);
	key->debugfs.stalink = NULL;
}
