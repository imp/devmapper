/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2010 Grigale Ltd. All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	SYS_DM_H
#define	SYS_DM_H

/*
 * Device Mapper internal structures definitions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/nvpair.h>

/*
 * The internal mapping list is kept as nvlist of per-mapping nvlists
 * Each device map is described by the nvlist containing next nvpairs
 *
 * Name     | Type         | Value
 * ---------+--------------+------
 * name     | STRING       | name of the device
 * minor    | INT          | device minor number
 * trgarr   | NVLIST ARRAY | array of target definitions
 *
 * The top-level nvlist uses mapping names as a key, so that the name
 * essentially appears twice. Once as the key in the top-level list,
 * and twice as a `name' nvpair in the map nvlist itself.
 */

#define	DM_NAME		"name"
#define	DM_MINOR	"minor"
#define	DM_TRGARR	"trgarr"

enum {
	DM_TARGET_LINEAR = 0,
	DM_TARGET_STRIPED,
	DM_TARGET_ERROR
};

enum {
	DM_OPENED = 1,
	DM_SUSPENDED = 2,
	DM_XXX = 4
};

typedef struct {
	dev_info_t	*dip;
	uint64_t	state;	/* State bit-field */
	krwlock_t	slock;  /* State rw-lock */
	nvlist_t	*mlp;	/* Mapping list pointer */
} dm_state_t;

#ifdef __cplusplus
}
#endif

#endif	/* SYS_DM_H */
