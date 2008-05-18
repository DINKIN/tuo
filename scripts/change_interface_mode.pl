#!/usr/bin/perl

$mode = ap;
print "$ARGV[0]\n";
if($#ARGV==0){
	$mode=$ARGV[0];
}

print "set ath0 to mode $mode\n";

`ifconfig ath0 down`;
`wlanconfig ath0 destroy`;
`wlanconfig ath0 create wlandev wifi0 wlanmode $mode`;
`iwconfig ath0 essid test channel 1 txpower 17`;
