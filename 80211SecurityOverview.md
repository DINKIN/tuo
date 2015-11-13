# Introduction #

Two basic services are defined in wireless access control: authentication and data confidentiality.

An RSNA (which forces a set of security requirements that must be obeyed by newly-produced wireless devices ) uses 802.1X authentication service + TKIP/CCMP to provide access control.

# Authentication #

802.11 authentication operates at the link level between STAs. Two STAs will not be associated unless the authentication between them is completed successfully.

802.11 defines two authentication methods:
  * Open System Authentication
  * Shared Key Authentication

An RSNA also supports preshared key (PSK) based authentication. It utilizes EAP but allows different EAP methods to be used.

Preauthentication is typically done by a Sta while it is already associated with an AP which is previously authenticated. This can greatly improve the mobility performance for BSS-transitioning.

Deauthentication will definitely cause deassociation, and can be done by either side of the authenticated party.

# Data Confidentiality #
"in the clear" state for sender and receiver when dealing with confidentiality: if not encryption method is specified, all the messages are sent without being protected; if the sender thinks it unacceptable, it shall not send the message; if the receiver thinks it unacceptable, it shall discard the message.

# key Management #
4-Way handshake and Group Key Handshake.

# Data Origin Authenticity #
CCMP or TKIP can handle this.

# Replay Detection #
STA can detect an unauthorized retransmission by CCMP or TKIP.