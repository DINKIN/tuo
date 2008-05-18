#include <includes.h>
#include <stdio.h>

#include "common.h"
#include "hostapd.h"
#include "sta_info.h"
#include "mesh.h"
#include "eloop.h"
#include "list.h"

/* Keep the mean chain length below this constant */
#define MEAN_CHAIN_LEN		2

#define MPATH_EXPIRED(mpath, now) ((mpath->flags & MESH_PATH_ACTIVE) && \
				(now >= mpath->exp_time) && \
				!(mpath->flags & MESH_PATH_FIXED))

struct mpath_node {
	struct hlist_node list;
	/* This indirection allows two different tables to point to the same
	 * mesh_path structure, useful when resizing
	 */
	struct mesh_path *mpath;
};

static struct mesh_table *mesh_paths;

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
	struct mesh_path *mpath;
	struct hlist_node *n;
	struct hlist_head *bucket;
	struct mesh_table *tbl;
	struct mpath_node *node;
	time_t now;

	time(&now);

	tbl = mesh_paths;

	bucket = &tbl->hash_buckets[STA_HASH(dst)];
	hlist_for_each_entry(node, n, bucket, list) {
		mpath = node->mpath;
		if (memcmp(dst, mpath->dst, ETH_ALEN) == 0) {
			if (MPATH_EXPIRED(mpath, now)) {
				mpath->flags &= ~MESH_PATH_ACTIVE;
			}
			return mpath;
		}
	}
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
	struct mesh_path *mpath, *new_mpath;
	struct mpath_node *node, *new_node;
	struct hlist_head *bucket;
	struct hlist_node *n;
	int grow = 0;
	int err = 0;
	u32 hash_idx;
	time_t now;

	if (memcmp(dst, hapd->own_addr, ETH_ALEN) == 0)
		/* never add ourselves as neighbours */
		return -1;

	if (is_multicast_ether_addr(dst))
		return -1;

	if (hapd->mpaths < MESH_MAX_MPATHS) {
		hapd->mpaths++;
	} else {
		return -1;
	}

	new_mpath = wpa_zalloc(sizeof(struct mesh_path));
	if (!new_mpath) {
		hapd->mpaths--;
		err = -1;
		goto endadd2;
	}
	memcpy(new_mpath->dst, dst, ETH_ALEN);
	new_mpath->flags = 0;
	new_node = wpa_zalloc(sizeof(struct mpath_node));
	new_node->mpath = new_mpath;
	new_mpath->timer_data = (void *) new_mpath;
	new_mpath->timer_function = mesh_path_timer;
	time(&now);
	new_mpath->exp_time = now;
	eloop_register_timeout(new_mpath->exp_time, 0, new_mpath->timer_function, new_mpath->timer_data, NULL);

	hash_idx = STA_HASH(dst);
	bucket = &mesh_paths->hash_buckets[hash_idx];

	hlist_for_each_entry(node, n, bucket, list) {
		mpath = node->mpath;
		if (memcmp(dst, mpath->dst, ETH_ALEN) == 0) {
			err = -2;
			hapd->mpaths--;
			free(new_node);
			free(new_mpath);
			goto endadd;
		}
	}

	hlist_add_head(&new_node->list, bucket);
	mesh_paths->entries++;

endadd:
endadd2:
	return err;
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

static void mesh_path_node_free(struct hlist_node *p, bool free_leafs)
{
	struct mesh_path *mpath;
	struct mpath_node *node = hlist_entry(p, struct mpath_node, list);
	mpath = node->mpath;
	if (free_leafs)
		free(mpath);
	free(node);
}

static void mesh_path_node_copy(struct hlist_node *p, struct mesh_table *newtbl)
{
	struct mesh_path *mpath;
	struct mpath_node *node, *new_node;
	u32 hash_idx;

	node = hlist_entry(p, struct mpath_node, list);
	mpath = node->mpath;
	new_node = wpa_zalloc(sizeof(struct mpath_node));
	new_node->mpath = mpath;
	hash_idx = STA_HASH(mpath->dst);
	hlist_add_head(&new_node->list,
			&newtbl->hash_buckets[hash_idx]);
}

int mesh_pathtbl_init(void)
{
	mesh_paths = mesh_table_alloc(STA_HASH_SIZE);
	mesh_paths->free_node = &mesh_path_node_free;
	mesh_paths->copy_node = &mesh_path_node_copy;
	mesh_paths->mean_chain_len = MEAN_CHAIN_LEN;
	if (!mesh_paths)
		return -ENOMEM;
	return 0;
}

void mesh_pathtbl_unregister(void)
{
	mesh_table_free(mesh_paths, true);
}

