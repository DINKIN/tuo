# Multi-Channel #

802.11a理论上的正交信道间隔4个信道，实际上需要间隔8个信道。

但是如果同一个节点既有接收信道又有发送信道，间隔还需要远。

# Multi-Hop #

```
Link Capacity 6Mbps

Single Interface (ath1), Single Channel (44)
7-6-10

./iperf -u -c 192.168.1.10 -b 8M -t 5

Single Hop Throughput 5.35Mbps Packet Drop Rate 0.11%

Two Hop Experiments (unit Mbps)
#exp    iperf_send_speed    tx      rx      Packet_Drop_Rate
1       1                   1       0.971   3.1%
2       2                   2       1.84    8.2%
3       3                   3       1.73    39%
4       4                   3.34    2.07    35%
5       5                   3.34    2.02    37%
6       6                   3.28    2.02    40%
7       7                   3.29    2.02    40%
8       8                   3.49    2.02    45%


Three Hop Experiments (unit Mbps)
#exp    iperf_send_speed    tx      rx      Packet_Drop_Rate
1       6                   2.68    1.47    33%

Multiple Interface, Multiple Channel
7_0-(36)-6_0 6_1-(44)-10_1

Two Hop Experiments (unit Mbps)
#exp    iperf_send_speed    tx      rx      Packet_Drop_Rate
1       1                   1       0.993   0.7%
2       2                   2       1.99    0.47%
3       3                   3       2.99    0.078%
4       4                   3.99    3.96    0.53%
5       5                   4.97    4.87    0.71%
6       6                   5.28    5.10    0.89%
7       7                   5.21    5.07    0.63%
8       8                   5.22    5.08    1.2%

Mulitple Hop Experiments (unit Mbps)
#exp    hops    iperf_send_speed    tx      rx      Packet_Drop_Rate    Delay
1       3       6                   5.25    5.09    0.045%              2.625ms
2       4       6                   5.19    5.05    0.045%              3.095ms
3       5       6                   5.23    5.03    0.27%               9.915ms
4       6       6                   5.19    5.03    0.54%               10.63ms
5       7       6                   5.24    4.78    4.9%               11.186ms

```