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
 * Copyright 2011 Grigale Ltd. All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	SYS_DM_OPS_H
#define	SYS_DM_OPS_H

/*
 * Device Mapper module operations
 */

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	DM_PLUGIN_OPS_REV_0	0
#define	DPO_REV			DM_PLUGIN_OPS_REV_0

typedef struct {
	int		dpo_rev;

	/* dm plugin module name */
	char		*dpo_name;

	/* plugin initialization */
	void		(*dpo_init)();

	/* plugin de-initialization */
	void		(*dpo_fini)();

	/* create mapping */
	void		(*dpo_create)();

	/* destroy mapping */
	void		(*dpo_destroy)();

	/* map io */
	void		(*dpo_mapio)();

	/* update statistics */
	void		(*dpo_stats)();

} dm_plugin_ops_t;


#ifdef __cplusplus
}
#endif

#endif	/* SYS_DM_IMPL_H */
