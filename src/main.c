/*
 * WMVolumeManager - Window Maker applet for managing hutpluggable volumes
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
#include <dockapp.h>

#include "ui.h"
#include "udisks.h"

int main(int argc, char *argv[])
{
	static char *dpyName = "";
	static char *theme = "default";
	static DAProgramOption op[] = {
		{"-d", "--display", "display to use", DOString, False, {&dpyName} },
		{"-t", "--theme", "icon theme", DOString, False, {&theme} }
	};

	DAParseArguments(argc, argv, op,
					 sizeof(op)/sizeof(DAProgramOption),
					 "",
					 PACKAGE_NAME " version " PACKAGE_VERSION);

	if (!wmvm_init_dockapp(dpyName, argc, argv, theme))
		return 1;

	if (!wmvm_do_udisks_init())
		return 1;

	wmvm_update_icon();

	wmvm_run_dockapp();

	return 0;
}
