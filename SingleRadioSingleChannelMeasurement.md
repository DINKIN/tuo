#Single Radio Single Channel Measurement by 802.11g

# Multi-Hop Experiments #

Node is soekris net-4801 box.

Topology

Laptop1 

&lt;-&gt;

 Node1 

&lt;-&gt;

 Node2 

&lt;-&gt;

 Node3 

&lt;-&gt;

 Node4

Iperf sender on laptop1 and receiver on soekris box.

```
#Hop    TCP Throughput(Mbps)
1       6.87
2       8.81
3       4.07
4       2.5
5       0.622
```

I found if there are nodes who can't be heard by other nodes, the performance drop a lot. I think the reason is the carrier sensing doesn't work in this case. In this case, the TDMA may be helpful.

Based the idea from MIT's XOR, we can introduce more retransmission for the further hop, causing the last hop packet drop will waste all previous bandwidth. But we should control the retransmission's direction. For example, according flow from Laptop1 to Node4, Node3 is the last hop and should have more retransmission for link Node3 -> Node4. but according flow from Node4 to Laptop1, Node3 is the first hop and should have less retransmission for link Node3 -> Node2. So the Node3 have to have the ability to control retransmission times for different destination.