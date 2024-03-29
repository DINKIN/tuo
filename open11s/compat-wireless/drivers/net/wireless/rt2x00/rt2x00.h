/*
	Copyright (C) 2004 - 2008 rt2x00 SourceForge Project
	<http://rt2x00.serialmonkey.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the
	Free Software Foundation, Inc.,
	59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
	Module: rt2x00
	Abstract: rt2x00 global information.
 */

#ifndef RT2X00_H
#define RT2X00_H

#include <linux/bitops.h>
#include <linux/skbuff.h>
#include <linux/workqueue.h>
#include <linux/firmware.h>
#include <linux/leds.h>
#include <linux/mutex.h>
#include <linux/etherdevice.h>

#include <net/mac80211.h>

#include "rt2x00debug.h"
#include "rt2x00leds.h"
#include "rt2x00reg.h"
#include "rt2x00queue.h"

/*
 * Module information.
 */
#define DRV_VERSION	"2.1.4"
#define DRV_PROJECT	"http://rt2x00.serialmonkey.com"

/*
 * Debug definitions.
 * Debug output has to be enabled during compile time.
 */
#define DEBUG_PRINTK_MSG(__dev, __kernlvl, __lvl, __msg, __args...)	\
	printk(__kernlvl "%s -> %s: %s - " __msg,			\
	       wiphy_name((__dev)->hw->wiphy), __FUNCTION__, __lvl, ##__args)

#define DEBUG_PRINTK_PROBE(__kernlvl, __lvl, __msg, __args...)	\
	printk(__kernlvl "%s -> %s: %s - " __msg,		\
	       KBUILD_MODNAME, __FUNCTION__, __lvl, ##__args)

#ifdef CONFIG_RT2X00_DEBUG
#define DEBUG_PRINTK(__dev, __kernlvl, __lvl, __msg, __args...)	\
	DEBUG_PRINTK_MSG(__dev, __kernlvl, __lvl, __msg, ##__args);
#else
#define DEBUG_PRINTK(__dev, __kernlvl, __lvl, __msg, __args...)	\
	do { } while (0)
#endif /* CONFIG_RT2X00_DEBUG */

/*
 * Various debug levels.
 * The debug levels PANIC and ERROR both indicate serious problems,
 * for this reason they should never be ignored.
 * The special ERROR_PROBE message is for messages that are generated
 * when the rt2x00_dev is not yet initialized.
 */
#define PANIC(__dev, __msg, __args...) \
	DEBUG_PRINTK_MSG(__dev, KERN_CRIT, "Panic", __msg, ##__args)
#define ERROR(__dev, __msg, __args...)	\
	DEBUG_PRINTK_MSG(__dev, KERN_ERR, "Error", __msg, ##__args)
#define ERROR_PROBE(__msg, __args...) \
	DEBUG_PRINTK_PROBE(KERN_ERR, "Error", __msg, ##__args)
#define WARNING(__dev, __msg, __args...) \
	DEBUG_PRINTK(__dev, KERN_WARNING, "Warning", __msg, ##__args)
#define NOTICE(__dev, __msg, __args...) \
	DEBUG_PRINTK(__dev, KERN_NOTICE, "Notice", __msg, ##__args)
#define INFO(__dev, __msg, __args...) \
	DEBUG_PRINTK(__dev, KERN_INFO, "Info", __msg, ##__args)
#define DEBUG(__dev, __msg, __args...) \
	DEBUG_PRINTK(__dev, KERN_DEBUG, "Debug", __msg, ##__args)
#define EEPROM(__dev, __msg, __args...) \
	DEBUG_PRINTK(__dev, KERN_DEBUG, "EEPROM recovery", __msg, ##__args)

/*
 * Standard timing and size defines.
 * These values should follow the ieee80211 specifications.
 */
#define ACK_SIZE		14
#define IEEE80211_HEADER	24
#define PLCP			48
#define BEACON			100
#define PREAMBLE		144
#define SHORT_PREAMBLE		72
#define SLOT_TIME		20
#define SHORT_SLOT_TIME		9
#define SIFS			10
#define PIFS			( SIFS + SLOT_TIME )
#define SHORT_PIFS		( SIFS + SHORT_SLOT_TIME )
#define DIFS			( PIFS + SLOT_TIME )
#define SHORT_DIFS		( SHORT_PIFS + SHORT_SLOT_TIME )
#define EIFS			( SIFS + (8 * (IEEE80211_HEADER + ACK_SIZE)) )

/*
 * IEEE802.11 header defines
 */
static inline int is_rts_frame(u16 fc)
{
	return (((fc & IEEE80211_FCTL_FTYPE) == IEEE80211_FTYPE_CTL) &&
		((fc & IEEE80211_FCTL_STYPE) == IEEE80211_STYPE_RTS));
}

static inline int is_cts_frame(u16 fc)
{
	return (((fc & IEEE80211_FCTL_FTYPE) == IEEE80211_FTYPE_CTL) &&
		((fc & IEEE80211_FCTL_STYPE) == IEEE80211_STYPE_CTS));
}

static inline int is_probe_resp(u16 fc)
{
	return (((fc & IEEE80211_FCTL_FTYPE) == IEEE80211_FTYPE_MGMT) &&
		((fc & IEEE80211_FCTL_STYPE) == IEEE80211_STYPE_PROBE_RESP));
}

static inline int is_beacon(u16 fc)
{
	return (((fc & IEEE80211_FCTL_FTYPE) == IEEE80211_FTYPE_MGMT) &&
		((fc & IEEE80211_FCTL_STYPE) == IEEE80211_STYPE_BEACON));
}

/*
 * Chipset identification
 * The chipset on the device is composed of a RT and RF chip.
 * The chipset combination is important for determining device capabilities.
 */
struct rt2x00_chip {
	u16 rt;
#define RT2460		0x0101
#define RT2560		0x0201
#define RT2570		0x1201
#define RT2561s		0x0301	/* Turbo */
#define RT2561		0x0302
#define RT2661		0x0401
#define RT2571		0x1300

	u16 rf;
	u32 rev;
};

/*
 * RF register values that belong to a particular channel.
 */
struct rf_channel {
	int channel;
	u32 rf1;
	u32 rf2;
	u32 rf3;
	u32 rf4;
};

/*
 * Antenna setup values.
 */
struct antenna_setup {
	enum antenna rx;
	enum antenna tx;
};

/*
 * Quality statistics about the currently active link.
 */
struct link_qual {
	/*
	 * Statistics required for Link tuning.
	 * For the average RSSI value we use the "Walking average" approach.
	 * When adding RSSI to the average value the following calculation
	 * is needed:
	 *
	 *        avg_rssi = ((avg_rssi * 7) + rssi) / 8;
	 *
	 * The advantage of this approach is that we only need 1 variable
	 * to store the average in (No need for a count and a total).
	 * But more importantly, normal average values will over time
	 * move less and less towards newly added values this results
	 * that with link tuning, the device can have a very good RSSI
	 * for a few minutes but when the device is moved away from the AP
	 * the average will not decrease fast enough to compensate.
	 * The walking average compensates this and will move towards
	 * the new values correctly allowing a effective link tuning.
	 */
	int avg_rssi;
	int false_cca;

	/*
	 * Statistics required for Signal quality calculation.
	 * For calculating the Signal quality we have to determine
	 * the total number of success and failed RX and TX frames.
	 * After that we also use the average RSSI value to help
	 * determining the signal quality.
	 * For the calculation we will use the following algorithm:
	 *
	 *         rssi_percentage = (avg_rssi * 100) / rssi_offset
	 *         rx_percentage = (rx_success * 100) / rx_total
	 *         tx_percentage = (tx_success * 100) / tx_total
	 *         avg_signal = ((WEIGHT_RSSI * avg_rssi) +
	 *                       (WEIGHT_TX * tx_percentage) +
	 *                       (WEIGHT_RX * rx_percentage)) / 100
	 *
	 * This value should then be checked to not be greated then 100.
	 */
	int rx_percentage;
	int rx_success;
	int rx_failed;
	int tx_percentage;
	int tx_success;
	int tx_failed;
#define WEIGHT_RSSI	20
#define WEIGHT_RX	40
#define WEIGHT_TX	40
};

/*
 * Antenna settings about the currently active link.
 */
struct link_ant {
	/*
	 * Antenna flags
	 */
	unsigned int flags;
#define ANTENNA_RX_DIVERSITY	0x00000001
#define ANTENNA_TX_DIVERSITY	0x00000002
#define ANTENNA_MODE_SAMPLE	0x00000004

	/*
	 * Currently active TX/RX antenna setup.
	 * When software diversity is used, this will indicate
	 * which antenna is actually used at this time.
	 */
	struct antenna_setup active;

	/*
	 * RSSI information for the different antenna's.
	 * These statistics are used to determine when
	 * to switch antenna when using software diversity.
	 *
	 *        rssi[0] -> Antenna A RSSI
	 *        rssi[1] -> Antenna B RSSI
	 */
	int rssi_history[2];

	/*
	 * Current RSSI average of the currently active antenna.
	 * Similar to the avg_rssi in the link_qual structure
	 * this value is updated by using the walking average.
	 */
	int rssi_ant;
};

/*
 * To optimize the quality of the link we need to store
 * the quality of received frames and periodically
 * optimize the link.
 */
struct link {
	/*
	 * Link tuner counter
	 * The number of times the link has been tuned
	 * since the radio has been switched on.
	 */
	u32 count;

	/*
	 * Quality measurement values.
	 */
	struct link_qual qual;

	/*
	 * TX/RX antenna setup.
	 */
	struct link_ant ant;

	/*
	 * Active VGC level
	 */
	int vgc_level;

	/*
	 * Work structure for scheduling periodic link tuning.
	 */
	struct delayed_work work;
};

/*
 * Small helper macro to work with moving/walking averages.
 */
#define MOVING_AVERAGE(__avg, __val, __samples) \
	( (((__avg) * ((__samples) - 1)) + (__val)) / (__samples) )

/*
 * When we lack RSSI information return something less then -80 to
 * tell the driver to tune the device to maximum sensitivity.
 */
#define DEFAULT_RSSI	( -128 )

/*
 * Link quality access functions.
 */
static inline int rt2x00_get_link_rssi(struct link *link)
{
	if (link->qual.avg_rssi && link->qual.rx_success)
		return link->qual.avg_rssi;
	return DEFAULT_RSSI;
}

static inline int rt2x00_get_link_ant_rssi(struct link *link)
{
	if (link->ant.rssi_ant && link->qual.rx_success)
		return link->ant.rssi_ant;
	return DEFAULT_RSSI;
}

static inline int rt2x00_get_link_ant_rssi_history(struct link *link,
						   enum antenna ant)
{
	if (link->ant.rssi_history[ant - ANTENNA_A])
		return link->ant.rssi_history[ant - ANTENNA_A];
	return DEFAULT_RSSI;
}

static inline int rt2x00_update_ant_rssi(struct link *link, int rssi)
{
	int old_rssi = link->ant.rssi_history[link->ant.active.rx - ANTENNA_A];
	link->ant.rssi_history[link->ant.active.rx - ANTENNA_A] = rssi;
	return old_rssi;
}

/*
 * Interface structure
 * Per interface configuration details, this structure
 * is allocated as the private data for ieee80211_vif.
 */
struct rt2x00_intf {
	/*
	 * All fields within the rt2x00_intf structure
	 * must be protected with a spinlock.
	 */
	spinlock_t lock;

	/*
	 * BSS configuration. Copied from the structure
	 * passed to us through the bss_info_changed()
	 * callback funtion.
	 */
	struct ieee80211_bss_conf conf;

	/*
	 * MAC of the device.
	 */
	u8 mac[ETH_ALEN];

	/*
	 * BBSID of the AP to associate with.
	 */
	u8 bssid[ETH_ALEN];

	/*
	 * Entry in the beacon queue which belongs to
	 * this interface. Each interface has its own
	 * dedicated beacon entry.
	 */
	struct queue_entry *beacon;

	/*
	 * Actions that needed rescheduling.
	 */
	unsigned int delayed_flags;
#define DELAYED_UPDATE_BEACON		0x00000001
#define DELAYED_CONFIG_ERP		0x00000002
#define DELAYED_LED_ASSOC		0x00000004
};

static inline struct rt2x00_intf* vif_to_intf(struct ieee80211_vif *vif)
{
	return (struct rt2x00_intf *)vif->drv_priv;
}

/**
 * struct hw_mode_spec: Hardware specifications structure
 *
 * Details about the supported modes, rates and channels
 * of a particular chipset. This is used by rt2x00lib
 * to build the ieee80211_hw_mode array for mac80211.
 *
 * @supported_bands: Bitmask contained the supported bands (2.4GHz, 5.2GHz).
 * @supported_rates: Rate types which are supported (CCK, OFDM).
 * @num_channels: Number of supported channels. This is used as array size
 *	for @tx_power_a, @tx_power_bg and @channels.
 * channels: Device/chipset specific channel values (See &struct rf_channel).
 * @tx_power_a: TX power values for all 5.2GHz channels (may be NULL).
 * @tx_power_bg: TX power values for all 2.4GHz channels (may be NULL).
 * @tx_power_default: Default TX power value to use when either
 *	@tx_power_a or @tx_power_bg is missing.
 */
struct hw_mode_spec {
	unsigned int supported_bands;
#define SUPPORT_BAND_2GHZ	0x00000001
#define SUPPORT_BAND_5GHZ	0x00000002

	unsigned int supported_rates;
#define SUPPORT_RATE_CCK	0x00000001
#define SUPPORT_RATE_OFDM	0x00000002

	unsigned int num_channels;
	const struct rf_channel *channels;

	const u8 *tx_power_a;
	const u8 *tx_power_bg;
	u8 tx_power_default;
};

/*
 * Configuration structure wrapper around the
 * mac80211 configuration structure.
 * When mac80211 configures the driver, rt2x00lib
 * can precalculate values which are equal for all
 * rt2x00 drivers. Those values can be stored in here.
 */
struct rt2x00lib_conf {
	struct ieee80211_conf *conf;
	struct rf_channel rf;

	struct antenna_setup ant;

	enum ieee80211_band band;

	u32 basic_rates;
	u32 slot_time;

	short sifs;
	short pifs;
	short difs;
	short eifs;
};

/*
 * Configuration structure for erp settings.
 */
struct rt2x00lib_erp {
	int short_preamble;

	int ack_timeout;
	int ack_consume_time;
};

/*
 * Configuration structure wrapper around the
 * rt2x00 interface configuration handler.
 */
struct rt2x00intf_conf {
	/*
	 * Interface type
	 */
	enum ieee80211_if_types type;

	/*
	 * TSF sync value, this is dependant on the operation type.
	 */
	enum tsf_sync sync;

	/*
	 * The MAC and BSSID addressess are simple array of bytes,
	 * these arrays are little endian, so when sending the addressess
	 * to the drivers, copy the it into a endian-signed variable.
	 *
	 * Note that all devices (except rt2500usb) have 32 bits
	 * register word sizes. This means that whatever variable we
	 * pass _must_ be a multiple of 32 bits. Otherwise the device
	 * might not accept what we are sending to it.
	 * This will also make it easier for the driver to write
	 * the data to the device.
	 */
	__le32 mac[2];
	__le32 bssid[2];
};

/*
 * rt2x00lib callback functions.
 */
struct rt2x00lib_ops {
	/*
	 * Interrupt handlers.
	 */
	irq_handler_t irq_handler;

	/*
	 * Device init handlers.
	 */
	int (*probe_hw) (struct rt2x00_dev *rt2x00dev);
	char *(*get_firmware_name) (struct rt2x00_dev *rt2x00dev);
	u16 (*get_firmware_crc) (void *data, const size_t len);
	int (*load_firmware) (struct rt2x00_dev *rt2x00dev, void *data,
			      const size_t len);

	/*
	 * Device initialization/deinitialization handlers.
	 */
	int (*initialize) (struct rt2x00_dev *rt2x00dev);
	void (*uninitialize) (struct rt2x00_dev *rt2x00dev);

	/*
	 * queue initialization handlers
	 */
	void (*init_rxentry) (struct rt2x00_dev *rt2x00dev,
			      struct queue_entry *entry);
	void (*init_txentry) (struct rt2x00_dev *rt2x00dev,
			      struct queue_entry *entry);

	/*
	 * Radio control handlers.
	 */
	int (*set_device_state) (struct rt2x00_dev *rt2x00dev,
				 enum dev_state state);
	int (*rfkill_poll) (struct rt2x00_dev *rt2x00dev);
	void (*link_stats) (struct rt2x00_dev *rt2x00dev,
			    struct link_qual *qual);
	void (*reset_tuner) (struct rt2x00_dev *rt2x00dev);
	void (*link_tuner) (struct rt2x00_dev *rt2x00dev);

	/*
	 * TX control handlers
	 */
	void (*write_tx_desc) (struct rt2x00_dev *rt2x00dev,
			       struct sk_buff *skb,
			       struct txentry_desc *txdesc,
			       struct ieee80211_tx_control *control);
	int (*write_tx_data) (struct rt2x00_dev *rt2x00dev,
			      struct data_queue *queue, struct sk_buff *skb,
			      struct ieee80211_tx_control *control);
	int (*get_tx_data_len) (struct rt2x00_dev *rt2x00dev,
				struct sk_buff *skb);
	void (*kick_tx_queue) (struct rt2x00_dev *rt2x00dev,
			       const unsigned int queue);

	/*
	 * RX control handlers
	 */
	void (*fill_rxdone) (struct queue_entry *entry,
			     struct rxdone_entry_desc *rxdesc);

	/*
	 * Configuration handlers.
	 */
	void (*config_filter) (struct rt2x00_dev *rt2x00dev,
			       const unsigned int filter_flags);
	void (*config_intf) (struct rt2x00_dev *rt2x00dev,
			     struct rt2x00_intf *intf,
			     struct rt2x00intf_conf *conf,
			     const unsigned int flags);
#define CONFIG_UPDATE_TYPE		( 1 << 1 )
#define CONFIG_UPDATE_MAC		( 1 << 2 )
#define CONFIG_UPDATE_BSSID		( 1 << 3 )

	void (*config_erp) (struct rt2x00_dev *rt2x00dev,
			    struct rt2x00lib_erp *erp);
	void (*config) (struct rt2x00_dev *rt2x00dev,
			struct rt2x00lib_conf *libconf,
			const unsigned int flags);
#define CONFIG_UPDATE_PHYMODE		( 1 << 1 )
#define CONFIG_UPDATE_CHANNEL		( 1 << 2 )
#define CONFIG_UPDATE_TXPOWER		( 1 << 3 )
#define CONFIG_UPDATE_ANTENNA		( 1 << 4 )
#define CONFIG_UPDATE_SLOT_TIME 	( 1 << 5 )
#define CONFIG_UPDATE_BEACON_INT	( 1 << 6 )
#define CONFIG_UPDATE_ALL		0xffff
};

/*
 * rt2x00 driver callback operation structure.
 */
struct rt2x00_ops {
	const char *name;
	const unsigned int max_sta_intf;
	const unsigned int max_ap_intf;
	const unsigned int eeprom_size;
	const unsigned int rf_size;
	const struct data_queue_desc *rx;
	const struct data_queue_desc *tx;
	const struct data_queue_desc *bcn;
	const struct data_queue_desc *atim;
	const struct rt2x00lib_ops *lib;
	const struct ieee80211_ops *hw;
#ifdef CONFIG_RT2X00_LIB_DEBUGFS
	const struct rt2x00debug *debugfs;
#endif /* CONFIG_RT2X00_LIB_DEBUGFS */
};

/*
 * rt2x00 device flags
 */
enum rt2x00_flags {
	/*
	 * Device state flags
	 */
	DEVICE_PRESENT,
	DEVICE_REGISTERED_HW,
	DEVICE_INITIALIZED,
	DEVICE_STARTED,
	DEVICE_STARTED_SUSPEND,
	DEVICE_ENABLED_RADIO,
	DEVICE_DISABLED_RADIO_HW,

	/*
	 * Driver features
	 */
	DRIVER_SUPPORT_MIXED_INTERFACES,
	DRIVER_REQUIRE_FIRMWARE,
	DRIVER_REQUIRE_BEACON_GUARD,
	DRIVER_REQUIRE_ATIM_QUEUE,
	DRIVER_REQUIRE_SCHEDULED,

	/*
	 * Driver configuration
	 */
	CONFIG_SUPPORT_HW_BUTTON,
	CONFIG_FRAME_TYPE,
	CONFIG_RF_SEQUENCE,
	CONFIG_EXTERNAL_LNA_A,
	CONFIG_EXTERNAL_LNA_BG,
	CONFIG_DOUBLE_ANTENNA,
	CONFIG_DISABLE_LINK_TUNING,
};

/*
 * rt2x00 device structure.
 */
struct rt2x00_dev {
	/*
	 * Device structure.
	 * The structure stored in here depends on the
	 * system bus (PCI or USB).
	 * When accessing this variable, the rt2x00dev_{pci,usb}
	 * macro's should be used for correct typecasting.
	 */
	void *dev;
#define rt2x00dev_pci(__dev)	( (struct pci_dev *)(__dev)->dev )
#define rt2x00dev_usb(__dev)	( (struct usb_interface *)(__dev)->dev )
#define rt2x00dev_usb_dev(__dev)\
	( (struct usb_device *)interface_to_usbdev(rt2x00dev_usb(__dev)) )

	/*
	 * Callback functions.
	 */
	const struct rt2x00_ops *ops;

	/*
	 * IEEE80211 control structure.
	 */
	struct ieee80211_hw *hw;
	struct ieee80211_supported_band bands[IEEE80211_NUM_BANDS];
	enum ieee80211_band curr_band;

	/*
	 * rfkill structure for RF state switching support.
	 * This will only be compiled in when required.
	 */
#ifdef CONFIG_RT2X00_LIB_RFKILL
	unsigned long rfkill_state;
#define RFKILL_STATE_ALLOCATED		1
#define RFKILL_STATE_REGISTERED		2
	struct rfkill *rfkill;
	struct input_polled_dev *poll_dev;
#endif /* CONFIG_RT2X00_LIB_RFKILL */

	/*
	 * If enabled, the debugfs interface structures
	 * required for deregistration of debugfs.
	 */
#ifdef CONFIG_RT2X00_LIB_DEBUGFS
	struct rt2x00debug_intf *debugfs_intf;
#endif /* CONFIG_RT2X00_LIB_DEBUGFS */

	/*
	 * LED structure for changing the LED status
	 * by mac8011 or the kernel.
	 */
#ifdef CONFIG_RT2X00_LIB_LEDS
	struct rt2x00_led led_radio;
	struct rt2x00_led led_assoc;
	struct rt2x00_led led_qual;
	u16 led_mcu_reg;
#endif /* CONFIG_RT2X00_LIB_LEDS */

	/*
	 * Device flags.
	 * In these flags the current status and some
	 * of the device capabilities are stored.
	 */
	unsigned long flags;

	/*
	 * Chipset identification.
	 */
	struct rt2x00_chip chip;

	/*
	 * hw capability specifications.
	 */
	struct hw_mode_spec spec;

	/*
	 * This is the default TX/RX antenna setup as indicated
	 * by the device's EEPROM. When mac80211 sets its
	 * antenna value to 0 we should be using these values.
	 */
	struct antenna_setup default_ant;

	/*
	 * Register pointers
	 * csr.base: CSR base register address. (PCI)
	 * csr.cache: CSR cache for usb_control_msg. (USB)
	 */
	union csr {
		void __iomem *base;
		void *cache;
	} csr;

	/*
	 * Mutex to protect register accesses on USB devices.
	 * There are 2 reasons this is needed, one is to ensure
	 * use of the csr_cache (for USB devices) by one thread
	 * isn't corrupted by another thread trying to access it.
	 * The other is that access to BBP and RF registers
	 * require multiple BUS transactions and if another thread
	 * attempted to access one of those registers at the same
	 * time one of the writes could silently fail.
	 */
	struct mutex usb_cache_mutex;

	/*
	 * Current packet filter configuration for the device.
	 * This contains all currently active FIF_* flags send
	 * to us by mac80211 during configure_filter().
	 */
	unsigned int packet_filter;

	/*
	 * Interface details:
	 *  - Open ap interface count.
	 *  - Open sta interface count.
	 *  - Association count.
	 */
	unsigned int intf_ap_count;
	unsigned int intf_sta_count;
	unsigned int intf_associated;

	/*
	 * Link quality
	 */
	struct link link;

	/*
	 * EEPROM data.
	 */
	__le16 *eeprom;

	/*
	 * Active RF register values.
	 * These are stored here so we don't need
	 * to read the rf registers and can directly
	 * use this value instead.
	 * This field should be accessed by using
	 * rt2x00_rf_read() and rt2x00_rf_write().
	 */
	u32 *rf;

	/*
	 * USB Max frame size (for rt2500usb & rt73usb).
	 */
	u16 usb_maxpacket;

	/*
	 * Current TX power value.
	 */
	u16 tx_power;

	/*
	 * Rssi <-> Dbm offset
	 */
	u8 rssi_offset;

	/*
	 * Frequency offset (for rt61pci & rt73usb).
	 */
	u8 freq_offset;

	/*
	 * Low level statistics which will have
	 * to be kept up to date while device is running.
	 */
	struct ieee80211_low_level_stats low_level_stats;

	/*
	 * RX configuration information.
	 */
	struct ieee80211_rx_status rx_status;

	/*
	 * Scheduled work.
	 */
	struct work_struct intf_work;
	struct work_struct filter_work;

	/*
	 * Data queue arrays for RX, TX and Beacon.
	 * The Beacon array also contains the Atim queue
	 * if that is supported by the device.
	 */
	int data_queues;
	struct data_queue *rx;
	struct data_queue *tx;
	struct data_queue *bcn;

	/*
	 * Firmware image.
	 */
	const struct firmware *fw;
};

/*
 * Generic RF access.
 * The RF is being accessed by word index.
 */
static inline void rt2x00_rf_read(struct rt2x00_dev *rt2x00dev,
				  const unsigned int word, u32 *data)
{
	*data = rt2x00dev->rf[word];
}

static inline void rt2x00_rf_write(struct rt2x00_dev *rt2x00dev,
				   const unsigned int word, u32 data)
{
	rt2x00dev->rf[word] = data;
}

/*
 *  Generic EEPROM access.
 * The EEPROM is being accessed by word index.
 */
static inline void *rt2x00_eeprom_addr(struct rt2x00_dev *rt2x00dev,
				       const unsigned int word)
{
	return (void *)&rt2x00dev->eeprom[word];
}

static inline void rt2x00_eeprom_read(struct rt2x00_dev *rt2x00dev,
				      const unsigned int word, u16 *data)
{
	*data = le16_to_cpu(rt2x00dev->eeprom[word]);
}

static inline void rt2x00_eeprom_write(struct rt2x00_dev *rt2x00dev,
				       const unsigned int word, u16 data)
{
	rt2x00dev->eeprom[word] = cpu_to_le16(data);
}

/*
 * Chipset handlers
 */
static inline void rt2x00_set_chip(struct rt2x00_dev *rt2x00dev,
				   const u16 rt, const u16 rf, const u32 rev)
{
	INFO(rt2x00dev,
	     "Chipset detected - rt: %04x, rf: %04x, rev: %08x.\n",
	     rt, rf, rev);

	rt2x00dev->chip.rt = rt;
	rt2x00dev->chip.rf = rf;
	rt2x00dev->chip.rev = rev;
}

static inline char rt2x00_rt(const struct rt2x00_chip *chipset, const u16 chip)
{
	return (chipset->rt == chip);
}

static inline char rt2x00_rf(const struct rt2x00_chip *chipset, const u16 chip)
{
	return (chipset->rf == chip);
}

static inline u16 rt2x00_rev(const struct rt2x00_chip *chipset)
{
	return chipset->rev;
}

static inline u16 rt2x00_check_rev(const struct rt2x00_chip *chipset,
				   const u32 rev)
{
	return (((chipset->rev & 0xffff0) == rev) &&
		!!(chipset->rev & 0x0000f));
}

/*
 * Duration calculations
 * The rate variable passed is: 100kbs.
 * To convert from bytes to bits we multiply size with 8,
 * then the size is multiplied with 10 to make the
 * real rate -> rate argument correction.
 */
static inline u16 get_duration(const unsigned int size, const u8 rate)
{
	return ((size * 8 * 10) / rate);
}

static inline u16 get_duration_res(const unsigned int size, const u8 rate)
{
	return ((size * 8 * 10) % rate);
}

/**
 * rt2x00queue_get_queue - Convert mac80211 queue index to rt2x00 queue
 * @rt2x00dev: Pointer to &struct rt2x00_dev.
 * @queue: mac80211/rt2x00 queue index
 *	(see &enum ieee80211_tx_queue and &enum rt2x00_bcn_queue).
 */
struct data_queue *rt2x00queue_get_queue(struct rt2x00_dev *rt2x00dev,
					 const unsigned int queue);

/**
 * rt2x00queue_get_entry - Get queue entry where the given index points to.
 * @rt2x00dev: Pointer to &struct rt2x00_dev.
 * @index: Index identifier for obtaining the correct index.
 */
struct queue_entry *rt2x00queue_get_entry(struct data_queue *queue,
					  enum queue_index index);

/**
 * rt2x00queue_index_inc - Index incrementation function
 * @queue: Queue (&struct data_queue) to perform the action on.
 * @action: Index type (&enum queue_index) to perform the action on.
 *
 * This function will increase the requested index on the queue,
 * it will grab the appropriate locks and handle queue overflow events by
 * resetting the index to the start of the queue.
 */
void rt2x00queue_index_inc(struct data_queue *queue, enum queue_index index);


/*
 * Interrupt context handlers.
 */
void rt2x00lib_beacondone(struct rt2x00_dev *rt2x00dev);
void rt2x00lib_txdone(struct queue_entry *entry,
		      struct txdone_entry_desc *txdesc);
void rt2x00lib_rxdone(struct queue_entry *entry,
		      struct rxdone_entry_desc *rxdesc);

/*
 * TX descriptor initializer
 */
void rt2x00lib_write_tx_desc(struct rt2x00_dev *rt2x00dev,
			     struct sk_buff *skb,
			     struct ieee80211_tx_control *control);

/*
 * mac80211 handlers.
 */
int rt2x00mac_tx(struct ieee80211_hw *hw, struct sk_buff *skb,
		 struct ieee80211_tx_control *control);
int rt2x00mac_start(struct ieee80211_hw *hw);
void rt2x00mac_stop(struct ieee80211_hw *hw);
int rt2x00mac_add_interface(struct ieee80211_hw *hw,
			    struct ieee80211_if_init_conf *conf);
void rt2x00mac_remove_interface(struct ieee80211_hw *hw,
				struct ieee80211_if_init_conf *conf);
int rt2x00mac_config(struct ieee80211_hw *hw, struct ieee80211_conf *conf);
int rt2x00mac_config_interface(struct ieee80211_hw *hw,
			       struct ieee80211_vif *vif,
			       struct ieee80211_if_conf *conf);
void rt2x00mac_configure_filter(struct ieee80211_hw *hw,
				unsigned int changed_flags,
				unsigned int *total_flags,
				int mc_count, struct dev_addr_list *mc_list);
int rt2x00mac_get_stats(struct ieee80211_hw *hw,
			struct ieee80211_low_level_stats *stats);
int rt2x00mac_get_tx_stats(struct ieee80211_hw *hw,
			   struct ieee80211_tx_queue_stats *stats);
void rt2x00mac_bss_info_changed(struct ieee80211_hw *hw,
				struct ieee80211_vif *vif,
				struct ieee80211_bss_conf *bss_conf,
				u32 changes);
int rt2x00mac_conf_tx(struct ieee80211_hw *hw, int queue,
		      const struct ieee80211_tx_queue_params *params);

/*
 * Driver allocation handlers.
 */
int rt2x00lib_probe_dev(struct rt2x00_dev *rt2x00dev);
void rt2x00lib_remove_dev(struct rt2x00_dev *rt2x00dev);
#ifdef CONFIG_PM
int rt2x00lib_suspend(struct rt2x00_dev *rt2x00dev, pm_message_t state);
int rt2x00lib_resume(struct rt2x00_dev *rt2x00dev);
#endif /* CONFIG_PM */

#endif /* RT2X00_H */
