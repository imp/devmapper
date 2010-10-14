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

#include <sys/dm.h>
#include <sys/dm_impl.h>

static void		*dm_statep;
static dm_info_t	dm_info[DM_INFO_TABLELEN];

static dm_info_t *
dm_info_alloc(dm_state_t *sp, const char *name, const char *dev)
{
	dm_info_t	*dmp = NULL;
	refstr_t	*rsname;
	refstr_t	*rsdev;

	rsname = refstr_alloc(name);
	rsdev = refstr_alloc(dev);

	/* Allocate new info structure */
	dmp = (dm_info_t *)rmalloc_wait(sp->dm_info_map, 1);
	dmp->name = rsname;
	dmp->dev = rsdev;

	return (dmp);
}

static void
dm_info_free(dm_state_t *sp, dm_info_t *dmp)
{
	refstr_rele(dmp->name);
	refstr_rele(dmp->dev);
	dmp->name = NULL;
	dmp->dev = NULL;
	rmfree(sp->dm_info_map, 1, (ulong_t)dmp);
}

static int
dm_attach_dev(dm_state_t *sp, const char *name, char *dev, cred_t *crp)
{
	dm_info_t	*dmp;
	int	rc;

	cmn_err(CE_CONT, "Attaching new map %s (%s)\n", name, dev);

	/* Allocate new info structure */
	dmp = dm_info_alloc(sp, name, dev);

	rc = ldi_open_by_name(dev, FREAD|FWRITE, crp, &dmp->lh, sp->li);
	if (rc != 0) {
		cmn_err(CE_WARN, "Failed to open device %s", dev);
		dm_info_free(sp, dmp);
		return (rc);
	}

	return (rc);
}

static int
dm_detach_dev(dm_state_t *sp, const char *mapname)
{
	dm_info_t	*dmp = NULL;
	int		i;
	int		rc;

	cmn_err(CE_CONT, "Detaching old map %s\n", mapname);

	/* Find the device in our info table */
	for (i = 0; (i < DM_INFO_TABLELEN) && (dmp == NULL); i++) {
		refstr_t *name = dm_info[i].name;

		if (name != NULL) {
			if (strncmp(refstr_value(name), mapname,
			    MAXNAMELEN) == 0) {
				dmp = dm_info + i;
			}
		}
	}

	if (dmp != NULL) {
		cmn_err(CE_CONT, "Found %s info block\n", mapname);
		ldi_close(dmp->lh, 0, NULL);
		dm_info_free(sp, dmp);
		rc = 0;
	} else {
		rc = EINVAL;
	}

	return (rc);
}

static int
dm_list(intptr_t buf, int mode)
{
	dm_entry_t		*dmlist;
	int		rc;

	dmlist = kmem_zalloc(sizeof (dm_entry_t) * DM_INFO_TABLELEN, KM_SLEEP);

	for (int i = 0; i < DM_INFO_TABLELEN; i++) {
		const char	*name = "";
		const char	*dev = "";

		if (dm_info[i].name != NULL) {
			name = refstr_value(dm_info[i].name);
			dev = refstr_value(dm_info[i].dev);
		}

		(void) strncpy(dmlist[i].name, name, MAXNAMELEN);
		(void) strncpy(dmlist[i].dev, dev, MAXPATHLEN);
	}

	rc = ddi_copyout(dmlist, (void *)buf,
	    sizeof (dm_entry_t) * DM_INFO_TABLELEN, mode);
	kmem_free(dmlist, sizeof (dm_entry_t) * DM_INFO_TABLELEN);

	return ((rc == -1) ? (EFAULT) : (0));
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
dm_close(dev_t dev, int flag, int otyp, cred_t *crp)
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
dm_read(dev_t dev, struct uio *uiop, cred_t *crp)
{
	return (0);
}

static int
dm_write(dev_t dev, struct uio *uiop, cred_t *crp)
{
	return (0);
}

static int
dm_ioctl(dev_t dev, int cmd, intptr_t arg, int mode, cred_t *crp, int *rvp)
{
	dm_state_t	*sp;
	dm_entry_t	dm_entry;
	int		instance;
	int		rc;

	instance = getminor(dev);

	sp = ddi_get_soft_state(dm_statep, instance);

	if (sp == NULL)
		return (ENXIO);

	if (instance != 0) {
		return (EINVAL);
	}

	if ((cmd == DM_ATTACH) || (cmd == DM_DETACH)) {
		rc = ddi_copyin((const void *)arg, &dm_entry,
		    sizeof (dm_entry_t), mode);
		if (rc == -1) {
			return (EFAULT);
		}
	}

	switch (cmd) {
	case DM_LIST:
		rc = dm_list(arg, mode);
		break;
	case DM_ATTACH:
		rc = dm_attach_dev(sp, dm_entry.name, dm_entry.dev, crp);
		rc = (rc == DDI_SUCCESS) ? 0 : EIO;
		break;
	case DM_DETACH:
		rc = dm_detach_dev(sp, dm_entry.name);

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
	.cb_read	= dm_read,
	.cb_write	= dm_write,
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
	for (int i = 0; i < DM_INFO_TABLELEN; i++) {
		dm_info[i].sp = sp;
		dm_info[i].target = i;
	}

	sp->dm_info_map = rmallocmap_wait(DM_INFO_TABLELEN);
	rmfree(sp->dm_info_map, DM_INFO_TABLELEN, (ulong_t)dm_info);

	if (ddi_create_minor_node(dip, "ctl", S_IFCHR,
	    instance, DDI_PSEUDO, 0) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "dm_attach: failed to create minor node");
		rmfreemap(sp->dm_info_map);
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

	rmfreemap(sp->dm_info_map);

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

	rc = mod_install(&dm_modlinkage);

	if (rc != 0) {
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

	ddi_soft_state_fini(&dm_statep);

	return (rc);
}
