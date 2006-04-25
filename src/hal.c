/*
 * hal.c - Window Maker Volume Manager, HAL integration
 *
 * Copyright (C) 2005 by Sir Raorn <raorn@altlinux.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>

#include "hal.h"
#include "ui.h"

#define WMVM_DEBUG
#ifdef WMVM_DEBUG
# define dbg(fmt,arg...) fprintf(stderr, "%s/%d: " fmt,__FILE__,__LINE__,##arg)
#else
# define dbg(fmt,arg...) do { } while(0)
#endif

#define warn(fmt,arg...) g_warning("%s/%d: " fmt,__FILE__,__LINE__,##arg)

gboolean wmvm_device_mount(char *device)
{
	char *argv[3];
	GError *error = NULL;

	argv[0] = BIN_MOUNT;
	argv[1] = device;
	argv[2] = NULL;

	if (!g_spawn_async(g_get_home_dir(),
						argv, NULL,
						G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL,
						NULL, NULL, NULL, &error)) {
		return FALSE;
	}

	return TRUE;
}

gboolean wmvm_device_umount(char *device)
{
	char *argv[3];
	GError *error = NULL;

	argv[0] = BIN_UMOUNT;
	argv[1] = device;
	argv[2] = NULL;

	if (!g_spawn_async(g_get_home_dir(),
						argv, NULL,
						G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL,
						NULL, NULL, NULL, &error)) {
		return FALSE;
	}

	return TRUE;
}

static dbus_bool_t hal_mainloop_integration(LibHalContext *ctx, DBusError *error)
{
	DBusConnection *dbus_connection;

	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, error);

	if (dbus_error_is_set(error))
		return FALSE;

	dbus_connection_setup_with_g_main(dbus_connection, NULL);

	libhal_ctx_set_dbus_connection(ctx, dbus_connection);

	return TRUE;
}

static void hal_device_added(LibHalContext *ctx, const char *udi)
{
	char *device = NULL, *storage_device = NULL, *storage_bus = NULL;
	char *mountpoint = NULL;
	int icon;
	gboolean removable, mountable;

	if (libhal_device_property_exists(ctx, udi, "volume.ignore", NULL) &&
		libhal_device_get_property_bool(ctx, udi, "volume.ignore", NULL))
		goto out;

	if (!libhal_device_property_exists(ctx, udi, "wmvolman.should_display", NULL) ||
		!libhal_device_get_property_bool(ctx, udi, "wmvolman.should_display", NULL))
		goto out;

	/* if it is a volume, it must have a device node */
	device = libhal_device_get_property_string(ctx, udi, "block.device", NULL);
	if (!device)
		goto out;

	/* get the backing storage device */
	storage_device = libhal_device_get_property_string(ctx, udi, "block.storage_device", NULL);
	if (!storage_device)
		goto out_dev;

	/*
	 * Does this device support removable media?  Note that we
	 * check storage_device and not our own UDI
	 */
	removable = libhal_device_get_property_bool(ctx, storage_device, "storage.removable", NULL);

	/* get the backing storage device */
	storage_bus = libhal_device_get_property_string(ctx, storage_device, "storage.bus", NULL);
	if (!storage_bus)
		goto out_stdev;

	mountable = (libhal_device_property_exists(ctx, udi, "wmvolman.should_mount", NULL) &&
				 libhal_device_get_property_bool(ctx, udi, "wmvolman.should_mount", NULL));

	icon = WMVM_ICON_UNKNOWN;

	if (libhal_device_get_property_bool(ctx, udi, "volume.is_disc", NULL)) {
		char *disc_type = NULL;

		if (libhal_device_property_exists(ctx, udi, "volume.disc.type", NULL))
			disc_type = libhal_device_get_property_string(ctx, udi, "volume.disc.type", NULL);

		if (disc_type != NULL) {
			if (!strcmp(disc_type, "cd_rom"))
				icon = WMVM_ICON_CDROM;
			else if (!strcmp(disc_type, "cd_r"))
				icon = WMVM_ICON_CDR;
			else if (!strcmp(disc_type, "cd_rw"))
				icon = WMVM_ICON_CDRW;
			else if (!strcmp(disc_type, "dvd_rom"))
				icon = WMVM_ICON_DVDROM;
			else if (!strcmp(disc_type, "dvd_ram"))
				icon = WMVM_ICON_DVDRAM;
			else if (!strcmp(disc_type, "dvd_r"))
				icon = WMVM_ICON_DVDR;
			else if (!strcmp(disc_type, "dvd_rw"))
				icon = WMVM_ICON_DVDRW;
			else if (!strcmp(disc_type, "dvd_plusr"))
				icon = WMVM_ICON_DVDPLUSR;
			else if (!strcmp(disc_type, "dvd_plusrw"))
				icon = WMVM_ICON_DVDPLUSRW;

			libhal_free_string(disc_type);
		}
	} else {
		char *drive_type = NULL;

		if (libhal_device_property_exists(ctx, storage_device, "storage.drive_type", NULL))
			drive_type = libhal_device_get_property_string(ctx, storage_device, "storage.drive_type", NULL);

		if (drive_type != NULL) {
			if (!strcmp(drive_type, "disk")) {
				if (removable) {
					icon = WMVM_ICON_REMOVABLE;

					if (!strcmp(storage_bus, "usb"))
						icon = WMVM_ICON_REMOVABLE_USB;
					else if (!strcmp(storage_bus, "ieee1394"))
						icon = WMVM_ICON_REMOVABLE_1394;
				} else {
					icon = WMVM_ICON_HARDDISK;

					if (!strcmp(storage_bus, "usb"))
						icon = WMVM_ICON_HARDDISK_USB;
					else if (!strcmp(storage_bus, "ieee1394"))
						icon = WMVM_ICON_HARDDISK_1394;
				}
			}

			libhal_free_string(drive_type);
		}
	}

	if (libhal_device_property_exists(ctx, udi, "volume.mount_point", NULL))
		mountpoint = libhal_device_get_property_string(ctx, udi, "volume.mount_point", NULL);

	/* folks, we have a new device! */
	dbg("Device added: udi=%s, dev=%s, icon=%d, %sMOUNTABLE\n", udi, device, icon, mountable ? "" : "NOT");

	wmvm_add_volume(udi, device, mountpoint, icon, mountable);

	if (mountpoint != NULL)
		libhal_free_string(mountpoint);

	libhal_free_string(storage_bus);
out_stdev:
	libhal_free_string(storage_device);
out_dev:
	libhal_free_string(device);
out:
	return;
}

static void hal_device_removed(LibHalContext *ctx, const char *udi)
{
	wmvm_remove_volume(udi);
}

static void hal_device_new_capability(LibHalContext *ctx, const char *udi, const char *capability)
{
	dbg("NewCapability(\"%s\", \"%s\")\n", udi, capability);
}

static void hal_device_lost_capability(LibHalContext *ctx, const char *udi, const char *capability)
{
	dbg("LostCapability(\"%s\", \"%s\")\n", udi, capability);
}

static void hal_property_modified(LibHalContext *ctx, const char *udi, const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
	if (!wmvm_is_managed_volume(udi))
		return;

	dbg("Property modified: %s, %s, %d, %d\n", udi, key, is_removed, is_added);

	if (!strcmp(key, "volume.is_mounted")) {
		wmvm_volume_set_mounted(udi, libhal_device_get_property_bool(ctx, udi, "volume.is_mounted", NULL));
	} else if (!strcmp(key, "volume.mount_point")) {
		char *mountpoint = libhal_device_get_property_string(ctx, udi, "volume.mount_point", NULL);

		wmvm_volume_set_mount_point(udi, mountpoint);

		if (mountpoint != NULL)
			libhal_free_string(mountpoint);
	}
}

static void hal_device_condition(LibHalContext *ctx, const char *udi, const char *condition_name, const char *condition_detail)
{
	dbg("Device condition: %s, %s, %s\n", udi, condition_name, condition_detail);
}


gboolean wmvm_do_hal_init(void)
{
	LibHalContext *ctx;
	DBusError error;
	char **devices;
	char **volumes;
	char *udi;
	int i;
	int nr;

	if (!(ctx = libhal_ctx_new())) {
		warn("failed to initialize HAL!\n");
		return FALSE;
	}

	dbus_error_init(&error);
	if (!hal_mainloop_integration(ctx, &error)) {
		warn("hal_initialize failed: %s\n", error.message);
		dbus_error_free(&error);
		return FALSE;
	}

	libhal_ctx_set_device_added(ctx, hal_device_added);
	libhal_ctx_set_device_removed(ctx, hal_device_removed);
	libhal_ctx_set_device_new_capability(ctx, hal_device_new_capability);
	libhal_ctx_set_device_lost_capability(ctx, hal_device_lost_capability);
	libhal_ctx_set_device_property_modified(ctx, hal_property_modified);
	libhal_ctx_set_device_condition(ctx, hal_device_condition);

	if (!libhal_device_property_watch_all(ctx, &error)) {
		warn("failed to watch all HAL properties!: %s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_free(ctx);
		return FALSE;
	}

	if (!libhal_ctx_init(ctx, &error)) {
		warn("hal_initialize failed: %s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_free(ctx);
		return FALSE;
	}


	/*
	 * Do something to ping the HAL daemon - the above functions will
	 * succeed even if hald is not running, so long as DBUS is.  But we
	 * want to exit silently if hald is not running, to behave on
	 * pre-2.6 systems.
	 */
	devices = libhal_get_all_devices(ctx, &nr, &error);
	if (!devices) {
		warn("seems that HAL is not running: %s\n", error.message);
		dbus_error_free(&error);

		libhal_ctx_shutdown(ctx, NULL);
		libhal_ctx_free(ctx);
		return FALSE;
	}
	libhal_free_string_array(devices);

	volumes = libhal_find_device_by_capability(ctx, "volume", &nr, &error);
	if (dbus_error_is_set(&error)) {
		warn("could not find volume devices: %s\n", error.message);
		dbus_error_free(&error);

		libhal_ctx_shutdown(ctx, NULL);
		libhal_ctx_free(ctx);
		return FALSE;
	}

	for (i = 0; i < nr; i++) {
		udi = volumes [i];

		hal_device_added(ctx, udi);
		if (libhal_device_property_exists(ctx, udi, "volume.is_mounted", NULL) &&
		    libhal_device_get_property_bool(ctx, udi, "volume.is_mounted", NULL)) {
			hal_property_modified(ctx, udi, "volume.is_mounted", FALSE, FALSE);
			hal_property_modified(ctx, udi, "volume.mount_point", FALSE, FALSE);
		}
	}

	libhal_free_string_array(volumes);
	return TRUE;
}

