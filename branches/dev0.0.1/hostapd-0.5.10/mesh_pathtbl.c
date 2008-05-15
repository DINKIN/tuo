#include <includes.h>
#include <stdio.h>

#include "common.h"
#include "hostapd.h"
#include "sta_info.h"
#include "mesh.h"

/**
 *
 * mesh_path_assign_nexthop - update mesh path next hop
 *
 * @mpath: mesh path to update
 * @sta: next hop to assign
 *
 * Locking: mpath->state_lock must be held when calling this function
 */
void mesh_path_assign_nexthop(struct mesh_path *mpath, struct sta_info *sta)
{
	mpath->next_hop = sta;
}

/**
 * mesh_path_lookup - look up a path in the mesh path table
 * @dst: hardware address (ETH_ALEN length) of destination
 * @dev: local interface
 *
 * Returns: pointer to the mesh path structure, or NULL if not found
 *
 * Locking: must be called within a read rcu section.
 */
struct mesh_path *mesh_path_lookup(u8 *dst)
{
	return NULL;
}

/**
 * mesh_path_add - allocate and add a new path to the mesh path table
 * @addr: destination address of the path (ETH_ALEN length)
 * @dev: local interface
 *
 * Returns: 0 on sucess
 *
 * State: the initial state of the new path is set to 0
 */
int mesh_path_add(u8 *dst, struct hostapd_data *hapd)
{
	return 0;
}

/**
 * mesh_path_tx_pending - sends pending frames in a mesh path queue
 *
 * @mpath: mesh path to activate
 *
 * Locking: the state_lock of the mpath structure must NOT be held when calling
 * this function.
 */
void mesh_path_tx_pending(struct mesh_path *mpath)
{
}

