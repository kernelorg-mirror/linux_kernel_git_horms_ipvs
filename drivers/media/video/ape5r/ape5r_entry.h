/*
 * Function        : V4L2 driver for APE5R
 *
 * Copyright (C) 2011 Renesas Electronics Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 */
#ifndef _APE5R_ENTRY
#define _APE5R_ENTRY

#include <rtapi/system_memory.h>
#include <rtapi/screen_display.h>
#include <rtapi/screen_overlay.h>

struct rtv4l2overlay_func {
/* memory */
	void* (*system_memory_info_new)(void);
	void  (*system_memory_info_delete)(   \
		system_mem_info_delete * info_delete);
	int   (*system_memory_ap_share_area)( \
		system_mem_ap_share_area * ap_share_area);
	int   (*system_memory_ap_share_mem)(  \
		system_mem_ap_share_mem * ap_share_mem);
	int   (*system_memory_ap_close)(  \
		system_mem_ap_close * ap_share_mem);

/* display */
	void* (*screen_display_new)(void);
	int   (*screen_display_start_hdmi)(  \
		screen_disp_start_hdmi * start_hdmi);
	int   (*screen_display_stop_hdmi)(   \
		screen_disp_stop_hdmi * stop_hdmi);
	void  (*screen_display_delete)(      \
		screen_disp_delete * disp_delete);
/* overlay */
	void* (*screen_overlay_new)(         \
		screen_ovl_new * ovl_new);
	int   (*screen_overlay_initialize)(  \
		screen_ovl_initialize * ovl_initialize);
	int   (*screen_overlay_set_parameters)(\
		screen_ovl_set_param * ovl_set_param);
	int   (*screen_overlay_start_notify)(  \
		screen_ovl_notify * ovl_notify);
	int   (*screen_overlay_write_image)(   \
		screen_ovl_write_image * ovl_write_image);
	int   (*screen_overlay_quit)(          \
		screen_ovl_quit * ovl_quit);
	void  (*screen_overlay_delete)(        \
		screen_ovl_delete * ovl_delete);
};

extern struct rtv4l2overlay_func *ape5r_v4l2_get_rtapi_interface(void);
extern int ape5r_check_rtapi_interface(void);

#endif
