#!/usr/bin/perl
`ifconfig ath0 down`;

print "reload driver\n";
`./madwifi.pl`;

print "change to ap mode\n";
`./change_interface_mode.pl ap`;

print "change mac address\n";
`ifconfig ath0 down`;
`ifconfig ath0 hw ether 00:1F:3A:85:A9:9c`;
`ifconfig ath0 up`;


