/*
 * ui.h - Window Maker Volume Manager, user interface
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

#ifndef __WMVM_UI_H__
#define __WMVM_UI_H__

enum WMVMIconName {
	WMVM_ICON_UNKNOWN = 0,
	WMVM_ICON_CD_UNKNOWN,
	WMVM_ICON_CDROM,
	WMVM_ICON_CDAUDIO,
	WMVM_ICON_CDR,
	WMVM_ICON_CDRW,
	WMVM_ICON_DVDROM,
	WMVM_ICON_DVDRAM,
	WMVM_ICON_DVDR,
	WMVM_ICON_DVDRW,
	WMVM_ICON_DVDPLUSR,
	WMVM_ICON_DVDPLUSRW,
	WMVM_ICON_BD,
	WMVM_ICON_BDR,
	WMVM_ICON_BDRE,
	WMVM_ICON_HDDVD,
	WMVM_ICON_HDDVDR,
	WMVM_ICON_HDDVDRW,
	WMVM_ICON_HARDDISK,
	WMVM_ICON_HARDDISK_USB,
	WMVM_ICON_HARDDISK_1394,
	WMVM_ICON_REMOVABLE,
	WMVM_ICON_REMOVABLE_USB,
	WMVM_ICON_REMOVABLE_1394,
	WMVM_ICON_CARD_CF,
	WMVM_ICON_CARD_MS,
	WMVM_ICON_CARD_SDMMC,
	WMVM_ICON_CARD_SM,
	WMVM_ICON_MAX
};

void wmvm_update_icon(void);
gboolean wmvm_is_managed_volume(const char *udi);
void wmvm_add_volume(const char *udi, const char *device, int icon, gboolean mountable);
void wmvm_remove_volume(const char *udi);
void wmvm_remove_all_volumes(void);
void wmvm_volume_set_mount_status(const char *udi, const char *mountpoint, gboolean mounted);
void wmvm_volume_set_busy(const char *udi, gboolean busy);
void wmvm_volume_set_error(const char *udi, gboolean error);

gboolean wmvm_init_dockapp(char *dpyName, int argc, char *argv[], char *theme);

void wmvm_run_dockapp(void);

#endif
