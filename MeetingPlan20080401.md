#A brief plan description for the meeting.

# Introduction #

Discuss the 802.11s draft detail.


# Details #

Contents:
  * How to implement the multi-radio.
  * How to support the encryption.

# Summary #

Multi-Radio support:
  * only one path table (routing table) _mesh\_paths_ for all interfaces.
  * _ifsta->preq\_queue_ storage the destination addresses which are waiting for routing calculation.


Encryption:

