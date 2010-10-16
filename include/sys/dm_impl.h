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

#ifndef	SYS_DM_IMPL_H
#define	SYS_DM_IMPL_H

/*
 * Device Mapper implementation internal structures definitions
 */

#include <sys/types.h>
#include <sys/map.h>
#include <sys/refstr.h>
#include <sys/sunldi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	dev_info_t	*dip;
	ldi_ident_t	li;	/* LDI identifier */
	struct map	*dm_minor_map;
	void		*dm_infop;
	uint64_t	state;	/* State bit-field */
} dm_state_t;

typedef struct {
	refstr_t	*name;	/* Mapping name */
	refstr_t	*dev;	/* Target device name */
	ldi_handle_t	lh;	/* LDI handle */
	uint64_t	target;	/* Target / Index in table */
} dm_info_t;

#ifdef __cplusplus
}
#endif

#endif	/* SYS_DM_IMPL_H */
