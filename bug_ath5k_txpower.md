#bug report for ath5k txpower setting

# Introduction #

This is not a critical bug, we can wait until ath5k developer fix the bug or before our release first version.

ath5k doesn't support change the txpower. Correlative function and data structure as follow:

  * iw extension handler define in mac80211

ieee80211\_iw\_handler\_def --> ieee80211\_handler -->
ieee80211\_ioctl\_siwtxpower --> ieee80211\_hw\_config -->

  * register data structure.
ath5k\_hw\_ops -->

  * handler in ath5k
ath5k\_config --> ath5k\_chan\_set --> ath5k\_hw\_set\_txpower\_limit --> ath5k\_hw\_txpower --> ath5k\_txpower\_table