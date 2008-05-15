#ifndef _MESH_H_
#define _MESH_H_

#include <linux/types.h>

typedef u8 bool;
#define false 0
#define true 1

/**
 * enum mesh_path_flags - mac80211 mesh path flags
 *
 *
 *
 * @MESH_PATH_ACTIVE: the mesh path is can be used for forwarding
 * @MESH_PATH_RESOLVED: the discovery process is running for this mesh path
 * @MESH_PATH_DSN_VALID: the mesh path contains a valid destination sequence
 * 	number
 * @MESH_PATH_FIXED: the mesh path has been manually set and should not be
 * 	modified
 * @MESH_PATH_RESOLVED: the mesh path can has been resolved
 *
 * MESH_PATH_RESOLVED and MESH_PATH_DELETE are used by the mesh path timer to
 * decide when to stop or cancel the mesh path discovery.
 */
enum mesh_path_flags {
	MESH_PATH_ACTIVE =	BIT(0),
	MESH_PATH_RESOLVING =	BIT(1),
	MESH_PATH_DSN_VALID =	BIT(2),
	MESH_PATH_FIXED	=	BIT(3),
	MESH_PATH_RESOLVED =	BIT(4),
};

/**
 * struct mesh_path - mac80211 mesh path structure
 *
 * @dst: mesh path destination mac address
 * @dev: mesh path device
 * @next_hop: mesh neighbor to which frames for this destination will be
 * 	forwarded
 * @timer: mesh path discovery timer
 * @frame_queue: pending queue for frames sent to this destination while the
 * 	path is unresolved
 * @dsn: destination sequence number of the destination
 * @metric: current metric to this destination
 * @hop_count: hops to destination
 * @exp_time: in jiffies, when the path will expire or when it expired
 * @discovery_timeout: timeout (lapse in jiffies) used for the last discovery
 * 	retry
 * @discovery_retries: number of discovery retries
 * @flags: mesh path flags, as specified on &enum mesh_path_flags
 * @state_lock: mesh pat state lock
 *
 *
 * The combination of dst and dev is unique in the mesh path table. Since the
 * next_hop STA is only protected by RCU as well, deleting the STA must also
 * remove/substitute the mesh_path structure and wait until that is no longer
 * reachable before destroying the STA completely.
 */
struct mesh_path {
	u8 dst[ETH_ALEN];
	struct hostapd_data *hapd;
	struct sta_info *next_hop;
//	struct timer_list timer;
	u32 dsn;
	u32 metric;
	u8 hop_count;
	unsigned long exp_time;
	u32 discovery_timeout;
	u8 discovery_retries;
	enum mesh_path_flags flags;
};

/* Mesh IEs constants */
#define MESH_CFG_LEN		19

/* Default maximum number of plinks per interface */
#define MESH_MAX_PLINKS		256

/* Pending ANA approval */
#define PLINK_CATEGORY		30
#define MESH_PATH_SEL_CATEGORY	32

/**
 * enum plink_state - state of a mesh peer link finite state machine
 *
 * @PLINK_LISTEN: initial state, considered the implicit state of non existant
 * 	mesh peer links
 * @PLINK_OPN_SNT: mesh plink open frame has been sent to this mesh peer
 * @PLINK_OPN_RCVD: mesh plink open frame has been received from this mesh peer
 * @PLINK_CNF_RCVD: mesh plink confirm frame has been received from this mesh
 * 	peer
 * @PLINK_ESTAB: mesh peer link is established
 * @PLINK_HOLDING: mesh peer link is being closed or cancelled
 * @PLINK_BLOCKED: all frames transmitted from this mesh plink are discarded
 */
enum plink_state {
	PLINK_LISTEN,
	PLINK_OPN_SNT,
	PLINK_OPN_RCVD,
	PLINK_CNF_RCVD,
	PLINK_ESTAB,
	PLINK_HOLDING,
	PLINK_BLOCKED
};

/* Mesh paths */
struct mesh_path *mesh_path_lookup(u8 *dst);
void mesh_path_assign_nexthop(struct mesh_path *mpath, struct sta_info *sta);
void mesh_path_tx_pending(struct mesh_path *mpath);
int mesh_path_add(u8 *dst, struct hostapd_data *hapd);
static inline void mesh_path_activate(struct mesh_path *mpath)
{
	mpath->flags |= MESH_PATH_ACTIVE | MESH_PATH_RESOLVED;
}

/* Mesh plinks */
int mesh_plink_open(struct hostapd_data *hapd, struct sta_info *sta);
void mesh_neighbour_update(struct hostapd_data *hapd, u8 *hw_addr, u64 rates);


void mesh_mgmt_ies_add(u8 *pos, struct hostapd_data *hapd);

/**
 * enum ieee80211_band - supported frequency bands
 *
 * The bands are assigned this way because the supported
 * bitrates differ in these bands.
 *
 * @IEEE80211_BAND_2GHZ: 2.4GHz ISM band
 * @IEEE80211_BAND_5GHZ: around 5GHz band (4.9-5.7)
 */
enum ieee80211_band {
	IEEE80211_BAND_2GHZ,
	IEEE80211_BAND_5GHZ,

	/* keep last */
	IEEE80211_NUM_BANDS
};

/**
 * enum ieee80211_channel_flags - channel flags
 *
 * Channel flags set by the regulatory control code.
 *
 * @IEEE80211_CHAN_DISABLED: This channel is disabled.
 * @IEEE80211_CHAN_PASSIVE_SCAN: Only passive scanning is permitted
 *	on this channel.
 * @IEEE80211_CHAN_NO_IBSS: IBSS is not allowed on this channel.
 * @IEEE80211_CHAN_RADAR: Radar detection is required on this channel.
 */
enum ieee80211_channel_flags {
	IEEE80211_CHAN_DISABLED		= 1<<0,
	IEEE80211_CHAN_PASSIVE_SCAN	= 1<<1,
	IEEE80211_CHAN_NO_IBSS		= 1<<2,
	IEEE80211_CHAN_RADAR		= 1<<3,
};

/**
 * struct ieee80211_channel - channel definition
 *
 * This structure describes a single channel for use
 * with cfg80211.
 *
 * @center_freq: center frequency in MHz
 * @hw_value: hardware-specific value for the channel
 * @flags: channel flags from &enum ieee80211_channel_flags.
 * @orig_flags: channel flags at registration time, used by regulatory
 *	code to support devices with additional restrictions
 * @band: band this channel belongs to.
 * @max_antenna_gain: maximum antenna gain in dBi
 * @max_power: maximum transmission power (in dBm)
 * @orig_mag: internal use
 * @orig_mpwr: internal use
 */
struct ieee80211_channel {
	enum ieee80211_band band;
	u16 center_freq;
	u16 hw_value;
	u32 flags;
	int max_antenna_gain;
	int max_power;
	u32 orig_flags;
	int orig_mag, orig_mpwr;
};

/**
 * enum ieee80211_rate_flags - rate flags
 *
 * Hardware/specification flags for rates. These are structured
 * in a way that allows using the same bitrate structure for
 * different bands/PHY modes.
 *
 * @IEEE80211_RATE_SHORT_PREAMBLE: Hardware can send with short
 *	preamble on this bitrate; only relevant in 2.4GHz band and
 *	with CCK rates.
 * @IEEE80211_RATE_MANDATORY_A: This bitrate is a mandatory rate
 *	when used with 802.11a (on the 5 GHz band); filled by the
 *	core code when registering the wiphy.
 * @IEEE80211_RATE_MANDATORY_B: This bitrate is a mandatory rate
 *	when used with 802.11b (on the 2.4 GHz band); filled by the
 *	core code when registering the wiphy.
 * @IEEE80211_RATE_MANDATORY_G: This bitrate is a mandatory rate
 *	when used with 802.11g (on the 2.4 GHz band); filled by the
 *	core code when registering the wiphy.
 * @IEEE80211_RATE_ERP_G: This is an ERP rate in 802.11g mode.
 */
enum ieee80211_rate_flags {
	IEEE80211_RATE_SHORT_PREAMBLE	= 1<<0,
	IEEE80211_RATE_MANDATORY_A	= 1<<1,
	IEEE80211_RATE_MANDATORY_B	= 1<<2,
	IEEE80211_RATE_MANDATORY_G	= 1<<3,
	IEEE80211_RATE_ERP_G		= 1<<4,
};

/**
 * struct ieee80211_rate - bitrate definition
 *
 * This structure describes a bitrate that an 802.11 PHY can
 * operate with. The two values @hw_value and @hw_value_short
 * are only for driver use when pointers to this structure are
 * passed around.
 *
 * @flags: rate-specific flags
 * @bitrate: bitrate in units of 100 Kbps
 * @hw_value: driver/hardware value for this rate
 * @hw_value_short: driver/hardware value for this rate when
 *	short preamble is used
 */
struct ieee80211_rate {
	u32 flags;
	u16 bitrate;
	u16 hw_value, hw_value_short;
};

/**
 * struct ieee80211_ht_info - describing STA's HT capabilities
 *
 * This structure describes most essential parameters needed
 * to describe 802.11n HT capabilities for an STA.
 *
 * @ht_supported: is HT supported by STA, 0: no, 1: yes
 * @cap: HT capabilities map as described in 802.11n spec
 * @ampdu_factor: Maximum A-MPDU length factor
 * @ampdu_density: Minimum A-MPDU spacing
 * @supp_mcs_set: Supported MCS set as described in 802.11n spec
 */
struct ieee80211_ht_info {
	u16 cap; /* use IEEE80211_HT_CAP_ */
	u8 ht_supported;
	u8 ampdu_factor;
	u8 ampdu_density;
	u8 supp_mcs_set[16];
};

/**
 * struct ieee80211_supported_band - frequency band definition
 *
 * This structure describes a frequency band a wiphy
 * is able to operate in.
 *
 * @channels: Array of channels the hardware can operate in
 *	in this band.
 * @band: the band this structure represents
 * @n_channels: Number of channels in @channels
 * @bitrates: Array of bitrates the hardware can operate with
 *	in this band. Must be sorted to give a valid "supported
 *	rates" IE, i.e. CCK rates first, then OFDM.
 * @n_bitrates: Number of bitrates in @bitrates
 */
struct ieee80211_supported_band {
	struct ieee80211_channel *channels;
	struct ieee80211_rate *bitrates;
	enum ieee80211_band band;
	int n_channels;
	int n_bitrates;
	struct ieee80211_ht_info ht_info;
};

#endif /* _MESH_H_ */
