#!/usr/bin/perl

$DIR="../madwifi-0.9.4";

if($ARGV[0] == "stop") {
	`rmmod $DIR/ath/ath_pci.ko`;
	`rmmod $DIR/ath_rate/sample/ath_rate_sample.ko`;
	`rmmod wlan_scan_ap.ko`;
	`rmmod $DIR/net80211/wlan_scan_sta.ko`;
	`rmmod $DIR/ath_hal/ath_hal.ko`;
	`rmmod $DIR/net80211/wlan.ko`;
}

if($ARGV[0] == "start") {
	$cmd = "insmod";
	`$cmd $DIR/net80211/wlan.ko`;
	`$cmd $DIR/ath_hal/ath_hal.ko`;
	`$cmd $DIR/net80211/wlan_scan_sta.ko`;
	`$cmd $DIR/ath_rate/sample/ath_rate_sample.ko`;
	`$cmd $DIR/ath/ath_pci.ko`;
}

