# Four Address Frame #

struct ieee80211\_hdr\_4addr {

> le16 frame\_ctl;

> le16 duration\_id;

> u8 addr1[ETH\_ALEN](ETH_ALEN.md);      // **next hop address**

> u8 addr2[ETH\_ALEN](ETH_ALEN.md);      // **tx interface address**

> u8 addr3[ETH\_ALEN](ETH_ALEN.md);      // **802.3 dst address**

> le16 seq\_ctl;

> u8 addr4[ETH\_ALEN](ETH_ALEN.md);      // **802.3 src address**

> u8 payload[0](0.md);
} attribute ((packed));