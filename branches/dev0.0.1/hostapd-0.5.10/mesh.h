#ifndef _MESH_H_
#define _MESH_H_

#include <linux/types.h>

/* Mesh IEs constants */
#define MESH_CFG_LEN		19

/* Default maximum number of plinks per interface */
#define MESH_MAX_PLINKS		256

/* Pending ANA approval */
#define PLINK_CATEGORY		30
#define MESH_PATH_SEL_CATEGORY	32

/**
 * enum plink_state - state of a mesh peer link finite state machine
 *
 * @PLINK_LISTEN: initial state, considered the implicit state of non existant
 * 	mesh peer links
 * @PLINK_OPN_SNT: mesh plink open frame has been sent to this mesh peer
 * @PLINK_OPN_RCVD: mesh plink open frame has been received from this mesh peer
 * @PLINK_CNF_RCVD: mesh plink confirm frame has been received from this mesh
 * 	peer
 * @PLINK_ESTAB: mesh peer link is established
 * @PLINK_HOLDING: mesh peer link is being closed or cancelled
 * @PLINK_BLOCKED: all frames transmitted from this mesh plink are discarded
 */
enum plink_state {
	PLINK_LISTEN,
	PLINK_OPN_SNT,
	PLINK_OPN_RCVD,
	PLINK_CNF_RCVD,
	PLINK_ESTAB,
	PLINK_HOLDING,
	PLINK_BLOCKED
};

int mesh_plink_open(struct hostapd_data *hapd, struct sta_info *sta);
void mesh_neighbour_update(struct hostapd_data *hapd, u8 *hw_addr, u64 rates);
void mesh_mgmt_ies_add(u8 *pos, struct hostapd_data *hapd);

#endif /* _MESH_H_ */
