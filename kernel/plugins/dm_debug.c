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


#include <sys/conf.h>
#include <sys/file.h>
#include <sys/modctl.h>
#include <sys/types.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#include <sys/dm.h>
#include <sys/dm_impl.h>
#include <sys/dm_ops.h>

static void
dm_debug_init(void)
{
	return;
}


static void
dm_debug_fini(void)
{
	return;
}


static void
dm_debug_create(void)
{
	return;
}


static void
dm_debug_destroy(void)
{
	return;
}


static void
dm_debug_mapio(void)
{
	return;
}


static void
dm_debug_stats(void)
{
	return;
}


static dm_plugin_ops_t dm_debug_ops = {
	.dpo_rev	= DPO_REV,
	.dpo_name	= "debug",
	.dpo_init	= dm_debug_init,
	.dpo_fini	= dm_debug_fini,
	.dpo_create	= dm_debug_create,
	.dpo_destroy	= dm_debug_destroy,
	.dpo_mapio	= dm_debug_mapio,
	.dpo_stats	= dm_debug_stats,
};

static struct modlmisc modlmisc = {
	.misc_modops	= &mod_miscops,
	.misc_linkinfo	= "Solaris Device Mapper debug plugin"
};

static struct modlinkage modlinkage = {
	.ml_rev		= MODREV_1,
	.ml_linkage	= {&modlmisc, NULL, NULL, NULL}
};

int
_init(void)
{
	return (mod_install(&modlinkage));
}

int
_info(struct modinfo *mip)
{
	return (mod_info(&modlinkage, mip));
}

int
_fini(void)
{
	return (mod_remove(&modlinkage));
}
