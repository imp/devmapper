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


#include <sys/blkdev.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/devops.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/map.h>
#include <sys/modctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "dm.h"

static void		*dm_statep;
static dm_4k_info_t	dm_4k_info[DM_4K_TABLELEN];

#define DM4K_BLKSIZE	4096

static void
dm_bd_driveinfo(void *prv, bd_drive_t *bdp)
{
	dm_4k_info_t	*dmp = (dm_4k_info_t *)prv;

	bdp->d_qsize = 32;
	bdp->d_maxxfer = 1024 * 1024; /* 1 MB - Fix me later */
	bdp->d_removable = B_FALSE;
	bdp->d_hotpluggable = B_FALSE;
	bdp->d_target = dmp->target;
}

static int
dm_bd_mediainfo(void *prv, bd_media_t *bmp)
{
	dm_4k_info_t	*dmp = (dm_4k_info_t *)prv;
	uint64_t	size;

	if (ldi_get_size(dmp->lh, &size) == DDI_FAILURE) {
		return (3);
	}

	bmp->m_blksize = DM4K_BLKSIZE;
	bmp->m_nblks = size / DM4K_BLKSIZE;
	bmp->m_readonly = B_FALSE;

	return (0);
}

static int
dm_devid_init(void *prv, dev_info_t *dip, ddi_devid_t *didp)
{
	return (0);
}

static int
dm_sync_cache(void *prv, bd_xfer_t *bxp)
{
	return (0);
}

static int
dm_bd_read(void *prv, bd_xfer_t *bxp)
{
	return (0);
}

static int
dm_bd_write(void *prv, bd_xfer_t *bxp)
{
	return (0);
}

#ifdef DUMP_SUPPORT
static int
dm_bd_dump(void *sp, bd_xfer_t *bxp)
{
	return (0);
}
#endif

static bd_ops_t bd_ops = {
	.o_version	= BD_OPS_VERSION_0,
	.o_drive_info	= dm_bd_driveinfo,
	.o_media_info	= dm_bd_mediainfo,
	.o_devid_init	= dm_devid_init,
	.o_sync_cache	= dm_sync_cache,
	.o_read		= dm_bd_read,
	.o_write	= dm_bd_write,
	.o_dump		= NULL,
};

static dm_4k_info_t *
dm_4k_info_alloc(dm_state_t *sp, const char *dev)
{
	dm_4k_info_t	*dmp = NULL;
	refstr_t	*rsdev;

	rsdev = refstr_alloc(dev);

	if (rsdev != NULL) {
		/* Allocate new info structure */
		dmp = (dm_4k_info_t *)rmalloc_wait(sp->dm4kmap, 1);
		dmp->dev = rsdev;
	}

	return (dmp);
}

static void
dm_4k_info_free(dm_state_t *sp, dm_4k_info_t *dmp)
{
	refstr_rele(dmp->dev);
	dmp->dev = NULL;
	rmfree(sp->dm4kmap, 1, (ulong)dmp);
}

static int
dm_attach_dev(dm_state_t *sp, char *dev, cred_t *crp)
{
	dm_4k_info_t	*dmp;
	int	rc;

	cmn_err(CE_CONT, "Attaching new dev %s\n", dev);

	/* Allocate new info structure */
	dmp = dm_4k_info_alloc(sp, dev);

	rc = ldi_open_by_name(dev, FREAD|FWRITE, crp, &dmp->lh, sp->li);
	if (rc != 0) {
		cmn_err(CE_WARN, "Failed to open device %s", dev);
		dm_4k_info_free(sp, dmp);
		return (rc);
	}

	dmp->bdh = bd_alloc_handle(dmp, &bd_ops, NULL, KM_SLEEP); /* XXX Fix DMA */

	ASSERT(dmp->bdh);

	bd_attach_handle(sp->dip, dmp->bdh);

	return (0);
}

static int
dm_detach_dev(dm_state_t *sp, const char *dev)
{
	dm_4k_info_t	*dmp = NULL;
	int		i;
	int		rc;

	cmn_err(CE_CONT, "Detaching old dev %s\n", dev);

	/* Find the device in our info table */
	for (i = 0; (i < DM_4K_TABLELEN) && (dmp == NULL); i++) {
		refstr_t *adev = dm_4k_info[i].dev;

		if (adev != NULL) {
			if (strncmp(refstr_value(adev), dev, MAXPATHLEN) == 0) {
				dmp = dm_4k_info + i;
			}
		}
	}

	if (dmp != NULL) {
		cmn_err(CE_CONT, "Found %s info block\n", dev);
		bd_detach_handle(dmp->bdh);
		bd_free_handle(dmp->bdh);
		ldi_close(dmp->lh, 0, NULL);
		dm_4k_info_free(sp, dmp);
		rc = 0;
	} else {
		rc = EINVAL;
	}

	return (rc);
}

static int
dm_list(intptr_t buf, int mode)
{
	for (int i = 0; i < DM_4K_TABLELEN; i++) {
		const char	*dev = "";
		void		*ubuf = (void *)(buf + i * MAXPATHLEN);

		if (dm_4k_info[i].dev != NULL) {
			dev = refstr_value(dm_4k_info[i].dev);
		}

		if (ddi_copyout(dev, ubuf, strlen(dev) + 1, mode) == -1) {
			return (EFAULT);
		}
	}

	return (0);
}

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
dm_ioctl(dev_t dev, int cmd, intptr_t arg, int mode, cred_t *crp, int *rvp)
{
	dm_state_t	*sp;
	dm_4k_info_t	dmp;
	dm_4k_t		dm_4k;
	int		instance;
	int		rc;

	instance = getminor(dev);

	sp = ddi_get_soft_state(dm_statep, instance);

	if (sp == NULL)
		return (ENXIO);

	if (instance != 0) {
		return (EINVAL);
	}

	if ((cmd == DM_4K_ATTACH) || (cmd == DM_4K_DETACH)) {
		rc = ddi_copyin((const void *)arg, &dm_4k, sizeof (dm_4k_t), mode);
		if (rc == -1) {
			return (EFAULT);
		}
	}

	switch (cmd) {
	case DM_4K_LIST:
		rc = dm_list(arg, mode);
		break;
	case DM_4K_ATTACH:
		rc = dm_attach_dev(sp, dm_4k.dev, crp);
		break;
	case DM_4K_DETACH:
		rc = dm_detach_dev(sp, dm_4k.dev);

		break;
	default:
		rc = EINVAL;
	}

	return (rc);
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

	if (ddi_soft_state_zalloc(dm_statep, instance) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "dm_attach: ddi_soft_state_zalloc() failed");
		return (DDI_FAILURE);
	}

	sp = ddi_get_soft_state(dm_statep, instance);

	sp->dip = dip;

	if (ldi_ident_from_dip(sp->dip, &sp->li) != 0) {
		cmn_err(CE_WARN, "dm_attach: failed to get LDI identification");
		ddi_soft_state_free(dm_statep, instance);
		return (DDI_FAILURE);
	}

	/* Pre-init device table */
	for (int i = 0; i < DM_4K_TABLELEN; i++) {
		dm_4k_info[i].sp = sp;
		dm_4k_info[i].target = i;
	}

	sp->dm4kmap = rmallocmap_wait(DM_4K_TABLELEN);
	rmfree(sp->dm4kmap, DM_4K_TABLELEN, (ulong)dm_4k_info);

	if (ddi_create_minor_node(dip, "ctl", S_IFCHR,
	    instance, DDI_PSEUDO, 0) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "dm_attach: failed to create minor node");
		rmfreemap(sp->dm4kmap);
		ldi_ident_release(sp->li);
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

	rmfreemap(sp->dm4kmap);

	ldi_ident_release(sp->li);

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

	if (rc != 0) {
		bd_mod_fini(&dm_dev_ops);
		ddi_soft_state_fini(&dm_statep);
	}

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

	if (rc != 0) {
		return (rc);
	}

	bd_mod_fini(&dm_dev_ops);
	ddi_soft_state_fini(&dm_statep);

	return (rc);
}
