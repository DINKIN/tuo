Some information provided by http://madwifi.org/wiki/UserDocs/UsersGuideExamples is not accurate. So we write this guideline from our specific experiences under Fedora 8.

# Install As a STA #

> Install the Ath5k in a PC to act as a STA and try to connect to an AP with essid "MYAP" and WPA\_PSK encryption key "MYPSWD".
```
Lookup the dmesg to find the name of wireless interface, say, wlan1

#] ifconfig wlan1 up
#] iwconfig wlan1 essid "MYAP"

Create a wpa_supplicant.conf for wpa_supplicant utils, and input the following content:

ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=wheel

network={
        ssid="MYAP"
        scan_ssid=1
        key_mgmt=WPA-PSK
        psk="MYPSWD"
}

#] wpa_supplicant -iwlan1 -c wpa_supplicant.conf 
#] iwconfig
#] ifconfig 
You will see wlan1 has associated with the AP and obtained a dynamic IP.

```