# Introduction #

Add your content here.


# Scheduling by priority queue #

Useful Commands:

  * show queue discipline.
﻿
tc qdisc show [dev wlan1 ](.md)

  * add a priority queue
﻿
tc qdisc add dev wlan1 root handle 1: prio

  * delete a priority queue
﻿
tc qdisc del dev wlan1 root prio

  * add a filter for priority queue
﻿
tc filter add dev $DEV parent 1: protocol ip prio 1 u32 match ip dst 192.168.1.5/32 flowid 1:1