/*
 * udisks.h - Window Maker Volume Manager, UDisks integration
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

#ifndef __WMVM_HAL_H__
#define __WMVM_HAL_H__

#include <glib.h>

gboolean wmvm_do_udisks_init(void);
void udisks_device_mount(const char *object_path);
void udisks_device_umount(const char *object_path);

#endif
