# 可移植Mesh功能实现 #

路由功能在应用层实现，三层路由可以基于OLSR实现，二层路由可以作为hostapd的扩展实现，两者可以共用一个框架，实现程序复用，使用C语言开发。

转发功能在内核实现，三层转发IP层已经实现，二层转发实现成一个独立的可加载模块，定义标准接口，特定网卡驱动程序实现该接口以支持二层转发功能。

安全功能，对于控制报文加解密，以及节点认证的工作可以在应用层来实现，对于数据报文的加解密需要使用内核模块来实现。

优点：
  * 独立任何驱动程序，驱动程序只需要实现二层转发所需要的接口。
  * 同时涵盖三层和二层路由。
  * 复杂的路由功能在应用层实现，便于开发和调试。
  * 二层和三层路由功能复用，节省开发和调试时间。

# 11s实现的考虑 #

mac80211中的802.11s实现是现在目前唯一能够找到的开源802.11s实现。但是目前能够和mac80211稳定工作的驱动勉强只有Broadcom的b43/b43legacy（支持AP, AdHoc, Full Monitor），但是其不支持802.11a（这个可能不是一个问题，因为11a的覆盖范围很小，只有在部署非常密集和使用定向天线的情况才有意义）。Atheros公司的ath5k驱动应该是非常有前途的驱动，但是现在还在开发中（只支持AdHoc），也非常的不稳定。

所以目前有两种选择:
1. 继续使用mac80211,但使用Broadcom公司的chips作为我们的开发平台，优点是驱动功能相对全且稳定，缺点是前期我们没有足够的网卡设备（如果有一部分资金的话，这个不是问题）。
2. 把11s的实现从mac80211中剥离出来成为一个独立的模块，可以和其它稳定的但不是和mac80211工作的驱动很好的集成，优点是可以使用很多稳定的开源驱动，不依赖于特定驱动。缺点是可行性和难度的问题。

# Mesh 实现的考虑 #

目前有两种Wifi Mesh，网络层Mesh（应用层路由的方案olsr）和链路层Mesh（802.11s）。最终应该是这两种都有应用的需求。

第一步需要考察一下阿德利亚使用的是哪个方案，还有就是目前的Mesh产品使用的是什么方案。

# XXX-to-802.11 bridge #
Next Step

3G-to-802.11
Zigbee-to-802.11
WiMax-to-802.11
Satelite-to-802.11

# 开发计划 #
  1. Build development environment. (Finish)
  1. Customize kernel configuration file, deploy 11s to mini-box and soekris. Install the testbed environment, demo and documentation. (Scott)
  1. Security Function. (jerry)
  1. Open11s works on Hostapd (Scott) (May)
    1. sending beacon (2008.4.16 finished)
    1. Peer link management protocol.
    1. On-demand HWMP + broadcast by controlled flooding.
    1. Airtime link metric and per-peer rate adaptation
    1. Layer-2 Forwarding on Madwifi.
  1. Multi-Radio.  (Scott) (June)
    1. Routing table modify
    1. HWMP
    1. Peer link management
  1. Multi-Channel.  (Scott) (June)
  1. Support AP.  (Scott) (July)
  1. QoS Support.  (Scott)
  1. inter-network support. (Scott)