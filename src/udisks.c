/*
 * udisks.c - Window Maker Volume Manager, UDisks integration
 *
 * Copyright (C) 2005,2010  Alexey I. Froloff <raorn@altlinux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "udisks.h"
#include "ui.h"

#include "udisks-daemon-glue.h"
#include "udisks-device-glue.h"

#include "udisks-marshal.h"

static DBusGConnection *dbus_bus = NULL;
static DBusGProxy *udisks_proxy = NULL;

static GHashTable *_get_device_props(DBusGProxy *proxy, const char *object_path)
{
	DBusGProxy *prop_proxy;
	GError *error;
	GHashTable *device_props;

	prop_proxy = dbus_g_proxy_new_for_name(dbus_bus, "org.freedesktop.UDisks", object_path, "org.freedesktop.DBus.Properties");
	error = NULL;
	device_props = NULL;
	if (!dbus_g_proxy_call(prop_proxy,
						   "GetAll",
						   &error,
						   G_TYPE_STRING,
						   "org.freedesktop.UDisks.Device",
						   G_TYPE_INVALID,
						   dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
						   &device_props,
						   G_TYPE_INVALID))
		g_message("Couldn't call GetAll() to get properties for %s: %s", object_path, error->message);

	g_object_unref(prop_proxy);
	return device_props;
}

static gboolean _device_should_display(GHashTable *device_props)
{
	GValue *val;

	/* Do not show system devices */
	if ((val = g_hash_table_lookup(device_props, "DeviceIsSystemInternal")) != NULL &&
		g_value_get_boolean(val) == TRUE)
		return FALSE;

	if ((val = g_hash_table_lookup(device_props, "DeviceIsOpticalDisc")) != NULL &&
		g_value_get_boolean(val) == TRUE) {
		/* Do not show removable devices without media */
		if ((val = g_hash_table_lookup(device_props, "DeviceIsMediaAvailable")) != NULL &&
			g_value_get_boolean(val) == FALSE)
			return FALSE;
	} else {
		/* Do not show non-removable devices without filesystem */
		if ((val = g_hash_table_lookup(device_props, "IdUsage")) != NULL &&
			strcmp(g_value_get_string(val), "filesystem") != 0)
			return FALSE;
	}

	return TRUE;
}

static gboolean _device_should_mount(GHashTable *device_props)
{
	GValue *val;

	/* Only mount devices with filesystem */
	if ((val = g_hash_table_lookup(device_props, "IdUsage")) != NULL &&
		strcmp(g_value_get_string(val), "filesystem") != 0)
		return FALSE;

	return TRUE;
}

static void udisks_device_changed(DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
	GHashTable *device_props;
	GValue *val;
	char *mountpoint;
	gboolean mounted;

	if ((device_props = _get_device_props(proxy, object_path)) == NULL)
		return;

	mounted = FALSE;
	if ((val = g_hash_table_lookup(device_props, "DeviceIsMounted")) != NULL)
		mounted = g_value_get_boolean(val);

	mountpoint = NULL;
	if (mounted == TRUE &&
		(val = g_hash_table_lookup(device_props, "DeviceMountPaths")) != NULL)
		mountpoint = ((gchar **) g_value_get_boxed(val))[0];

	wmvm_volume_set_mount_status(object_path, mountpoint, mounted);

	g_hash_table_unref(device_props);
	return;
}

static void udisks_device_job_changed(DBusGProxy *proxy,
									  const char *object_path,
									  gboolean job_in_progress,
									  const char *job_id,
									  guint32 job_initiated_by_uid,
									  gboolean job_is_cancellable,
									  double job_percentage,
									  gpointer user_data)
{
	wmvm_volume_set_busy(object_path, job_in_progress);
}

static void udisks_device_added(DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
	GHashTable *device_props;
	GValue *val;
	const char *device;
	int icon;
	gboolean mountable;
	gboolean busy;

	if ((device_props = _get_device_props(proxy, object_path)) == NULL)
		return;

	if (!_device_should_display(device_props))
		goto out;

	if ((val = g_hash_table_lookup(device_props, "DeviceFilePresentation")) == NULL ||
		(device = g_value_get_string(val)) == NULL)
		goto out;

	mountable = _device_should_mount(device_props);

	icon = WMVM_ICON_UNKNOWN;

	if ((val = g_hash_table_lookup(device_props, "DeviceIsOpticalDisc")) != NULL &&
		g_value_get_boolean(val) == TRUE) {
		if (mountable == FALSE &&
			(val = g_hash_table_lookup(device_props, "OpticalDiscNumAudioTracks")) != NULL &&
			g_value_get_uint(val) > 0) {
			icon = WMVM_ICON_CDAUDIO;
		} else {
			const char *disk_type = NULL;

			if ((val = g_hash_table_lookup(device_props, "DriveMedia")) != NULL)
				disk_type = g_value_get_string(val);

#define DISK_IS(t) disk_type != NULL && strcmp(disk_type, (t)) == 0
			if (DISK_IS("optical_cd")) {
				icon = WMVM_ICON_CDROM;
			} else if (DISK_IS("optical_cd_r")) {
				icon = WMVM_ICON_CDR;
			} else if (DISK_IS("optical_cd_rw")) {
				icon = WMVM_ICON_CDRW;
			} else if (DISK_IS("optical_dvd")) {
				icon = WMVM_ICON_DVDROM;
			} else if (DISK_IS("optical_dvd_r")) {
				icon = WMVM_ICON_DVDR;
			} else if (DISK_IS("optical_dvd_rw")) {
				icon = WMVM_ICON_DVDRW;
			} else if (DISK_IS("optical_dvd_ram")) {
				icon = WMVM_ICON_DVDRAM;
			} else if (DISK_IS("optical_dvd_plus_r")) {
				icon = WMVM_ICON_DVDPLUSR;
			} else if (DISK_IS("optical_dvd_plus_rw")) {
				icon = WMVM_ICON_DVDPLUSRW;
			} else if (DISK_IS("optical_dvd_plus_r_dl")) {
				icon = WMVM_ICON_DVDPLUSR;
			} else if (DISK_IS("optical_dvd_plus_rw_dl")) {
				icon = WMVM_ICON_DVDPLUSRW;
			} else if (DISK_IS("optical_bd")) {
				icon = WMVM_ICON_BD;
			} else if (DISK_IS("optical_bd_r")) {
				icon = WMVM_ICON_BDR;
			} else if (DISK_IS("optical_bd_re")) {
				icon = WMVM_ICON_BDRE;
			} else if (DISK_IS("optical_hddvd")) {
				icon = WMVM_ICON_HDDVD;
			} else if (DISK_IS("optical_hddvd_r")) {
				icon = WMVM_ICON_HDDVDR;
			} else if (DISK_IS("optical_hddvd_rw")) {
				icon = WMVM_ICON_HDDVDRW;
			}
#undef DISK_IS
		}
	} else {
		GHashTable *drive_props;
		const char *drive_path;
		const char *media_type;

		drive_path = NULL;
		if ((val = g_hash_table_lookup(device_props, "PartitionSlave")) != NULL) {
			drive_path = g_value_get_boxed(val);
		}

		if (drive_path == NULL ||
			(drive_props = _get_device_props(proxy, drive_path)) == NULL)
			drive_props = device_props;

		media_type = NULL;

		if ((val = g_hash_table_lookup(drive_props, "DriveMedia")) != NULL)
			media_type = g_value_get_string(val);

#define MEDIA_IS(t) media_type != NULL && strcmp(media_type, (t)) == 0
		if (MEDIA_IS("flash")) {
			icon = WMVM_ICON_CARD_CF;
		} else if (MEDIA_IS("flash_cf")) {
			icon = WMVM_ICON_CARD_CF;
		} else if (MEDIA_IS("flash_ms")) {
			icon = WMVM_ICON_CARD_MS;
		} else if (MEDIA_IS("flash_sm")) {
			icon = WMVM_ICON_CARD_SM;
		} else if (MEDIA_IS("flash_sd")) {
			icon = WMVM_ICON_CARD_SDMMC;
		} else if (MEDIA_IS("flash_sdhc")) {
			icon = WMVM_ICON_CARD_SDMMC;
		} else if (MEDIA_IS("flash_mmc")) {
			icon = WMVM_ICON_CARD_SDMMC;
		} else {
			const char *drive_iface;

			drive_iface = NULL;
			if ((val = g_hash_table_lookup(drive_props, "DriveConnectionInterface")) != NULL)
				drive_iface = g_value_get_string(val);

			if ((val = g_hash_table_lookup(drive_props, "DeviceIsRemovable")) != NULL) {
				if (g_value_get_boolean(val) == TRUE) {
					icon = WMVM_ICON_REMOVABLE;

					if (drive_iface != NULL && strcmp(drive_iface, "usb"))
						icon = WMVM_ICON_REMOVABLE_USB;
					else if (drive_iface != NULL && strcmp(drive_iface, "firewire"))
						icon = WMVM_ICON_REMOVABLE_1394;
				} else {
					icon = WMVM_ICON_HARDDISK;

					if (drive_iface != NULL && strcmp(drive_iface, "usb"))
						icon = WMVM_ICON_HARDDISK_USB;
					else if (drive_iface != NULL && strcmp(drive_iface, "firewire"))
						icon = WMVM_ICON_HARDDISK_1394;
				}
			}
		}
#undef MEDIA_IS

		if (drive_props != device_props)
			g_hash_table_unref(drive_props);
	}

	wmvm_add_volume(object_path, device, icon, mountable);
	udisks_device_changed(proxy, object_path, user_data);

	busy = FALSE;
	if ((val = g_hash_table_lookup(device_props, "JobInProgress")) != NULL)
		busy = g_value_get_boolean(val);

	wmvm_volume_set_busy(object_path, busy);

out:
	g_hash_table_unref(device_props);
	return;
}

static void udisks_device_removed(DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
	wmvm_remove_volume(object_path);
}

static void udisks_device_mount_cb(DBusGProxy *proxy, char * mountpath, GError *error, gpointer userdata)
{
	wmvm_volume_set_error(dbus_g_proxy_get_path(proxy), error != NULL);
	return;
}

void udisks_device_mount(const char *object_path)
{
	DBusGProxy *proxy;

	proxy = dbus_g_proxy_new_for_name (dbus_bus, "org.freedesktop.UDisks", object_path, "org.freedesktop.UDisks.Device");
	org_freedesktop_UDisks_Device_filesystem_mount_async(proxy, NULL, NULL, udisks_device_mount_cb, NULL);
}

static void udisks_device_umount_cb(DBusGProxy *proxy, GError *error, gpointer userdata)
{
	wmvm_volume_set_error(dbus_g_proxy_get_path(proxy), error != NULL);
	return;
}

void udisks_device_umount(const char *object_path)
{
	DBusGProxy *proxy;

	proxy = dbus_g_proxy_new_for_name (dbus_bus, "org.freedesktop.UDisks", object_path, "org.freedesktop.UDisks.Device");
	org_freedesktop_UDisks_Device_filesystem_unmount_async(proxy, NULL, udisks_device_umount_cb, NULL);
}

static gboolean reinit_udisks_connection(gpointer data);

static DBusHandlerResult signal_filter(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
		g_message("Disconnected from DBus");
		g_object_unref(udisks_proxy);
		udisks_proxy = NULL;
		dbus_g_connection_unref(dbus_bus);
		dbus_bus = NULL;
		g_timeout_add(1000, reinit_udisks_connection, NULL);
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean init_udisks_connection(void)
{
	GError *error;

	error = NULL;
	if (dbus_bus == NULL) {
		dbus_bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
		if (dbus_bus == NULL) {
			g_message("Error connecting to message bus: %s", error->message);
			g_error_free(error);
			return FALSE;
		}
		// faggots from #dbus may go screw themselves
		dbus_connection_set_exit_on_disconnect(dbus_g_connection_get_connection(dbus_bus), FALSE);
		dbus_connection_add_filter(dbus_g_connection_get_connection(dbus_bus), signal_filter, NULL, NULL);
	}

	if (udisks_proxy == NULL) {
		udisks_proxy = dbus_g_proxy_new_for_name(dbus_bus,
												  "org.freedesktop.UDisks",
												  "/org/freedesktop/UDisks",
												  "org.freedesktop.UDisks");

		dbus_g_proxy_add_signal(udisks_proxy, "DeviceAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
		dbus_g_proxy_add_signal(udisks_proxy, "DeviceRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
		dbus_g_proxy_add_signal(udisks_proxy, "DeviceChanged", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
		dbus_g_proxy_add_signal(udisks_proxy, "DeviceJobChanged", DBUS_TYPE_G_OBJECT_PATH,
																  G_TYPE_BOOLEAN,
																  G_TYPE_STRING,
																  G_TYPE_UINT,
																  G_TYPE_BOOLEAN,
																  G_TYPE_DOUBLE,
																  G_TYPE_INVALID);

		dbus_g_proxy_connect_signal(udisks_proxy, "DeviceAdded", G_CALLBACK(udisks_device_added), NULL, NULL);
		dbus_g_proxy_connect_signal(udisks_proxy, "DeviceRemoved", G_CALLBACK(udisks_device_removed), NULL, NULL);
		dbus_g_proxy_connect_signal(udisks_proxy, "DeviceChanged", G_CALLBACK(udisks_device_changed), NULL, NULL);
		dbus_g_proxy_connect_signal(udisks_proxy, "DeviceJobChanged", G_CALLBACK(udisks_device_job_changed), NULL, NULL);
	}

	return TRUE;
}

static gboolean reinit_udisks_connection(gpointer data)
{
	return !init_udisks_connection();
}

gboolean wmvm_do_udisks_init(void)
{
	GPtrArray *devices;
	GError *error;
	int n;

	g_type_init();

	dbus_g_object_register_marshaller (udisks_marshal_VOID__BOXED_BOOLEAN_STRING_UINT_BOOLEAN_DOUBLE,
									   G_TYPE_NONE,
									   DBUS_TYPE_G_OBJECT_PATH,
									   G_TYPE_BOOLEAN,
									   G_TYPE_STRING,
									   G_TYPE_UINT,
									   G_TYPE_BOOLEAN,
									   G_TYPE_DOUBLE,
									   G_TYPE_INVALID);

	if (!init_udisks_connection()) {
		return FALSE;
	}

	/* prime the list of devices */
	error = NULL;
	if (!org_freedesktop_UDisks_enumerate_devices(udisks_proxy, &devices, &error)) {
		g_message("Error enumerating devices: %s", error->message);
		g_error_free(error);
		return FALSE;
	}

	for (n = 0; n < (int) devices->len; n++)
		udisks_device_added(udisks_proxy, g_ptr_array_index(devices, n), NULL);
	g_ptr_array_foreach(devices, (GFunc) g_free, NULL);
	g_ptr_array_free(devices, TRUE);

	return TRUE;
}
