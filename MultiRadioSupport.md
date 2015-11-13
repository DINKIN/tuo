#Modification for Supporting Multi-Radio Mesh Node.

# Routing Table #

Find routing item by hashing destination MAC rather than destination MAC and packet incoming interface.

Modify in function

# PREQ #

Initial node copy the PREQ packet to each Mesh-Point interface in function ....

Forwarding PREQ packet to each Mesh-Point interface in function ....

# Forwarding #

## ieee80211s\_lookup\_nexthop - put the appropriate next hop on a mesh frame ##
  * @next\_hop: output argument for next hop address
  * @skb: frame to be sent
  * @dev: network device the frame will be sent through
int mesh\_nexthop\_lookup(u8 **next\_hop, struct sk\_buff**skb, struct net\_device **dev)**

==>

  * @next\_hop: output argument for next hop address
  * @skb: frame to be sent
  * @income\_dev: network device the frame came in
  * @outgoing\_dev: network device the frame will be sent through
int mesh\_nexthop\_lookup(u8 **next\_hop, struct sk\_buff**skb,
struct net\_device **income\_dev,
struct net\_device**outgoing\_dev)