#!/usr/bin/perl

`ifconfig ath1 down`;
`wlanconfig ath1 destroy`;
`wlanconfig ath1 create wlandev wifi0 wlanmode monitor`;
`ifconfig ath1 up`;
