# Other solution prices #

Mountain View uses some 400 Tropos routers covering 12 square miles, or about 35 to 40 routers per square mile. That sounds like a (now standard) $100K+ per sq mile install, or $1.2 million.

# Antenna #

我们需要工作在2.4GHz的板状天线.我发现国内定向天线的供应还是很充足的,我觉得我们可以从价格,供货远近,供货时间,OEM能力等方面挑选一些公司(你看还有什么需要注意的).

北京胜百天线有限公司
SPDB-2400-14V45
http://www.sunparl.com/showpro.asp?id=774&bclass=%B0%E5%D7%B4%CC%EC%CF%DF&rootid=

廊坊市大洋洲电子科技有限公司
http://cn.lfdyz.cnele.com/

下面的链接有4款板状定向天线,都在300多,也就是在$50左右,所以在国内搞定定向天线应该不是问题.
http://www.sun118.com/athena/offerlist/bjhengan-sale-1-true-487143.html

这是淘宝上的板状天线
http://search.china.alibaba.com/search/offer_search.htm?keywords=%B0%E5%D7%B4%CC%EC%CF%DF

# Atheros SOC #
Fonera+ (FON2201)
拥有一个以太网口和一个WAN口

> WILIGEAR Integrated radio board WBD-500
http://www.wiligear.com/?q=products/cpu-boards/wbd-500

802. 11b / G Mini Router
http://yufotech.trustpass.alibaba.com/product/101002791/802_11b_G_Mini_Router.html
> Atheros Communications' AR5007AP-G is a complete 802.11b/g wireless access point SoC that integrates a wireless network processor, media access controller (MAC), baseband, and all radio functions up to the antenna.

> The chip represents a component reduction of 40 percent compared to the company's AR5006AP-G access point solution. It supports IEEE 802.11i security and 802.11e quality of service (QoS) specifications.

> Key WLAN components that have been integrated onto the device include a power amplifier, low noise amplifiers (LNA), antenna switch, and transmit/receive filtering.

> According to the company, this is the most highly integrated single-chip wireless LAN (WLAN) solution for access point and router products.

> The AR5007AP-G is priced at $12.50 each in 10,000-unit quantities. It is available in volume quantities now. More information is available at www.atheros.com.

I think Meraki indoor unit is based on Atheros AR5007AP-G design which has power amplifier, LNA and even antenna TX/RX switch all inside the SoC. This old news item ( »www.wirelessnetdesignline.com/sh···74401548) mention SoC cost was $12.50 in 10K volume.

I hear that Accton and other ODMs are selling fully packaged white-box versions to OEMs at $25 range and many wholesale at $49 or lower. We need a few WISP-OS companies to sell low-cost firmware licenses for these units.

AP51 seems to be the Atheros AR5006AP-G reference design. I had assumed AR5007AP-G which uses chip marked AR2317

The biggest downside of AR2315/AR2316 is overheating problem and that's why every major vendor is already upgrading or has upgraded to AR2317/AR2318.


11a/b/g 108Mpbs Atheros SOC modual-500mW
  * Other Price Terms: $100
  * Supply Ability: 1000pcs per month
  * Delivery Time: 14days

Atheros Ar2315 Wireless AP / Cilent Soc Platform
http://yufotech.trustpass.alibaba.com/product/12197450/Atheros_Ar2315_Wireless_AP_Cilent_Soc_Platform.html
> Price Terms: FOB Taiwan USD 50~55
Terms of Payment: L/C,T/T,Western Union
Supply Ability: 5000 Piece/Pieces per Month
Minimum Order: 100 Piece/Pieces
Packaging: Bulk package, Bootloader
Delivery Lead Time: 4 weeks



# Outdoor Node #
http://yufotech.en.ec21.com/product_detail.jsp?group_id=GC01216727&product_id=CA01629728&product_nm=802.11b/G%20AP/Bridge/Client/WDS%20with%20High%20Power%20Output%20Radio

# Outdoor Node with Directional Antenna #
这个设备集成了节点和天线
http://yufotech.en.ec21.com/product_detail.jsp?group_id=GC01216727&product_id=CA01629705&product_nm=802_11b%252FG_AP%252FBrdige%252FClient%252FWDS_with_High_Power_Output_Radio_Built-in_12%252F18dBi_Antenna