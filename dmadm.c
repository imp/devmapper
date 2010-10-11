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


#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/dm.h>

#define	DMCTLNAME	"/dev/dmctl"

static int
dm_list(int dmctl)
{
	char	names[MAXPATHLEN][DM_4K_TABLELEN];
	int	rc;

	rc = ioctl(dmctl, DM_4K_LIST, &names);
	if (rc == 0) {
		;
	}
	return (rc);
}

static int
dm_version(int dmctl)
{
	return (0);
}

int
main(int argc, char **argv)
{
	int	dmctl;

	dmctl = open(DMCTLNAME, O_RDWR);

	if (dmctl == 0) {
		printf("Failed to open DM control node\n");
		exit(EXIT_FAILURE);
	}

	(void) close(dmctl);
	return (0);
}
