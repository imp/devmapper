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
#include <sys/dm_ops.h>

static dm_state_t	dm_state = {
	.dip		= NULL,
};


/*
 * Device mapping plugin management
 *
 * device mapper is extendible via plugin framework
 * explain here more
 */

typedef struct {
	ddi_modhandle_t		pmod;
	dm_plugin_ops_t		*dmp_ops;
	int			refcnt;
} dm_plugin_entry_t;

typedef struct {
	dm_plugin_entry_t	**table;
	uint32_t		count;
	kmutex_t		lock;
} dm_plugin_table_t;

static dm_plugin_table_t	dm_plugin_table;

#define	DM_PLUGIN_MODNAMELEN	80

static void
dm_plugin_table_init(void)
{
	mutex_init(&dm_plugin_table.lock, NULL, MUTEX_DRIVER, NULL);
}

static void
dm_plugin_table_fini(void)
{
	mutex_destroy(&dm_plugin_table.lock);
}

static void
dm_plugin_add(dm_plugin_entry_t *plugin)
{
	ASSERT(plugin);

	mutex_enter(&dm_plugin_table.lock);
	
	mutex_exit(&dm_plugin_table.lock);
}

static void
dm_plugin_rem(dm_plugin_entry_t *plugin)
{
	ASSERT(plugin);

	mutex_enter(&dm_plugin_table.lock);

	mutex_exit(&dm_plugin_table.lock);
}

/* Lookup named plugin and return pointer to its entry if found */
static dm_plugin_entry_t *
dm_plugin_lookup(const char *name)
{
	dm_plugin_entry_t	**table;
	dm_plugin_entry_t	*plugin = NULL;

	mutex_enter(&dm_plugin_table.lock);

	table = dm_plugin_table.table;

	for (int i = 0; i < dm_plugin_table.count; i++) {
		if (strcmp(table[i]->dmp_ops->dpo_name, name) == 0) {
			plugin = table[i];
			break;
		}
	}

	mutex_exit(&dm_plugin_table.lock);

	return (plugin);
}

/*
 * Given the name load the plugin module.
 * The module name is constructed as 'misc/dm/dm_NAME', where NAME
 * is the dm_plugin_load() argument
 */
static dm_plugin_entry_t *
dm_plugin_load(const char *name)
{
	char			modname[DM_PLUGIN_MODNAMELEN];
	dm_plugin_entry_t	*plugin;
	int			error;

	(void) snprintf(modname, DM_PLUGIN_MODNAMELEN, "misc/dm/dm_%s", name);
	plugin = kmem_zalloc(sizeof (*plugin), KM_SLEEP);
	plugin->pmod = ddi_modopen(modname, KRTLD_MODE_FIRST, &error);
	if (error != 0) {
		kmem_free(plugin, sizeof (*plugin));
		return (NULL);
	}

	plugin->dmp_ops = ddi_modsym(plugin->pmod, "dm_ops", &error);
	if (plugin->dmp_ops == NULL) {
		ddi_modclose(plugin->pmod);
		kmem_free(plugin, sizeof (*plugin));
		return (NULL);
	}

	return (plugin);
}

/* Given the plugin entry unload the plugin module */
static int
dm_plugin_unload(dm_plugin_entry_t *plugin)
{
	ASSERT(plugin);

	ddi_modclose(plugin->pmod);
	kmem_free(plugin, sizeof (*plugin));

	return (DDI_SUCCESS);
}

/* Load and add plugin by name */
static int
dm_plugin_resgister(const char *name)
{
	dm_plugin_entry_t	*plugin;

	plugin = dm_plugin_load(name);
	dm_plugin_add(plugin);

	return (DDI_SUCCESS);
}

/* Lookup, remove and unload plugin by name */
static int
dm_plugin_unresgister(const char *name)
{
	dm_plugin_entry_t	*plugin;

	plugin = dm_plugin_lookup(name);
	dm_plugin_rem(plugin);
	dm_plugin_unload(plugin);

	return (DDI_SUCCESS);
}

/*
 * minor number management routines
 */
static void
dm_minor_init(dm_state_t *sp)
{
	/* Pre-init device table */
	sp->dm_minor_map = rmallocmap_wait(DM_MINOR_MAX);
	rmfree(sp->dm_minor_map, DM_MINOR_MAX, 1);
}

static void
dm_minor_fini(dm_state_t *sp)
{
	rmfreemap(sp->dm_minor_map);
}

static minor_t
dm_minor_alloc(dm_state_t *sp)
{
	return (rmalloc_wait(sp->dm_minor_map, 1));
}

static void
dm_minor_free(dm_state_t *sp, minor_t minor)
{
	rmfree(sp->dm_minor_map, 1, (ulong_t)minor);
}


static int
dm_info_init(dm_state_t *sp)
{
	return (ddi_soft_state_init(&sp->dm_infop, sizeof (dm_info_t), 0));
}

static void
dm_info_fini(dm_state_t *sp)
{
	ddi_soft_state_fini(&sp->dm_infop);
}

static dm_info_t *
dm_info_alloc(dm_state_t *sp, minor_t minor, const char *name, const char *dev)
{
	dm_info_t	*dmp = NULL;
	refstr_t	*rsname;
	refstr_t	*rsdev;

	/* Allocate new info structure */
	if (ddi_soft_state_zalloc(sp->dm_infop, (int)minor) == DDI_FAILURE) {
		return (NULL);
	}

	dmp = ddi_get_soft_state(sp->dm_infop, (int)minor);
	cmn_err(CE_CONT, "New dm_info (%d) allocated %p\n", minor, dmp);
	rsname = refstr_alloc(name);
	rsdev = refstr_alloc(dev);
	dmp->name = rsname;
	dmp->dev = rsdev;

	return (dmp);
}

static void
dm_info_free(dm_state_t *sp, minor_t minor)
{
	dm_info_t	*dmp;

	dmp = ddi_get_soft_state(sp->dm_infop, (int)minor);

	refstr_rele(dmp->name);
	refstr_rele(dmp->dev);

	ddi_soft_state_free(sp->dm_infop, (int)minor);
}

static dm_info_t *
dm_info_get(dm_state_t *sp, minor_t minor)
{
	return (ddi_get_soft_state(sp->dm_infop, (int)minor));
}

/* Loop through all the item in the dm_info state till the match found */
static minor_t
dm_name2minor(dm_state_t *sp, const char *name)
{
	minor_t	minor = 0;

	for (int i = 1; i < DM_MINOR_MAX; i++) {
		dm_info_t *dmp = ddi_get_soft_state(sp->dm_infop, i);

		if (dmp == NULL)
			continue;
		if (strncmp(refstr_value(dmp->name), name, MAXNAMELEN) == 0) {
			minor = (minor_t)i;
			break;
		}
	}
	return (minor);
}


static int
dm_create_minor_nodes(dm_state_t *sp, char *name, minor_t minor)
{
	char	*nodename;
	int	rc;

	nodename = kmem_alloc(MAXNAMELEN + 4, KM_SLEEP);

	(void) snprintf(nodename, MAXNAMELEN + 4, "%s,raw", name);
	rc = ddi_create_minor_node(sp->dip, nodename, S_IFCHR, minor,
	    DDI_NT_BLOCK, 0);

	(void) snprintf(nodename, MAXNAMELEN + 4, "%s,blk", name);
	rc = ddi_create_minor_node(sp->dip,  nodename, S_IFBLK, minor,
	    DDI_NT_BLOCK, 0);

	kmem_free(nodename, MAXNAMELEN + 4);

	return (DDI_SUCCESS);
}


static void
dm_remove_minor_nodes(dm_state_t *sp, char *name)
{
	char	*nodename;

	nodename = kmem_alloc(MAXNAMELEN + 4, KM_SLEEP);

	(void) snprintf(nodename, MAXNAMELEN + 4, "%s,raw", name);
	ddi_remove_minor_node(sp->dip, nodename);

	(void) snprintf(nodename, MAXNAMELEN + 4, "%s,blk", name);
	ddi_remove_minor_node(sp->dip, nodename);

	kmem_free(nodename, MAXNAMELEN + 4);
}


static int
dm_ioctl_dev(dev_t dev, int cmd, intptr_t arg, int mode, cred_t *crp, int *rvp)
{
	minor_t		minor;
	int		rc;
	dm_state_t	*sp = &dm_state;
	dm_info_t	*dmip;

	minor = getminor(dev);

	dmip = dm_info_get(sp, minor);

	if (dmip == NULL)
		return (ENXIO);

	return (EINVAL);
}

/*
 * Mapping manipulations
 */

static int
dm_list_mappings(dm_state_t *sp, intptr_t buf, int mode)
{
	dm_entry_t	*dmlist;
	int		rc;

	dmlist = kmem_zalloc(sizeof (dm_entry_t) * DM_MINOR_MAX, KM_SLEEP);

	for (int i = 1; i <= DM_MINOR_MAX; i++) {
		const char	*name = "";
		const char	*dev = "";
		dm_info_t	*dmip = dm_info_get(sp, (minor_t)i);

		if (dmip != NULL) {
			name = refstr_value(dmip->name);
			dev = refstr_value(dmip->dev);
			cmn_err(CE_CONT, "Found mapping %s (%d)\n", name, i);
		}

		(void) strncpy(dmlist[i - 1].name, name, MAXNAMELEN);
		(void) strncpy(dmlist[i - 1].dev, dev, MAXPATHLEN);
	}

	rc = ddi_copyout(dmlist, (void *)buf,
	    sizeof (dm_entry_t) * DM_MINOR_MAX, mode);
	kmem_free(dmlist, sizeof (dm_entry_t) * DM_MINOR_MAX);

	return ((rc == -1) ? (EFAULT) : (0));
}

/*
 * Allocate new mapping and attach it
 */ 
static int
dm_attach_mapping(dm_state_t *sp, char *name, char *dev, cred_t *crp)
{
	dm_info_t	*dmp;
	minor_t		minor;
	int		rc;

	cmn_err(CE_CONT, "Attaching new map %s (%s)\n", name, dev);

	minor = dm_minor_alloc(sp);

	/* Allocate new info structure */
	dmp = dm_info_alloc(sp, minor, name, dev);

	rc = ldi_open_by_name(dev, FREAD|FWRITE, crp, &dmp->lh, sp->li);
	if (rc != 0) {
		cmn_err(CE_WARN, "Failed to open device %s", dev);
		dm_info_free(sp, minor);
		return (rc);
	}

	rc = dm_create_minor_nodes(sp, name, minor);

	if (rc != DDI_SUCCESS) {
		dm_remove_minor_nodes(sp, name);
		ldi_close(dmp->lh, 0, NULL);
		dm_info_free(sp, minor);
		dm_minor_free(sp, minor);
		return (rc);
	}
	return (rc);
}

/*
 * Detach the existing mapping and free its resources
 */
static int
dm_detach_mapping(dm_state_t *sp, char *name)
{
	minor_t		minor;
	dm_info_t	*dmp = NULL;

	cmn_err(CE_CONT, "Detaching existing map %s\n", name);

	minor = dm_name2minor(sp, name);

	if (minor == 0) {
		return (EINVAL);
	}

	dmp = dm_info_get(sp, minor);

	cmn_err(CE_CONT, "Found %s info block\n", name);

	dm_remove_minor_nodes(sp, name);
	ldi_close(dmp->lh, 0, NULL);
	dm_info_free(sp, minor);
	dm_minor_free(sp, minor);

	return (0);
}


/*
 * Standard Solaris character driver entry points
 */
static int
dm_open(dev_t *devp, int flag, int otyp, cred_t *cp)
{
	minor_t		minor = getminor(*devp);
	dm_state_t	*sp = &dm_state;
	dm_info_t	*dmip;

	/* Control node */
	if (minor == 0) {
		/* Are we attached ? */
		if (sp->dip == NULL) {
			return (ENXIO);
		} else {
			return (0);
		}
	}

	dmip = dm_info_get(sp, minor);
	if (dmip == NULL)
		return (ENXIO);

	return (0);
}

static int
dm_close(dev_t dev, int flag, int otyp, cred_t *crp)
{
	minor_t		minor = getminor(dev);
	dm_state_t	*sp = &dm_state;
	dm_info_t	*dmip;

	/* Control node */
	if (minor == 0) {
		/* Are we attached ? */
		if (sp->dip == NULL) {
			return (ENXIO);
		} else {
			return (0);
		}
	}

	dmip = dm_info_get(sp, minor);
	if (dmip == NULL)
		return (ENXIO);

	return (0);
}

static int
dm_read(dev_t dev, struct uio *uiop, cred_t *crp)
{
	minor_t		minor = getminor(dev);
	dm_state_t	*sp = &dm_state;
	dm_info_t	*dmip;

	/* Control node doesn't support IO */
	if (minor == 0) {
		return (EIO);
	}

	dmip = dm_info_get(sp, minor);

	if (dmip == NULL) {
		return (ENXIO);
	}

	return (ldi_read(dmip->lh, uiop, crp));
}

static int
dm_write(dev_t dev, struct uio *uiop, cred_t *crp)
{
	minor_t		minor = getminor(dev);
	dm_state_t	*sp = &dm_state;
	dm_info_t	*dmip;

	/* Control node doesn't support IO */
	if (minor == 0) {
		return (EIO);
	}
	dmip = dm_info_get(sp, minor);

	if (dmip == NULL) {
		return (ENXIO);
	}

	return (ldi_write(dmip->lh, uiop, crp));
}

static int
dm_ioctl(dev_t dev, int cmd, intptr_t arg, int mode, cred_t *crp, int *rvp)
{
	minor_t		minor = getminor(dev);
	int		rc;
	dm_state_t	*sp = &dm_state;
	dm_entry_t	dm_entry;

	/* If this is not a control node handle it elsewhere */
	if (minor != 0) {
		return (dm_ioctl_dev(dev, cmd, arg, mode, crp, rvp));
	}

	if ((cmd == DM_ATTACH_MAPPING) || (cmd == DM_DETACH_MAPPING)) {
		rc = ddi_copyin((const void *)arg, &dm_entry,
		    sizeof (dm_entry_t), mode);
		if (rc == -1) {
			return (EFAULT);
		}
	}

	switch (cmd) {
	case DM_LIST_MAPPINGS:
		rc = dm_list_mappings(sp, arg, mode);
		break;
	case DM_ATTACH_MAPPING:
		rc = dm_attach_mapping(sp, dm_entry.name, dm_entry.dev, crp);
		rc = (rc == DDI_SUCCESS) ? 0 : EIO;
		break;
	case DM_DETACH_MAPPING:
		rc = dm_detach_mapping(sp, dm_entry.name);

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
	int		rc;
	dm_state_t	*sp = &dm_state;

	switch (cmd) {
	case DDI_INFO_DEVT2DEVINFO:
		*rp = sp->dip;
		rc = DDI_SUCCESS;
		break;
	case DDI_INFO_DEVT2INSTANCE:
		*rp = (void *)(uintptr_t)0;
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
	dm_state_t	*sp = &dm_state;
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

	sp->dip = dip;

	if (ldi_ident_from_dip(sp->dip, &sp->li) != 0) {
		cmn_err(CE_WARN, "dm_attach: failed to get LDI identification");
		return (DDI_FAILURE);
	}

	dm_minor_init(sp);
	dm_info_init(sp);

	if (ddi_create_minor_node(dip, "ctl", S_IFCHR,
	    instance, DDI_PSEUDO, 0) != DDI_SUCCESS) {
		cmn_err(CE_WARN, "dm_attach: failed to create minor node");
		dm_minor_fini(sp);
		ldi_ident_release(sp->li);
		return (DDI_FAILURE);
	}

	ddi_report_dev(dip);

	return (DDI_SUCCESS);
}

static int
dm_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	dm_state_t	*sp = &dm_state;
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

	ddi_remove_minor_node(dip, 0);

	dm_info_fini(sp);
	dm_minor_fini(sp);

	ldi_ident_release(sp->li);

	/* Mark us detached */
	sp->dip = NULL;

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
	return (mod_install(&dm_modlinkage));
}

int
_info(struct modinfo *mip)
{
	return (mod_info(&dm_modlinkage, mip));
}

int
_fini(void)
{
	return (mod_remove(&dm_modlinkage));
}
