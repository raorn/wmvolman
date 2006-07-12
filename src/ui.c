/*
 * ui.c - Window Maker Volume Manager, user interface
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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <dockapp.h>

#include <time.h>

#include "ui.h"
#include "hal.h"

#include "wmvolman-master.xpm"
#include "wmvolman-buttons.xpm"

typedef struct _WMVMSource {
	GSource source;
	GPollFD poll_fd;
} WMVMSource;

static GMainLoop *loop;

static Window iconWin;

static DAShapedPixmap *master, *buttons, *icon_none;
static DAShapedPixmap *wmvm_device_icons[WMVM_ICON_MAX];

static struct WMVMDeviceIconDesc {
	char *name;
	enum WMVMIconName fallback;
} wmvm_device_icon_names[WMVM_ICON_MAX] = {
	{"unknown.xpm", -1},                         /* WMVM_ICON_UNKNOWN = 0, */
	{"cdrom-unknown.xpm", WMVM_ICON_UNKNOWN},    /* WMVM_ICON_CD_UNKNOWN, */
	{"cdrom.xpm", WMVM_ICON_CD_UNKNOWN},         /* WMVM_ICON_CDROM, */
	{"disc-audio.xpm", WMVM_ICON_CDROM},        /* WMVM_ICON_CDAUDIO, */
	{"disc-cdr.xpm", WMVM_ICON_CDROM},           /* WMVM_ICON_CDR, */
	{"disc-cdrw.xpm", WMVM_ICON_CDROM},          /* WMVM_ICON_CDRW, */
	{"disc-dvdrom.xpm", WMVM_ICON_CDROM},        /* WMVM_ICON_DVDROM, */
	{"disc-dvdram.xpm", WMVM_ICON_DVDROM},       /* WMVM_ICON_DVDRAM, */
	{"disc-dvdr.xpm", WMVM_ICON_DVDROM},         /* WMVM_ICON_DVDR, */
	{"disc-dvdrw.xpm", WMVM_ICON_DVDROM},        /* WMVM_ICON_DVDRW, */
	{"disc-dvdr-plus.xpm", WMVM_ICON_DVDROM},    /* WMVM_ICON_DVDPLUSR, */
	{"disc-dvdrw-plus.xpm", WMVM_ICON_DVDROM},   /* WMVM_ICON_DVDPLUSRW, */
	{"removable.xpm", WMVM_ICON_UNKNOWN},        /* WMVM_ICON_REMOVABLE, */
	{"removable-usb.xpm", WMVM_ICON_REMOVABLE},  /* WMVM_ICON_REMOVABLE_USB, */
	{"removable-1394.xpm", WMVM_ICON_REMOVABLE}, /* WMVM_ICON_REMOVABLE_1394, */
	{"harddisk.xpm", WMVM_ICON_UNKNOWN},         /* WMVM_ICON_HARDDISK, */
	{"harddisk-usb.xpm", WMVM_ICON_HARDDISK},    /* WMVM_ICON_HARDDISK_USB, */
	{"harddisk-1394.xpm", WMVM_ICON_HARDDISK}    /* WMVM_ICON_HARDDISK_1394, */
};

typedef struct _WMVMVolume {
	char *hal_udi;
	char *device;
	char *mount_point;
	char *display_name;
	gboolean mountable;
	DAShapedPixmap *icon;
	gboolean mounted;
} WMVMVolume;

static GList *wmvm_volumes = NULL;
static WMVMVolume *current = NULL;

static DARect icon_area = { 22, 18, 36, 24 };

#define MAX_POS	8
static int cpos = 0, dpos = 1, tpause = 2;

typedef struct _WMVMButton {
	DARect r;
	int state;
	void (*action)(void);
} WMVMButton;

static int pressed = -1;

static inline int IN_RECT(int __x, int __y, DARect *__r)
{
	return !((__x < __r->x) ||
			(__x > __r->x + __r->width) ||
			(__y < __r->y) ||
			(__y > __r->y + __r->height));
}

#define BUTT_MOUNT	0
#define BUTT_LEFT	1
#define BUTT_RIGHT	2

#define STATE_NORMAL	0
#define STATE_DOWN		1
#define STATE_DISABLED	2

static void wmvm_mountumount(void);
static void wmvm_list_left(void);
static void wmvm_list_right(void);

static WMVMButton wmvm_buttons[] = {
	{{  5, 48, 28, 11 }, STATE_NORMAL, wmvm_mountumount},
	{{ 33, 48, 13, 11 }, STATE_NORMAL, wmvm_list_left},
	{{ 46, 48, 13, 11 }, STATE_NORMAL, wmvm_list_right}
};

static gboolean wmvm_event_prepare(GSource *src, gint *tm)
{
	*tm = -1;
	if (XPending(DADisplay))
		return TRUE;

	return FALSE;
}

static gboolean wmvm_event_check(GSource *src)
{
	if (XPending(DADisplay))
		return TRUE;

	return FALSE;
}

static gboolean wmvm_event_dispatch(GSource *src, GSourceFunc cb, gpointer data)
{
	XEvent evt;

	while (XPending(DADisplay)) {
		XNextEvent(DADisplay, &evt);
		DAProcessEvent(&evt);
	}

	return TRUE;
}

static void wmvm_draw_button(int b)
{
	if(b == -1)
		return;

	DASPCopyArea(buttons, master,
				 wmvm_buttons[b].r.x - 5, wmvm_buttons[b].r.height * wmvm_buttons[b].state,
				 wmvm_buttons[b].r.width, wmvm_buttons[b].r.height,
				 wmvm_buttons[b].r.x,     wmvm_buttons[b].r.y);
}

static void wmvm_refresh_window()
{
	DASPSetPixmap(master);
}

static void wmvm_draw_char(char c, int pos)
{
	char *p;
	static char *syms = "0123456789 -.\'()*/_";
	int fromx, fromy;

	if (pos >= MAX_POS)
		return;

	if (c >= 'a' && c <= 'z') {
		fromx = (c - 'a')*6 + 1;
		fromy = 41;
	} else if (c >= 'A' && c <= 'Z') {
		fromx = (c - 'A')*6 + 1;
		fromy = 41;
	} else if ((p = strchr(syms, c)) != NULL) {
		fromx = (p - syms)*6 + 1;
		fromy = 51;
	} else {
		fromx = 115;
		fromy = 51;
	}

	DASPCopyArea(buttons, master,
				 fromx, fromy, 5, 7, 8 + pos*6, 8);
}

static void wmvm_draw_string(char *str)
{
	int i;
	char *p;

	if (str && strlen(str) > cpos) {
		for (i = 0, p = str + cpos; i < MAX_POS && *p; i++, p++)
			wmvm_draw_char(*p, i);
		for (; i < MAX_POS; i++)
			wmvm_draw_char(' ', i);
	} else {
		for (i = 0; i < MAX_POS; i++)
			wmvm_draw_char(' ', i);
	}
}

static gboolean wmvm_timeout(gpointer data)
{
	if (tpause == 0 && current && current->display_name && strlen(current->display_name) > MAX_POS) {
		cpos += dpos;
		if (cpos <= 0 || cpos >= strlen(current->display_name) - MAX_POS) {
			cpos = (cpos <= 0) ? 0 : (strlen(current->display_name) - MAX_POS);
			dpos *= -1;
			tpause = 2;
		}
		wmvm_draw_string(current->display_name);
		wmvm_refresh_window();
		return TRUE;
	}
	if (tpause > 0)
		tpause--;
	return TRUE;
}

static void wmvm_update_button_state(WMVMVolume *vol)
{
	GList *c;

	if (vol != NULL && vol == current && (c = g_list_find(wmvm_volumes, vol)) != NULL) {
		if (!vol->mountable) {
			wmvm_buttons[BUTT_MOUNT].state = STATE_DISABLED;
		} else if (wmvm_buttons[BUTT_MOUNT].state == STATE_DISABLED) {
			wmvm_buttons[BUTT_MOUNT].state = STATE_NORMAL;
		}
		if (g_list_previous(c) == NULL) {
			wmvm_buttons[BUTT_LEFT].state = STATE_DISABLED;
		} else if (wmvm_buttons[BUTT_LEFT].state == STATE_DISABLED) {
			wmvm_buttons[BUTT_LEFT].state = STATE_NORMAL;
		}
		if (g_list_next(c) == NULL) {
			wmvm_buttons[BUTT_RIGHT].state = STATE_DISABLED;
		} else if (wmvm_buttons[BUTT_RIGHT].state == STATE_DISABLED) {
			wmvm_buttons[BUTT_RIGHT].state = STATE_NORMAL;
		}
	}
}

void wmvm_update_icon(void)
{
	int i;

	if (current != NULL) {
		/* buttons */
		/*wmvm_update_button_state(current);*/

		if (pressed != -1 && wmvm_buttons[pressed].state == STATE_DISABLED)
			pressed = -1;

		if (current->mounted) {
			DASPCopyArea(buttons, buttons, 54, 0, 28, 33, 0, 0);
		} else {
			DASPCopyArea(buttons, buttons, 82, 0, 28, 33, 0, 0);
		}

		wmvm_draw_button(BUTT_MOUNT);
		wmvm_draw_button(BUTT_LEFT);
		wmvm_draw_button(BUTT_RIGHT);

		if (current->icon)
			DASPSetPixmapForWindow(iconWin, current->icon);
		else
			DASPSetPixmapForWindow(iconWin, icon_none);
		/* text */
		wmvm_draw_string(current->display_name);
	} else {
		pressed = -1;
		for (i = 0; i < 3; i++) {
			wmvm_buttons[i].state = STATE_DISABLED;
			wmvm_draw_button(i);
		}
		DASPSetPixmapForWindow(iconWin, icon_none);
		for (i = 0; i < MAX_POS; i++)
			wmvm_draw_char(' ', i);
	}

	wmvm_refresh_window();
}

static void wmvm_set_current(WMVMVolume *newcur)
{
	gboolean needs_update = current != newcur;

	current = newcur;
	if (needs_update)
	{
		cpos = 0;
		dpos = 1;
		tpause = 2;
		pressed = -1;
		wmvm_update_button_state(current);
		wmvm_update_icon();
	}
}

static void wmvm_mountumount(void)
{
	if (current != NULL && current->device != NULL) {
		if (current->mounted)
			wmvm_device_umount(current->device, current->mount_point);
		else
			wmvm_device_mount(current->hal_udi, current->device);
	}
	wmvm_draw_button(pressed);
	wmvm_refresh_window();
}

static void wmvm_list_left(void)
{
	GList *c;

	if (current != NULL && (c = g_list_find(wmvm_volumes, current)) != NULL)
		if (g_list_previous(c) != NULL)
			wmvm_set_current(g_list_previous(c)->data);
}

static void wmvm_list_right(void)
{
	GList *c;

	if (current != NULL && (c = g_list_find(wmvm_volumes, current)) != NULL)
		if (g_list_next(c) != NULL)
			wmvm_set_current(g_list_next(c)->data);
}

static void da_button_press(int button, int state, int x, int y)
{
	int i;

	switch(button){
	case 1:
		pressed = -1;
		for (i = 0; i < sizeof(wmvm_buttons)/sizeof(wmvm_buttons[0]); i++)
			if (wmvm_buttons[i].state != STATE_DISABLED && IN_RECT(x, y, &(wmvm_buttons[i].r)))
				pressed = i;

		if (pressed != -1) {
			wmvm_buttons[pressed].state = STATE_DOWN;
			wmvm_draw_button(pressed);
			wmvm_refresh_window();
		}
		break;
	case 4:
		if (IN_RECT(x, y, &icon_area)) {
			wmvm_list_left();
		}
		break;
	case 5:
		if (IN_RECT(x, y, &icon_area)) {
			wmvm_list_right();
		}
		break;
	default:
		break;
	}
}

static void da_button_release(int button, int state, int x, int y)
{
	int i, p = -1;

	if(pressed == -1)
		return;

	for (i = 0; i < sizeof(wmvm_buttons)/sizeof(wmvm_buttons[0]); i++)
		if (IN_RECT(x, y, &(wmvm_buttons[i].r)))
			p = i;

	if(wmvm_buttons[pressed].state != STATE_DOWN)
		return;

	wmvm_buttons[pressed].state = STATE_NORMAL;

	if (pressed == p && wmvm_buttons[pressed].action)
		(*wmvm_buttons[pressed].action)();
	else
		wmvm_update_icon();

	pressed = -1;
}

static void wmvm_set_title(WMVMVolume *vol)
{
	gboolean needs_update = (current == vol);

	if (vol->mount_point && *vol->mount_point) {
		needs_update = needs_update && (vol->display_name != vol->mount_point);
		vol->display_name = vol->mount_point;
	} else {
		needs_update = needs_update && (vol->display_name != vol->device);
		vol->display_name = vol->device;
	}

	if (needs_update)
	{
		cpos = 0;
		dpos = 1;
		tpause = 2;
		wmvm_draw_string(current->display_name);
	}
}

static WMVMVolume *wmvm_find_volume(const char *udi)
{
	WMVMVolume *ret = NULL;
	GList *i;

	if (udi == NULL)
		return NULL;

	for (i = g_list_first(wmvm_volumes); i != NULL; i = g_list_next(i)) {
		ret = i->data;
		if (ret && strcmp(ret->hal_udi, udi) == 0)
			break;
	}

	return (i == NULL ? NULL : ret);
}

gboolean wmvm_is_managed_volume(const char *udi)
{
	return (wmvm_find_volume(udi) != NULL);
}

void wmvm_add_volume(const char *udi, const char *device, const char *mountpoint, int icon, gboolean mountable)
{
	WMVMVolume *vol;

	if (udi == NULL || device == NULL)
		return;

	vol = calloc(1, sizeof(WMVMVolume));
	if (vol == NULL)
		return;


	vol->hal_udi = strdup(udi);
	vol->device = strdup(device);
	if (mountpoint)
		vol->mount_point = strdup(mountpoint);
	vol->mountable = mountable;
	if (icon >= WMVM_ICON_UNKNOWN && icon < WMVM_ICON_MAX)
		vol->icon = wmvm_device_icons[icon];
	else
		vol->icon = wmvm_device_icons[WMVM_ICON_UNKNOWN];

	wmvm_set_title(vol);

	wmvm_volumes = g_list_append(wmvm_volumes, vol);

	if (pressed == -1 || current == NULL) {
		wmvm_update_button_state(vol);
		wmvm_set_current(vol);
	} else {
		wmvm_update_button_state(current);
		wmvm_update_icon();
	}

	return;
}

void wmvm_remove_volume(const char *udi)
{
	WMVMVolume *vol;

	if ((vol = wmvm_find_volume(udi)) == NULL)
		return;


	if (current == vol) {
		GList *c = g_list_find(wmvm_volumes, vol);

		if (g_list_previous(c))
			wmvm_set_current(g_list_previous(c)->data);
		else if (g_list_next(c))
			wmvm_set_current(g_list_next(c)->data);
		else
			wmvm_set_current(NULL);
	}

	wmvm_volumes = g_list_remove(wmvm_volumes, vol);
	if (vol->hal_udi) free(vol->hal_udi);
	if (vol->device) free(vol->device);
	if (vol->mount_point) free(vol->mount_point);
	free(vol);

	wmvm_update_button_state(current);
	wmvm_update_icon();
}

void wmvm_volume_set_mounted(const char *udi, gboolean mounted)
{
	WMVMVolume *vol;

	if ((vol = wmvm_find_volume(udi)) == NULL)
		return;


	vol->mounted = mounted;

	wmvm_update_button_state(vol);
	wmvm_update_icon();
}

void wmvm_volume_set_mount_point(const char *udi, const char *mountpoint)
{
	WMVMVolume *vol;

	if ((vol = wmvm_find_volume(udi)) == NULL)
		return;


	if (vol->mount_point) free(vol->mount_point);
	vol->mount_point = NULL;
	if (mountpoint)
		vol->mount_point = strdup(mountpoint);

	wmvm_set_title(vol);
	wmvm_refresh_window();
}

static void wmvm_init_icons(char *theme)
{
#include "icon_none.xpm"
	int i;
	gchar *usericondir = NULL;
	gchar *globalicondir = WMVM_ICONS_DIR;
	const gchar *home = g_getenv("HOME");

	icon_none = DAMakeShapedPixmapFromData(icon_none_xpm);

	if (NULL == theme || 0 == theme[0])
		theme = "default";

	if (home && *home)
		usericondir = g_build_filename(home, ".wmvolman", NULL);

	for (i = WMVM_ICON_UNKNOWN; i < WMVM_ICON_MAX; i++) {
		int j;
		DAShapedPixmap *pix = NULL;
		gchar *file = NULL;
		gchar *p;

		wmvm_device_icons[i] = icon_none;

		for (p = usericondir, j = 2; j; p = globalicondir, j--) {
			if (p && *p) {
				file = g_build_filename(p, theme,  wmvm_device_icon_names[i].name, NULL);
				if (file && *file && g_file_test(file, G_FILE_TEST_EXISTS)) {
					pix = DAMakeShapedPixmapFromFile(file);
					g_free(file);
					file = NULL;
					break;
				}
				if (file) {
					g_free(file);
					file = NULL;
				}
			}
		}
		wmvm_device_icons[i] = (pix == NULL && wmvm_device_icon_names[i].fallback != -1 ) ? wmvm_device_icons[wmvm_device_icon_names[i].fallback] : pix;
	}

	if (usericondir) g_free(usericondir);
}

gboolean wmvm_init_dockapp(char *dpyName, int argc, char *argv[], char *theme)
{
	WMVMSource *wmvm_source;
	static GSourceFuncs event_funcs = {
		wmvm_event_prepare,
		wmvm_event_check,
		wmvm_event_dispatch,
		NULL
	};
	static DACallbacks cb = {
		NULL,				/* destroy */
		da_button_press,	/* buttonPress */
		da_button_release,	/* buttonRelease */
		NULL,				/* motion */
		NULL,				/* enter */
		NULL,				/* leave */
		NULL				/* timeout */
	};

	DAInitialize(dpyName, "WMVolMan", 64, 64, argc, argv);
	DASetCallbacks(&cb);

	if ((master = DAMakeShapedPixmapFromData(wmvolman_master_xpm)) == NULL)
		return FALSE;
	if ((buttons = DAMakeShapedPixmapFromData(wmvolman_buttons_xpm)) == NULL)
		return FALSE;

	wmvm_init_icons(theme);

	DASPSetPixmap(master);

	iconWin = XCreateSimpleWindow(DADisplay, DAWindow, 22, 18, 36, 24, 0, 0, 0);
	DASPSetPixmapForWindow(iconWin, icon_none);

	if ((wmvm_source = (WMVMSource *) g_source_new(&event_funcs, sizeof(WMVMSource))) == NULL)
		return FALSE;

	loop = g_main_loop_new(NULL, FALSE);

	wmvm_source->poll_fd.fd = ConnectionNumber(DADisplay);
	wmvm_source->poll_fd.events = G_IO_IN;
	g_source_add_poll((GSource *) wmvm_source, &wmvm_source->poll_fd);

	g_source_attach((GSource *)wmvm_source, NULL);
	g_source_unref((GSource *)wmvm_source);

	g_timeout_add(250, wmvm_timeout, NULL);

	DAShow();

	return TRUE;
}

void wmvm_run_dockapp(void)
{
	g_main_loop_run(loop);
}
