/*
 * hal.h - Window Maker Volume Manager, HAL integration
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

#ifndef __WMVM_HAL_H__
#define __WMVM_HAL_H__

#include <glib.h>

gboolean wmvm_device_mount(char *device);
gboolean wmvm_device_umount(char *device);
gboolean wmvm_do_hal_init(void);

#endif
