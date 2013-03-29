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
#include <gio/gio.h>
#include <udisks/udisks.h>

#include "udisks.h"
#include "ui.h"

static UDisksClient *udisks_client = NULL;

static gboolean _monitor_has_name_owner(void)
{
	gchar *name_owner;
	gboolean ret;

	name_owner = g_dbus_object_manager_client_get_name_owner(G_DBUS_OBJECT_MANAGER_CLIENT(udisks_client_get_object_manager(udisks_client)));
	ret = (name_owner != NULL);
	g_free(name_owner);

	return ret;
}

static gboolean _device_should_display(UDisksBlock *block, UDisksDrive *drive)
{
	/* Do not show system devices */
	if (udisks_block_get_hint_system(block))
		return FALSE;

	/* Do not show ignored devices */
	if (udisks_block_get_hint_ignore(block))
		return FALSE;

	if (drive && udisks_drive_get_optical(drive)) {
		/* Do not show removable devices without media */
		if (!udisks_drive_get_media_available(drive))
			return FALSE;
	} else {
		/* Do not show devices without filesystem */
		if (g_strcmp0(udisks_block_get_id_usage(block), "filesystem") != 0)
			return FALSE;
	}

	return TRUE;
}

static gboolean _device_should_mount(UDisksBlock *block, UDisksDrive *drive)
{
	if (g_strcmp0(udisks_block_get_id_usage(block), "filesystem") != 0)
		return FALSE;

	return TRUE;
}

static void _update_object(GDBusObject *object, gboolean is_added)
{
	const gchar *object_path;
	UDisksBlock *block;
	UDisksFilesystem *filesystem;
	UDisksJob *job;

	object_path = g_dbus_object_get_object_path(object);

	if ((block = udisks_object_peek_block(UDISKS_OBJECT(object))) != NULL) {

		UDisksDrive *drive = NULL;
		const char *device;
		int icon;
		gboolean mountable;
		gboolean busy;

		if (!is_added) {
			wmvm_remove_volume(object_path);
			goto out_block;
		}

		drive = udisks_client_get_drive_for_block(udisks_client, block);

		if (!_device_should_display(block, drive)) {
			wmvm_remove_volume(object_path);
			goto out_block;
		}

		if ((device = udisks_block_get_device(block)) == NULL) {
			wmvm_remove_volume(object_path);
			goto out_block;
		}

		mountable = _device_should_mount(block, drive);

		icon = WMVM_ICON_UNKNOWN;

		if (drive) {
			const char *media = udisks_drive_get_media(drive);

			if (udisks_drive_get_optical(drive)) {
				if (mountable == FALSE && udisks_drive_get_optical_num_audio_tracks(drive) > 0) {
					icon = WMVM_ICON_CDAUDIO;
				} else {
#define DISK_IS(t) g_strcmp0(media, (t)) == 0
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
#define MEDIA_IS(t) g_strcmp0(media, (t)) == 0
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
					const char *drive_iface = udisks_drive_get_connection_bus(drive);

					if (udisks_drive_get_removable(drive)) {
						icon = WMVM_ICON_REMOVABLE;

						if (g_strcmp0(drive_iface, "usb") == 0)
							icon = WMVM_ICON_REMOVABLE_USB;
						else if (g_strcmp0(drive_iface, "ieee1394") == 0)
							icon = WMVM_ICON_REMOVABLE_1394;
						/*else if (g_strcmp0(drive_iface, "sdio") == 0)
						  icon = WMVM_ICON_REMOVABLE_SDIO;*/
					} else {
						icon = WMVM_ICON_HARDDISK;

						if (g_strcmp0(drive_iface, "usb") == 0)
							icon = WMVM_ICON_HARDDISK_USB;
						else if (g_strcmp0(drive_iface, "ieee1394") == 0)
							icon = WMVM_ICON_HARDDISK_1394;
						/*else if (g_strcmp0(drive_iface, "sdio") == 0)
						  icon = WMVM_ICON_HARDDISK_SDIO;*/
					}
				}
#undef MEDIA_IS
			}
		}

		wmvm_update_volume(object_path, device, icon, mountable);

		busy = FALSE;
		{
			GList *jobs = udisks_client_get_jobs_for_object(udisks_client, UDISKS_OBJECT(object));

			busy = g_list_length(jobs) > 0;

			g_list_foreach(jobs, (GFunc) g_object_unref, NULL);
			g_list_free(jobs);
		}

		wmvm_volume_set_busy(object_path, busy);

out_block:
		if (drive)
			g_object_unref(drive);
	}

	if ((job = udisks_object_peek_job(UDISKS_OBJECT(object))) != NULL) {
		const gchar *const *objects;

		for (objects = udisks_job_get_objects(job); objects && *objects; objects++)
			wmvm_volume_set_busy(*objects, is_added);
	}

	if ((filesystem = udisks_object_peek_filesystem(UDISKS_OBJECT(object))) != NULL) {
		const gchar *const *mountpoints = udisks_filesystem_get_mount_points(filesystem);

		if (mountpoints != NULL && *mountpoints != NULL) {
			wmvm_volume_set_mount_status(object_path, *mountpoints, TRUE);
		} else {
			wmvm_volume_set_mount_status(object_path, NULL, FALSE);
		}
	}

	/* if (( = udisks_object_peek_(object)) != NULL) {
	}*/

	return;
}

static void udisks_object_added(GDBusObjectManager *manager, GDBusObject *object, gpointer user_data)
{
	if (!_monitor_has_name_owner())
		return;

	_update_object(object, TRUE);
}

static void udisks_object_removed(GDBusObjectManager *manager, GDBusObject *object, gpointer user_data)
{
	if (!_monitor_has_name_owner())
		return;

	_update_object(object, FALSE);
}

static void udisks_interface_added(GDBusObjectManager *manager, GDBusObject *object, GDBusInterface *interface, gpointer user_data)
{
	if (!_monitor_has_name_owner())
		return;

	_update_object(object, TRUE);
}

static void udisks_interface_removed(GDBusObjectManager *manager, GDBusObject *object, GDBusInterface *interface, gpointer user_data)
{
	if (!_monitor_has_name_owner())
		return;

	_update_object(object, FALSE);
}

static void udisks_interface_proxy_properties_changed(GDBusObjectManagerClient *manager,
													  GDBusObjectProxy *object_proxy,
													  GDBusProxy *interface_proxy,
													  GVariant *changed_properties,
													  const gchar* const *invalidated_properties,
													  gpointer user_data)
{
	const gchar *object_path;
	UDisksObject *object;

	object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object_proxy));
	object = udisks_client_get_object(udisks_client, object_path);

	_update_object(G_DBUS_OBJECT(object), TRUE);

	g_object_unref(object);
}

static void udisks_device_mount_cb(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	GError *error;

	error = NULL;
	wmvm_volume_set_error(g_dbus_object_get_object_path(g_dbus_interface_get_object(G_DBUS_INTERFACE(source_object))),
						  !udisks_filesystem_call_mount_finish(UDISKS_FILESYSTEM(source_object), NULL, res, &error));

	return;
}

void udisks_device_mount(const char *object_path)
{
	UDisksObject *object;
	UDisksFilesystem *filesystem;
	GVariantBuilder builder;

	object = udisks_client_get_object(udisks_client, object_path);
	filesystem = udisks_object_peek_filesystem(UDISKS_OBJECT(object));
	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

	udisks_filesystem_call_mount(filesystem, g_variant_builder_end(&builder), NULL, udisks_device_mount_cb, NULL);
}

static void udisks_device_unmount_cb(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	GError *error;

	error = NULL;
	wmvm_volume_set_error(g_dbus_object_get_object_path(g_dbus_interface_get_object(G_DBUS_INTERFACE(source_object))),
						  !udisks_filesystem_call_unmount_finish(UDISKS_FILESYSTEM(source_object), res, &error));

	return;
}

void udisks_device_unmount(const char *object_path)
{
	UDisksObject *object;
	UDisksFilesystem *filesystem;
	GVariantBuilder builder;

	object = udisks_client_get_object(udisks_client, object_path);
	filesystem = udisks_object_peek_filesystem(UDISKS_OBJECT(object));
	g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

	udisks_filesystem_call_unmount(filesystem, g_variant_builder_end(&builder), NULL, udisks_device_unmount_cb, NULL);
}

static gboolean init_udisks_connection(void)
{
	GError *error;
	GDBusObjectManager *manager;

	error = NULL;
	if (udisks_client == NULL) {
		udisks_client = udisks_client_new_sync(NULL, &error);

		manager = udisks_client_get_object_manager(udisks_client);

		g_signal_connect(manager,
						 "object-added",
						 G_CALLBACK(udisks_object_added),
						 NULL);
		g_signal_connect(manager,
						 "object-removed",
						 G_CALLBACK(udisks_object_removed),
						 NULL);
		g_signal_connect(manager,
						 "interface-added",
						 G_CALLBACK(udisks_interface_added),
						 NULL);
		g_signal_connect(manager,
						 "interface-removed",
						 G_CALLBACK(udisks_interface_removed),
						 NULL);
		g_signal_connect(manager,
						 "interface-proxy-properties-changed",
						 G_CALLBACK(udisks_interface_proxy_properties_changed),
						 NULL);
	}

	return TRUE;
}

gboolean wmvm_do_udisks_init(void)
{
	GList *objects;

	g_type_init();

	if (!init_udisks_connection()) {
		return FALSE;
	}

	objects = g_dbus_object_manager_get_objects(udisks_client_get_object_manager(udisks_client));

	g_list_foreach(objects, (GFunc) _update_object, (gpointer) TRUE);
	g_list_foreach(objects, (GFunc) g_object_unref, NULL);
	g_list_free(objects);

	return TRUE;
}
