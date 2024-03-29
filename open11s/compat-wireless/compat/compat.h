#ifndef LINUX_26_COMPAT_H
#define LINUX_26_COMPAT_H

#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/netpoll.h>
#include <linux/rtnetlink.h>
#include <linux/audit.h>
#include <linux/workqueue.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/genetlink.h>
#include <linux/scatterlist.h>
#include <linux/usb.h>
#include <linux/hw_random.h>
#include <linux/leds.h>
#include <linux/pm_qos_params.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/uaccess.h>

#include <net/arp.h>
#include <net/neighbour.h>

#include <linux/compat_autoconf.h>

#include <asm/io.h>

/* Compat work for 2.6.21 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))

/* reuse ax25_ptr */
#define ieee80211_ptr ax25_ptr

#ifdef CONFIG_AX25
#error Compat reuses the AX.25 pointer so that may not be enabled!
#endif

static inline unsigned char *skb_mac_header(const struct sk_buff *skb)
{
	return skb->mac.raw;
}

static inline void skb_set_mac_header(struct sk_buff *skb, int offset)
{
	skb->mac.raw = skb->data + offset;
}

static inline void skb_reset_mac_header(struct sk_buff *skb)
{
	skb->mac.raw = skb->data;
}

static inline void skb_reset_network_header(struct sk_buff *skb)
{
	skb->nh.raw = skb->data;
}

static inline void skb_set_network_header(struct sk_buff *skb, int offset)
{
	skb->nh.raw = skb->data + offset;
}

static inline void skb_set_transport_header(struct sk_buff *skb, int offset)
{
	skb->h.raw = skb->data + offset;
}

static inline unsigned char *skb_transport_header(struct sk_buff *skb)
{
	return skb->h.raw;
}

static inline unsigned char *skb_network_header(const struct sk_buff *skb)
{
	return skb->nh.raw;
}

static inline unsigned char *skb_tail_pointer(const struct sk_buff *skb)
{
	return skb->tail;
}

static inline struct iphdr *ip_hdr(const struct sk_buff *skb)
{
	return (struct iphdr *)skb_network_header(skb);
}

static inline void skb_copy_from_linear_data(const struct sk_buff *skb,
					     void *to,
					     const unsigned int len)
{
	memcpy(to, skb->data, len);
}

static inline void skb_copy_from_linear_data_offset(const struct sk_buff *skb,
						    const int offset, void *to,
						    const unsigned int len)
{
	memcpy(to, skb->data + offset, len);
}

#define __maybe_unused	__attribute__((unused))

#define uninitialized_var(x) x = x

/* This will lead to very weird behaviour... */
#define NLA_BINARY NLA_STRING

static inline int pci_set_mwi(struct pci_dev *dev)
{
	return -ENOSYS;
}

static inline void pci_clear_mwi(struct pci_dev *dev)
{
}

#define list_first_entry(ptr, type, member) \
        list_entry((ptr)->next, type, member)

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)) */

/* Compat work for 2.6.21, 2.6.22 and 2.6.23 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))

/*
 * Force link bug if constructor is used, can't be done compatibly
 * because constructor arguments were swapped since then!
 */
extern void __incompatible_kmem_cache_create(void);

/* 2.6.21 and 2.6.22 kmem_cache_create() takes 6 arguments */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23))
#define kmem_cache_create(name, objsize, align, flags, ctor) 	\
	({							\
		if (ctor) __incompatible_kmem_cache_create();	\
		kmem_cache_create((name), (objsize), (align),	\
				  (flags), NULL, NULL);		\
	})
#endif

/* 2.6.23 kmem_cache_create() takes 5 arguments */
#if (LINUX_VERSION_CODE == KERNEL_VERSION(2,6,23))
#define kmem_cache_create(name, objsize, align, flags, ctor) 	\
	({							\
		if (ctor) __incompatible_kmem_cache_create();	\
		kmem_cache_create((name), (objsize), (align),	\
				  (flags), NULL);		\
	})
#endif

/* From include/linux/mod_devicetable.h */

/* SSB core, see drivers/ssb/ */
#ifndef SSB_DEVICE
struct ssb_device_id {
	__u16   vendor;
	__u16   coreid;
	__u8    revision;
};
#define SSB_DEVICE(_vendor, _coreid, _revision)  \
	{ .vendor = _vendor, .coreid = _coreid, .revision = _revision, }
#define SSB_DEVTABLE_END  \
	{ 0, },

#define SSB_ANY_VENDOR          0xFFFF
#define SSB_ANY_ID              0xFFFF
#define SSB_ANY_REV             0xFF
#endif


/* Namespace stuff, introduced on 2.6.24 */
#define dev_get_by_index(a, b)		dev_get_by_index(b)
#define __dev_get_by_index(a, b)	__dev_get_by_index(b)

/*
 * Display a 6 byte device address (MAC) in a readable format.
 */
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
extern char *print_mac(char *buf, const u8 *addr);
#define DECLARE_MAC_BUF(var) char var[18] __maybe_unused

extern int		eth_header(struct sk_buff *skb, struct net_device *dev,
				unsigned short type, void *daddr,
				void *saddr, unsigned len);
extern int		eth_rebuild_header(struct sk_buff *skb);
extern void		eth_header_cache_update(struct hh_cache *hh, struct net_device *dev,
				unsigned char * haddr);
extern int		eth_header_cache(struct neighbour *neigh,
			struct hh_cache *hh);

/* This structure is simply not present on 2.6.22 and 2.6.23 */
struct header_ops {
	int     (*create) (struct sk_buff *skb, struct net_device *dev,
		unsigned short type, void *daddr,
		void *saddr, unsigned len);
	int     (*parse)(const struct sk_buff *skb, unsigned char *haddr);
	int     (*rebuild)(struct sk_buff *skb);
	#define HAVE_HEADER_CACHE
	int     (*cache)(struct neighbour *neigh, struct hh_cache *hh);
	void    (*cache_update)(struct hh_cache *hh,
		struct net_device *dev,
		unsigned char *haddr);
};

/* net/ieee80211/ieee80211_crypt_tkip uses sg_init_table. This was added on 
 * 2.6.24. CONFIG_DEBUG_SG was added in 2.6.24 as well, so lets just ignore
 * the debug stuff. Note that adding this required changes to the struct
 * scatterlist on include/asm/scatterlist*, so the right way to port this
 * is to simply ignore the new structure changes and zero the scatterlist
 * array. We lave the kdoc intact for reference.
 */

/**
 * sg_mark_end - Mark the end of the scatterlist
 * @sg:          SG entryScatterlist
 *
 * Description:
 *   Marks the passed in sg entry as the termination point for the sg
 *   table. A call to sg_next() on this entry will return NULL.
 *
 **/
static inline void sg_mark_end(struct scatterlist *sg)
{
}

/**
 * sg_init_table - Initialize SG table
 * @sgl:           The SG table
 * @nents:         Number of entries in table
 *
 * Notes:
 *   If this is part of a chained sg table, sg_mark_end() should be
 *   used only on the last table part.
 *
 **/
static inline void sg_init_table(struct scatterlist *sgl, unsigned int nents)
{
	memset(sgl, 0, sizeof(*sgl) * nents);
}

/**
 * usb_endpoint_num - get the endpoint's number
 * @epd: endpoint to be checked
 *
 * Returns @epd's number: 0 to 15.
 */
static inline int usb_endpoint_num(const struct usb_endpoint_descriptor *epd)
{
	return epd->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
}

/* Helper to make struct pci_dev is_pcie compatibility code smaller */
int compat_is_pcie(struct pci_dev *pdev);

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)) */

/* Compat work for kernels <= 2.6.22 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23))

/* dev_mc_list was replaced with dev_addr_list as of 2.6.23 */
#define dev_addr_list	dev_mc_list
#define da_addr		dmi_addr
#define da_addrlen	dmi_addrlen
#define da_users	dmi_users
#define da_gusers	dmi_gusers

/* dev_set_promiscuity() was moved to __dev_set_promiscuity() on 2.6.23 and 
 * dev_set_promiscuity() became a wrapper. */
#define __dev_set_promiscuity dev_set_promiscuity

/* Our own 2.6.22 port on compat.c */
extern void	dev_mc_unsync(struct net_device *to, struct net_device *from);
extern int	dev_mc_sync(struct net_device *to, struct net_device *from);

/* Our own 2.6.22 port on compat.c */
extern void	__dev_set_rx_mode(struct net_device *dev);

/* Simple to add this */
extern int cancel_delayed_work_sync(struct delayed_work *work);

#define cancel_delayed_work_sync cancel_rearming_delayed_work

#define debugfs_rename(a, b, c, d) 1

/* nl80211 requires multicast group support which is new and added on
 * 2.6.23. We can't add support for it for older kernels to support it
 * genl_family structure was changed. Lets just let through the
 * genl_register_mc_group call. This means no multicast group suppport */

#define genl_register_mc_group(a, b) 0

/**
 * struct genl_multicast_group - generic netlink multicast group
 * @name: name of the multicast group, names are per-family
 * @id: multicast group ID, assigned by the core, to use with
 * 	genlmsg_multicast().
 * @list: list entry for linking
 * @family: pointer to family, need not be set before registering
 */
struct genl_multicast_group
{
	struct genl_family      *family;        /* private */
	struct list_head        list;           /* private */
	char                    name[GENL_NAMSIZ];
	u32                     id;
};


/* Added as of 2.6.23 */
int pci_try_set_mwi(struct pci_dev *dev);

/* Added as of 2.6.23 */
#ifdef CONFIG_PM_SLEEP
/*
 * Tell the freezer that the current task should be frozen by it
 */
static inline void set_freezable(void)
{
	current->flags &= ~PF_NOFREEZE;
}

#else
static inline void set_freezable(void) {}
#endif /* CONFIG_PM_SLEEP */

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)) */

/* Compat work for 2.6.24 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))

/* The patch:
 * commit 8b5f6883683c91ad7e1af32b7ceeb604d68e2865
 * Author: Marcin Slusarz <marcin.slusarz@gmail.com>
 * Date:   Fri Feb 8 04:20:12 2008 -0800
 *
 *     byteorder: move le32_add_cpu & friends from OCFS2 to core
 *
 * moves le*_add_cpu and be*_add_cpu functions from OCFS2 to core
 * header (1st) and converted some existing code to it. We port
 * it here as later kernels will most likely use it.
 */
static inline void le16_add_cpu(__le16 *var, u16 val)
{
	*var = cpu_to_le16(le16_to_cpu(*var) + val);
}

static inline void le32_add_cpu(__le32 *var, u32 val)
{
	*var = cpu_to_le32(le32_to_cpu(*var) + val);
}

static inline void le64_add_cpu(__le64 *var, u64 val)
{
	*var = cpu_to_le64(le64_to_cpu(*var) + val);
}

static inline void be16_add_cpu(__be16 *var, u16 val)
{
	u16 v = be16_to_cpu(*var);
	*var = cpu_to_be16(v + val);
}

static inline void be32_add_cpu(__be32 *var, u32 val)
{
	u32 v = be32_to_cpu(*var);
	*var = cpu_to_be32(v + val);
}

static inline void be64_add_cpu(__be64 *var, u64 val)
{
	u64 v = be64_to_cpu(*var);
	*var = cpu_to_be64(v + val);
}

/* 2.6.25 changes hwrng_unregister()'s behaviour by supporting 
 * suspend of its parent device (the misc device, which is itself the
 * hardware random number generator). It does this by passing a parameter to
 * unregister_miscdev() which is not supported in older kernels. The suspend
 * parameter allows us to enable access to the device's hardware
 * number generator during suspend. As far as wireless is concerned this means
 * if a driver goes to suspend it you won't have the HNR available in
 * older kernels. */
static inline void __hwrng_unregister(struct hwrng *rng, bool suspended)
{
	hwrng_unregister(rng);
}

static inline void led_classdev_unregister_suspended(struct led_classdev *lcd)
{
	led_classdev_unregister(lcd);
}


#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)) */
#endif /* LINUX_26_COMPAT_H */
