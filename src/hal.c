/*
 * hal.c - Window Maker Volume Manager, Hal integration
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301	 USA
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

static DBusConnection *dbus_connection = NULL;
static LibHalContext *hal_context = NULL;

static DBusHandlerResult signal_filter(DBusConnection *connection, DBusMessage *message, void *user_data);
static gboolean init_dbus_connection(void);
static gboolean init_hal_context(void);
static void deinit_hal_context(void);
static gboolean reinit_dbus_connection(gpointer data);

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
		icon = WMVM_ICON_CD_UNKNOWN;

		if (!mountable &&
			libhal_device_property_exists(ctx, udi, "volume.disc.has_audio", NULL) &&
			libhal_device_get_property_bool(ctx, udi, "volume.disc.has_audio", NULL)) {
			icon = WMVM_ICON_CDAUDIO;
		} else {
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
				else if (!strcmp(disc_type, "dvd_plus_r"))
					icon = WMVM_ICON_DVDPLUSR;
				else if (!strcmp(disc_type, "dvd_plus_rw"))
					icon = WMVM_ICON_DVDPLUSRW;

				libhal_free_string(disc_type);
			}
		}
	} else {
		char *drive_type = NULL;

		if (libhal_device_property_exists(ctx, storage_device, "storage.drive_type", NULL))
			drive_type = libhal_device_get_property_string(ctx, storage_device, "storage.drive_type", NULL);

		if (drive_type != NULL) {
			if (!strcmp(drive_type, "cdrom")) {
				/* cdrom, but does not have volume.is_disk property? */
				icon = WMVM_ICON_CD_UNKNOWN;
			} else if (!strcmp(drive_type, "disk")) {
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
			} /*else if (!strcmp(drive_type, "floppy")) {
			}*/ /*else if (!strcmp(drive_type, "tape")) {
			}*/ else if (!strcmp(drive_type, "compact_flash")) {
				icon = WMVM_ICON_CARD_CF;
			} else if (!strcmp(drive_type, "memory_stick")) {
				icon = WMVM_ICON_CARD_MS;
			} else if (!strcmp(drive_type, "sd_mmc")) {
				icon = WMVM_ICON_CARD_SDMMC;
			} else if (!strcmp(drive_type, "smart_media")) {
				icon = WMVM_ICON_CARD_SM;
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

static DBusHandlerResult signal_filter(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	/* Disconnected from DBus */
	if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
		dbg("disconnected from DBus\n");
		deinit_hal_context();
		dbus_connection_unref(dbus_connection);
		dbus_connection = NULL;
		g_timeout_add(1000, reinit_dbus_connection, NULL);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	/* Check for signal NameOwnerChange */
	if (dbus_message_is_signal(message, DBUS_INTERFACE_DBUS, "NameOwnerChanged")) {
		DBusError error;
		char *service;
		char *old_owner = NULL;
		char *new_owner = NULL;

		dbus_error_init(&error);
		if (dbus_message_get_args(message, &error,
								  DBUS_TYPE_STRING, &service,
								  DBUS_TYPE_STRING, &old_owner,
								  DBUS_TYPE_STRING, &new_owner,
								  DBUS_TYPE_INVALID)) {
			if (strcmp(service, "org.freedesktop.Hal") == 0) {
				dbg("NameOwnerChanged for org.freedesktop.Hal\n");
				gboolean old_owner_good, new_owner_good;

				old_owner_good = (old_owner != NULL && (strlen(old_owner) > 0));
				new_owner_good = (new_owner != NULL && (strlen(new_owner) > 0));

				if (!old_owner_good && new_owner_good) {
					dbg("Hal just appeared\n");
					init_hal_context();
					dbus_error_free(&error);
					return DBUS_HANDLER_RESULT_HANDLED;
				} else if (old_owner_good && !new_owner_good) {
					dbg("Hal just disappeared\n");
					deinit_hal_context();
					dbus_error_free(&error);
					return DBUS_HANDLER_RESULT_HANDLED;
				}
			}
		} else {
			warn("error geting NameOwner of org.freedesktop.Hal: %s\n", error.message);
			dbus_error_free(&error);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		dbus_error_free(&error);
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean init_dbus_connection(void)
{
	DBusError error;

	dbus_error_init(&error);
	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

	if ((dbus_connection == NULL) || dbus_error_is_set(&error)) {
		warn("can't get DBus connection: %s\n", error.message);
		dbus_error_free(&error);
		return FALSE;
	}

	dbus_connection_setup_with_g_main(dbus_connection, NULL);
	dbus_connection_set_exit_on_disconnect(dbus_connection, FALSE);
	dbus_connection_add_filter(dbus_connection, signal_filter, NULL, NULL);

	dbus_bus_add_match(dbus_connection,
					   "type='signal',"
					   "interface='" DBUS_INTERFACE_DBUS "',"
					   "sender='" DBUS_SERVICE_DBUS "',"
					   "member='NameOwnerChanged'",
					   &error);

	dbus_error_free(&error);
	return TRUE;
}

static gboolean init_hal_context(void)
{
	DBusError error;
	char **volumes;
	char *udi;
	int i;
	int nr;

	if (dbus_connection == NULL)
		return FALSE;

	if (hal_context != NULL)
		return TRUE;

	dbus_error_init(&error);
	if (!dbus_bus_name_has_owner(dbus_connection, "org.freedesktop.Hal", &error) || dbus_error_is_set(&error)) {
		warn("Hal is not running: %s\n", error.message);
		dbus_error_free(&error);
		return FALSE;
	}

	if ((hal_context = libhal_ctx_new()) == NULL) {
		warn("failed to allocate Hal context\n");
		dbus_error_free(&error);
		return FALSE;
	}

	libhal_ctx_set_dbus_connection(hal_context, dbus_connection);

	libhal_ctx_set_device_added(hal_context, hal_device_added);
	libhal_ctx_set_device_removed(hal_context, hal_device_removed);
	libhal_ctx_set_device_new_capability(hal_context, hal_device_new_capability);
	libhal_ctx_set_device_lost_capability(hal_context, hal_device_lost_capability);
	libhal_ctx_set_device_property_modified(hal_context, hal_property_modified);
	libhal_ctx_set_device_condition(hal_context, hal_device_condition);

	if (!libhal_ctx_init(hal_context, &error) || dbus_error_is_set(&error)) {
		warn("failed to initialize Hal context: %s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_free(hal_context);
		hal_context = NULL;
		return FALSE;
	}

	if (!libhal_device_property_watch_all(hal_context, &error) || dbus_error_is_set(&error)) {
		warn("failed to watch all Hal properties: %s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_shutdown(hal_context, NULL);
		libhal_ctx_free(hal_context);
		hal_context = NULL;
		return FALSE;
	}

	volumes = libhal_find_device_by_capability(hal_context, "volume", &nr, &error);
	if (dbus_error_is_set(&error)) {
		warn("could not find volume devices: %s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_shutdown(hal_context, NULL);
		libhal_ctx_free(hal_context);
		hal_context = NULL;
		return FALSE;
	}

	for (i = 0; i < nr; i++) {
		udi = volumes [i];

		hal_device_added(hal_context, udi);
		if (libhal_device_property_exists(hal_context, udi, "volume.is_mounted", NULL) &&
		    libhal_device_get_property_bool(hal_context, udi, "volume.is_mounted", NULL)) {
			hal_property_modified(hal_context, udi, "volume.is_mounted", FALSE, FALSE);
			hal_property_modified(hal_context, udi, "volume.mount_point", FALSE, FALSE);
		}
	}

	libhal_free_string_array(volumes);
	dbus_error_free(&error);
	return TRUE;
}

static void deinit_hal_context(void)
{
	wmvm_remove_all_volumes();

	if (hal_context != NULL) {
		/* Whenever I try to shutdown Hal context if DBus connection is dropped,
		 * I get SIGSEGV.  Check DBus connection before shutting down Hal context */
		if (dbus_connection != NULL && dbus_connection_get_is_connected(dbus_connection))
			libhal_ctx_shutdown(hal_context, NULL);
		libhal_ctx_free(hal_context);
		hal_context = NULL;
	}

	return;
}

static gboolean reinit_dbus_connection(gpointer data)
{
	dbg("reinit connection\n");
	if (dbus_connection == NULL) {
		dbg("... DBus\n");
		if (init_dbus_connection() != TRUE) {
			return TRUE;
		}
	}

	if (dbus_connection != NULL && hal_context == NULL) {
		dbg("... Hal\n");
		if (init_hal_context() != TRUE) {
			return TRUE;
		}
	}

	dbg("connection reinitialized\n");
	return FALSE;
}


gboolean wmvm_do_hal_init(void)
{
	if (!init_dbus_connection()) {
		return FALSE;
	}

	if (!init_hal_context()) {
		dbus_connection_unref(dbus_connection);
		dbus_connection = NULL;
		return FALSE;
	}

	return TRUE;
}

