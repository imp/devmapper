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
 * Copyright 2010 Grigale Ltd.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/types.h>
#include <libdevinfo.h>
#include <regex.h>
#include <devfsadm.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/mkdev.h>
#include <sys/dm.h>


/*
 * For the master device:
 *	/dev/dmctl -> /devices/pseudo/dm@0:ctl
 *
 * For each other device
 *	/dev/mapper/dsk/1 -> /devices/pseudo/dm@0:1
 *	/dev/mapper/rdsk/1 -> /devices/pseudo/dm@0:1,raw
 */
static int
dm(di_minor_t minor, di_node_t node)
{
	dev_t	dev;
	char mn[MAXNAMELEN + 1];
	char blkname[MAXNAMELEN + 1];
	char rawname[MAXNAMELEN + 1];
	char path[PATH_MAX + 1];

	(void) strcpy(mn, di_minor_name(minor));

	if (strcmp(mn, "ctl") == 0) {
		(void) devfsadm_mklink(DM_CTL_NAME, node, minor, 0);
	} else {
		dev = di_minor_devt(minor);
		(void) snprintf(blkname, sizeof (blkname), "%d",
		    (int)minor(dev));
		(void) snprintf(rawname, sizeof (rawname), "%d,raw",
		    (int)minor(dev));

		if (strcmp(mn, blkname) == 0) {
			(void) snprintf(path, sizeof (path), "%s/%s",
			    DM_BLOCK_NAME, blkname);
		} else if (strcmp(mn, rawname) == 0) {
			(void) snprintf(path, sizeof (path), "%s/%s",
			    DM_CHAR_NAME, blkname);
		} else {
			return (DEVFSADM_CONTINUE);
		}

		(void) devfsadm_mklink(path, node, minor, 0);
	}
	return (DEVFSADM_CONTINUE);
}

/*
 * Wrapper around devfsadm_rm_all() that allows termination of remove
 * process
 */
static int
dm_rm_all(char *link)
{
	devfsadm_rm_all(link);
	return (DEVFSADM_TERMINATE);
}
/*
 * devfs create callback register
 */
static devfsadm_create_t dm_create_cbt[] = {
	{ "pseudo", "ddi_pseudo", DM_DRIVER_NAME,
	    TYPE_EXACT | DRV_EXACT, ILEVEL_0, dm,
	},
};
DEVFSADM_CREATE_INIT_V0(dm_create_cbt);

/*
 * devfs cleanup register
 */
static devfsadm_remove_V1_t dm_remove_cbt[] = {
	{"pseudo", "^mapper/r?dsk/[0-9]+$", RM_ALWAYS | RM_PRE | RM_HOT,
	    ILEVEL_0, dm_rm_all},
};
DEVFSADM_REMOVE_INIT_V1(dm_remove_cbt);
