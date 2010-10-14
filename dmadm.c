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
#include <string.h>
#include <unistd.h>

#include <sys/dm.h>

#define	DMCTLNAME	"/dev/dmctl"

static int
dm_list(int dmctl, int argc, char **argv, const char *usage)
{
	dm_4k_t	names[DM_4K_TABLELEN];
	int	rc;

	rc = ioctl(dmctl, DM_4K_LIST, &names);
	if (rc != 0) {
		return (rc);
	}

	for (int i = 0; i < DM_4K_TABLELEN; i++) {
		if ((names[i].name) && (strlen(names[i].name) > 0)) {
			printf("%d - %s\t(%s)\n", i,
			    names[i].name, names[i].dev);
		}
	}
	return (rc);
}

static int
dm_version(int dmctl, int argc, char **argv, const char *usage)
{
	(void) printf("Solaris Device Mapper facility\n"
	    "\tPackage version\t\t%d\n"
	    "\tCLI ABI version\t\t%d\n"
	    "\tKernel ABI version\t%d\n",
	    DM_VERSION, DM_ABI_VERSION, DM_ABI_VERSION);
	return (0);
}

static int
dm_create(int dmctl, int argc, char **argv, const char *usage)
{
	dm_4k_t		map;
	int		rc;

	if (argc < 2) {
		(void) fprintf(stderr, usage);
		return (EXIT_FAILURE);
	}

	(void) strncpy(map.name, argv[0], MAXNAMELEN);
	(void) strncpy(map.dev, argv[1], MAXPATHLEN);
	rc = ioctl(dmctl, DM_4K_ATTACH, &map);

	return ((rc == -1) ? EXIT_FAILURE : EXIT_SUCCESS);
}

static int
dm_remove(int dmctl, int argc, char **argv, const char *usage)
{
	dm_4k_t		map;
	int		rc;

	if (argc < 1) {
		(void) fprintf(stderr, usage);
		return (EXIT_FAILURE);
	}

	(void) strncpy(map.name, argv[0], MAXNAMELEN);
	rc = ioctl(dmctl, DM_4K_DETACH, &map);

	return ((rc == -1) ? EXIT_FAILURE : EXIT_SUCCESS);
}

static int
dm_failure(int dmctl, int argc, char **argv, const char *usage)
{
	return (EXIT_FAILURE);
}

/* func(int dmctl, int argc, char **argv, const char *usage) */
typedef int func_t(int, int, char **, const char *);

typedef struct {
	char		*cmd;
	func_t		*func;
	const char	*usage;
} cmd_t;

static cmd_t commands[] = {
	{"version", dm_version, "version"},
	{"list", dm_list, "list [mapping]"},
	{"show", dm_list, "show <mapping>"},
	{"create", dm_create, "create <mapping>"},
	{"remove", dm_remove, "remove <mapping>"},
	{NULL, NULL, NULL}
};

static void
usage(char *prog)
{
	(void) fprintf(stderr, "usage: %s <subcommand> <args> ...\n", prog);

	for (int i = 0; commands[i].cmd != NULL; i++) {
		(void) fprintf(stderr, "\t%s\n", commands[i].usage);
	}
}

int
main(int argc, char **argv)
{
	int	dmctl;
	int	rc = EXIT_FAILURE;

	if (argc < 2) {
		usage(argv[0]);
		return (rc);
	}

	for (int i = 0; commands[i].cmd != NULL; i++) {

		if (strcmp(argv[1], commands[i].cmd) == 0) {
			/* Open the device mapper control node */
			if ((dmctl = open(DMCTLNAME, O_RDWR)) == -1) {
				perror("Failed to open DM control node");
				return (EXIT_FAILURE);
			}

			rc = commands[i].func(dmctl, argc - 2, &argv[2],
			    commands[i].usage);
			(void) close(dmctl);
			if (rc != 0) {
				(void) fprintf(stderr, "%s command failed\n",
				    commands[i].cmd);
			}
			return (rc);
		}
	}

	(void) fprintf(stderr, "%s: unknown subcommand '%s'\n",
	    argv[0], argv[1]);
	usage(argv[0]);
	return (rc);
}
