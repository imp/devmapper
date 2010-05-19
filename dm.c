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


#include <sys/conf.h>
#include <sys/devops.h>
#include <sys/modctl.h>
#include <sys/blkdev.h>
#include <sys/stat.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "dm.h"

static void *dm_statep;

static int
dm_open(dev_t *devp, int flag, int otyp, cred_t *cp)
{
	int		instance;
	dm_state_t	*sp;

	instance = getminor(*devp);

	sp = ddi_get_soft_state(dm_statep, instance);

	if (sp == NULL)
		return (ENXIO);

	if (instance == 0) {
		/* That is a control node */
		return (0);
	} else {
		return (EINVAL);
	}
}

static int
dm_close(dev_t dev, int flag, int otyp, cred_t *cp)
{
	int		instance;
	dm_state_t	*sp;

	instance = getminor(dev);

	sp = ddi_get_soft_state(dm_statep, instance);

	if (sp == NULL)
		return (ENXIO);

	if (instance == 0) {
		return (0);
	} else {
		return (0);
	}
}

static int
dm_ioctl(dev_t dev, int cmd, intptr_t arg, int mode, cred_t *cp, int *rvp)
{
	int		instance;
	dm_state_t	*sp;

	instance = getminor(dev);

	sp = ddi_get_soft_state(dm_statep, instance);

	if (sp == NULL)
		return (ENXIO);

	if (instance == 0) {
		return (0);
	} else {
		return (EINVAL);
	}
}

static struct cb_ops dm_cb_ops = {
	.cb_open	= dm_open,
	.cb_close	= dm_close,
	.cb_strategy	= nodev,
	.cb_print	= nodev,
	.cb_dump	= nodev,
	.cb_read	= nodev,
	.cb_write	= nodev,
	.cb_ioctl	= dm_ioctl,
	.cb_devmap	= nodev,
	.cb_mmap	= nodev,
	.cb_segmap	= nodev,
	.cb_chpoll	= nochpoll,
	.cb_prop_op	= ddi_prop_op,
	.cb_str		= NULL,
	.cb_flag	= D_NEW | D_MP | D_64BIT,
	.cb_rev		= CB_REV,
	.cb_aread	= nodev,
	.cb_awrite	= nodev
};

static int
dm_getinfo(dev_info_t *dip, ddi_info_cmd_t cmd, void *arg, void **rp)
{
	int		instance;
	dm_state_t	*sp;
	int		rc;

	instance = getminor((dev_t)arg);

	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		sp = ddi_get_soft_state(dm_statep, instance);
		if (sp != NULL) {
			*rp = sp->dip;
			rc = DDI_SUCCESS;
		} else {
			*rp = NULL;
			rc = DDI_FAILURE;
		}
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*rp = (void *)(uintptr_t)instance;
		rc = DDI_SUCCESS;
		break;
	default:
		rc = DDI_FAILURE;
	}

	return (rc);
}

static int
dm_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	dm_state_t	*sp;
	int		instance;

	instance = ddi_get_instance(dip);

	ASSERT(instance == 0);

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
	default:
		return (DDI_FAILURE);
	}

	if (ddi_soft_state_zalloc(dm_statep, instance) != DDI_SUCCESS)
		return (DDI_FAILURE);

	sp = ddi_get_soft_state(dm_statep, instance);

	sp->dip = dip;

	if (ddi_create_minor_node(dip, "ctl", S_IFCHR,
	    instance, DDI_PSEUDO, 0) != DDI_SUCCESS) {
		ddi_soft_state_free(dm_statep, instance);
		return (DDI_FAILURE);
	}

	ddi_report_dev(dip);

	return (DDI_SUCCESS);
}

static int
dm_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	dm_state_t	*sp;
	int		instance;

	instance = ddi_get_instance(dip);

	VERIFY(instance == 0);

	switch (cmd) {
	case DDI_DETACH:
		break;
	case DDI_SUSPEND:
	default:
		return (DDI_FAILURE);
	}

	sp = ddi_get_soft_state(dm_statep, instance);

	ddi_remove_minor_node(dip, 0);

	ddi_soft_state_free(dm_statep, instance);

	return (DDI_SUCCESS);
}

static struct dev_ops dm_dev_ops = {
	.devo_rev	= DEVO_REV,
	.devo_refcnt	= 0,
	.devo_getinfo	= dm_getinfo,
	.devo_identify	= nulldev,
	.devo_probe	= nulldev,
	.devo_attach	= dm_attach,
	.devo_detach	= dm_detach,
	.devo_reset	= nodev,
	.devo_cb_ops	= &dm_cb_ops,
	.devo_bus_ops	= NULL,
	.devo_power	= NULL,
	.devo_quiesce	= ddi_quiesce_not_supported
};

static struct modldrv dm_modldrv = {
	.drv_modops	= &mod_driverops,
	.drv_linkinfo	= "Solaris Device Mapper v0",
	.drv_dev_ops	= &dm_dev_ops
};

static struct modlinkage dm_modlinkage = {
	.ml_rev		= MODREV_1,
	.ml_linkage	= {&dm_modldrv, NULL, NULL, NULL}
};

int
_init(void)
{
	int rc;

	rc = ddi_soft_state_init(&dm_statep, sizeof (dm_state_t), 0);
	if (rc != 0)
		return (rc);

	bd_mod_init(&dm_dev_ops);
	rc = mod_install(&dm_modlinkage);

	if (rc != 0)
		bd_mod_fini(&dm_dev_ops);
		ddi_soft_state_fini(&dm_statep);

	return (rc);
}

int
_info(struct modinfo *mip)
{
	return (mod_info(&dm_modlinkage, mip));
}

int
_fini(void)
{
	int rc;

	rc = mod_remove(&dm_modlinkage);

	if (rc != 0)
		return (rc);

	bd_mod_fini(&dm_dev_ops);
	ddi_soft_state_fini(&dm_statep);

	return (rc);
}
