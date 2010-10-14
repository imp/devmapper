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
 * Device Mapper control ABI definitions
 */

#include <sys/types.h>
#include <sys/param.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	DM_DRIVER_NAME	"dm"

#define	DM_VERSION	0
#define	DM_ABI_VERSION	0

#define	DM_INFO_TABLELEN	1024

#define	DM_LIST		1024
#define	DM_ATTACH	1025
#define	DM_DETACH	1026

typedef struct {
	char		name[MAXNAMELEN];
	char		dev[MAXPATHLEN];
	uint64_t	flags;
} dm_entry_t;

#ifdef __cplusplus
}
#endif

#endif	/* SYS_DM_H */
