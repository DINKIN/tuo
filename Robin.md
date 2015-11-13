#Overview of Robin

# Password #
ssh, security ap passwords are both

0p3nm35h

# Compile #

Have a look at readme on
https://www2.hosted-projects.com/trac/ansanto/robin/browser/compiling_robin

There are two readme file. One is for kamikaze\_7.09 and the other is for kamikaze\_trunk.

## Compile kamikaze\_trunk ##

Follow compiling\_robin/README\_trunk

a patch to package/base-files/Makefile
```
<<
$(CP) ./files/* $(1)
>>
$(CP) --remove-destination ./files/* $(1)
```
chmod +x build\_dir/linux-atheros/base-files/ipkg/base-files-atheros/usr/sbin/

chmod +x build\_dir/linux-atheros/base-files/ipkg/base-files-atheros/etc/init.d/

chmod +x build\_dir/linux-atheros/base-files/ipkg/base-files-atheros/etc/cp.conf/


# Flash #

The different FON and Meraki devices, with Atheros SoC, use the RedBoot bootloader and have either 4/16 or 8/32 MB of Flash/RAM and the default firmware is in both cases based on OpenWrt. It is fairly easy to substitute the existing system with the current OpenWrt Kamikaze release. The initial step required to install OpenWrt Kamikaze on these devices is the configuration of the RedBoot bootloader. It is possible to re-flash these devices without changing the bootloader configuration, but it is not recommended. **The argument is quite simple: Recovery from a firmware re-flash gone bad is only possible if RedBoot is setup to accept "inquireries" before boot of the system, from the flash chip.** Apart from this, **the size of the kernel of Kamikaze has changed so it is needed to alter the layout of the partitions on the devices.** Such re-partitioning is done with the help of RedBoot.

http://wiki.x-wrt.org/index.php/Kamikaze_Installation
http://ruckman.net/tech/2008/03/03/guide-to-flashing-a-merakiacctonfonero/
http://wiki.openwrt.org/OpenWrtDocs/Hardware/Fon/Fonera
http://www.open-mesh.com/activekb/questions/8/Flashing+the+Accton+Mini-Router+with+ROBIN

You need low-level access to the ethernet device,
(run as root on Linux, install WinPcap on Windows see http://winpcap.org/ )
http://wiki.openwrt.org/OpenWrtDocs/Hardware/Meraki/Mini
```
   1.Boot your Linux computer (or Live CD) and bring up a terminal window (under "Applications", then "Accessories", then "terminal" on Ubuntu)

   2.Make sure it is connected to the internet, then type the following to download the files you will need:

      sudo apt-get upgrade
      sudo apt-get install wget
      wget http://www.open-mesh.com/flashing/openwrt-atheros-2.6-vmlinux.gz
      wget http://www.open-mesh.com/flashing/openwrt-atheros-2.6-root.jffs2-64k
      wget http://www.open-mesh.com/flashing/easyflash
      wget http://www.open-mesh.com/flashing/flash
      chmod +x easyflash
      chmod +x flash

   3.Plug in your Accton or FON 2200 (check the bottom label for the model number) to your Ethernet LAN port on your computer. *Use a cross linked cable or a switch. sudo ifconfig eth0 up*
   
      Type: ./flash (note: If your router is NOT connected to eth0, modify the file "flash" accordingly.) It will show as follow.

Reading rootfs file openwrt-atheros-2.6-root.jffs2-64k with 3276800 bytes...
Reading kernel file openwrt-atheros-2.6-vmlinux.gz with 1048576 bytes...
rootfs(0x006a0000) + kernel(0x00100000) + nvram(0x00000000) sums up to 0x007a0000 bytes


   5.Plug in the router to A/C power.

   6.You should see something like this displayed:


Non arp received. Make sure, the device is connected directly!
Peer MAC: 00:12:cf:80:68:ea
Peer IP : 192.168.0.1
Your MAC: 00:ba:be:ca:ff:ee
Your IP : 192.168.0.0
Setting IP address...
Loading rootfs...
Sending rootfs, 6400 blocks...
Initializing partitions...
Rootfs partition size now 0x006b0000
Flashing rootfs...
Loading kernel...
Sending kernel, 2048 blocks...
Flashing kernel...
Setting boot_script_data...
Done. Restarting device...

   7.Go get lunch. This will take 15-30 minutes, depending upon the flash chip speed in your router. DO NOT INTERRUPT!

   8.When it is done, the Flash Utility will automatically close. 
```


# Update #

Robin update its information to server by http/https.
  * crond -c /etc/crontabs. Crond execute scheduled commands. /etc/crontabs/root as follow
```
0-59/5  * * * * /sbin/update
22              * * * * /sbin/upgrade
```
> First line run the script /sbin/update every 5 minutes to update the information to server.
> Second line run the script /sbin/upgrade on 22 minutes for each hour to upgrade the firmware.
> This file format detail can be found in crontab.
  * /etc/update/ directory contains all update related file. Log is in /tmp/update.log
  * /etc/update/received.clean contains the configuration from the server. /etc/update/update.arg is the uploading information from the local in url format.
  * Then the /sbin/update will call /etc/init.d/settings dashboard. It use /etc/update/received.clean and the scripts in /usr/sbin/update-\**to update the configuration files in /etc/config.**

# UCI #

/bin/uci is a script to access the configuration file. It use the /etc/functions.sh and the scripts on /lib/config to access the configuration files on /etc/config

# watchout4node #

script /sbin/watchout4node watch the follow aspects:
  * well known MadWifi/Atheros bugs
  * low memory
  * batmand
  * gateway upstream router connection
  * name server
  * internet connection
  * misc

> According the check results, the node will reboot or /sbin/node\_restart

# Nodogsplash #

Nodogsplash offers a simple way to provide restricted access to an internet connection. It is intended for use on wireless access points running OpenWRT (but may also work on other Linux-based devices).

## Speed Limit ##

Nodogsplash also optionally implements basic traffic control on its managed interface. This feature lets you specify the maximum aggregate upload and download bandwidth that can be taken by clients connected on that interface.

Nodogsplash implements this functionality by enabling two intermediate queue devices (IMQ's), one for upload and one for download, and attaching simple rate-limited HTB qdiscs to them. Rules are inserted in the router's iptables mangle PREROUTING and POSTROUTING tables to jump to these IMQ's. The result is simple but effective tail-drop rate limiting (no packet classification or fairness queueing is done).

There are two configuration file in /etc/nodogsplash, nodogsplash.conf nodogsplash.conf.std. The content of those files is as follow.

```
FirewallRuleSet preauthenticated-users {
FirewallRule allow
}
FirewallRuleSet authenticated-users {
FirewallRule allow
}
FirewallRuleSet users-to-router {
FirewallRule allow udp port 53
FirewallRule allow tcp port 53
FirewallRule allow udp port 67
FirewallRule allow tcp port 20
FirewallRule allow tcp port 21
FirewallRule allow tcp port 22
FirewallRule allow tcp port 23
FirewallRule allow tcp port 80
FirewallRule allow tcp port 443
}
GatewayName orange test
RedirectURL http://
ClientIdleTimeout 1440
ClientForceTimeout 1440
AuthenticateImmediately 1
TrafficControl 1
DownloadLimit 4000
UploadLimit 100
#bogus2 772827811
GatewayInterface     br-lan
```

You should change the DownloadLimit or UploadLimit in nodogsplash.conf.std. When node reboot, system will copy nodogsplash.conf.std to nodogsplash.conf.

Now, open-mesh doesn't support update the DownloadLimit and UploadLimit.

## ndsctl ##

# DHCP #

DNSMasq works as DHCP server on open mesh AP very well. But it can't work with MySecure.

# Routing Table #

To have a look robin's routing table, you should use ip rule and ip route show table XX instead of route, which just output the main routing table. The follow is an example.

```
root@open-mesh:/# ip rule
0:      from all lookup local 
6600:   from all to 5.0.0.0/8 lookup 66 
6699:   from all lookup 65 
6700:   from all to 5.0.0.0/8 lookup 67 
6800:   from all iif lo lookup 68 
6801:   from 127.0.0.0/8 lookup 68 
6802:   from 10.104.234.0/25 lookup 68 
6803:   from 10.104.234.128/25 lookup 68 
32766:  from all lookup main 
32767:  from all lookup default 
root@open-mesh:/# 
root@open-mesh:/# /usr/sbin/ip route show table local
broadcast 10.104.234.255 dev ath2  proto kernel  scope link  src 10.104.234.129 
broadcast 127.255.255.255 dev lo  proto kernel  scope link  src 127.0.0.1 
broadcast 5.0.0.0 dev ath0  proto kernel  scope link  src 5.128.104.234 
local 169.254.0.1 dev gate0  proto kernel  scope host  src 169.254.0.1 
broadcast 10.104.234.127 dev br-lan  proto kernel  scope link  src 10.104.234.1 
broadcast 10.104.234.0 dev br-lan  proto kernel  scope link  src 10.104.234.1 
local 10.104.234.1 dev br-lan  proto kernel  scope host  src 10.104.234.1 
broadcast 10.104.234.128 dev ath2  proto kernel  scope link  src 10.104.234.129 
broadcast 5.255.255.255 dev ath0  proto kernel  scope link  src 5.128.104.234 
local 5.128.104.234 dev ath0  proto kernel  scope host  src 5.128.104.234 
local 10.104.234.129 dev ath2  proto kernel  scope host  src 10.104.234.129 
broadcast 127.0.0.0 dev lo  proto kernel  scope link  src 127.0.0.1 
local 127.0.0.1 dev lo  proto kernel  scope host  src 127.0.0.1 
local 127.0.0.0/8 dev lo  proto kernel  scope host  src 127.0.0.1 
root@open-mesh:/# 
root@open-mesh:/# /usr/sbin/ip route show table 68   
throw 10.104.234.0/25  proto static 
throw 10.104.234.128/25  proto static 
throw 5.0.0.0/8  proto static 
throw 127.0.0.0/8  proto static 
default dev gate0  proto static  scope link  src 169.254.0.1 
```

**Important: You have to use /usr/sbin/ip instead of /bin/ip**