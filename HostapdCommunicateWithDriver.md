# Driver to Hostapd #

  1. Application layer use Rtnetlink (Netlink, which is used to transfer information between kernel and userspace processes).

```
rtnetlink_socket = socket(PF_NETLINK, int socket_type, NETLINK_ROUTE); 
```

  1. kernel is supported by Wireless Extensions APIs ( linux/net/wireless/wext.c )

```
1181/* ---------------------------------------------------------------- */
1182/*
1183 * Main event dispatcher. Called from other parts and drivers.
1184 * Send the event on the appropriate channels.
1185 * May be called from interrupt context.
1186 */
1187void wireless_send_event(struct net_device *    dev,
1188                         unsigned int           cmd,
1189                         union iwreq_data *     wrqu,
1190                         char *                 extra)
1191{
......
1259        /* Send via the RtNetlink event channel */
1260        rtmsg_iwinfo(dev, (char *) event, event_len);
......

1265        return;         /* Always success, I guess ;-) */
1266}
```

  1. Driver use wireless\_send\_event notify the event to application layer.

```
ieee80211_notify_node_join(struct ieee80211_node *ni, int newassoc)
{
......
		wireless_send_event(dev, IWEVREGISTERED, &wreq, NULL);
......
}
```