#How to Disable ICMP Redirects

# Linux #

http://www.itsyourip.com/Security/how-to-disable-icmp-redirects-in-linux-for-security-redhatdebianubuntususe-tested/

```
Server# /sbin/sysctl -w net.ipv4.conf.all.accept_redirects = 0
Server# /sbin/sysctl -w net.ipv4.conf.all.send_redirects = 0

Server# /sbin/sysctl -w net.ipv6.conf.all.accept_redirects = 0
Server# /sbin/sysctl -w net.ipv6.conf.all.send_redirects = 0
```

# Windows #

http://www.itsyourip.com/networking/disable-icmp-redirects-in-windows2000xp2003/
```
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters

In the right pane, double-click "EnableICMPRedirect" DWORD and change the value to "0"
```