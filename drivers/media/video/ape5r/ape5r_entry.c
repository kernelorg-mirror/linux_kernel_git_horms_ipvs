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

#include "ape5r_entry.h"
#include <linux/init.h>
#include <linux/module.h>

static struct rtv4l2overlay_func func = {
/* memory */
	.system_memory_info_new      = system_memory_info_new,
	.system_memory_info_delete   = system_memory_info_delete,
	.system_memory_ap_share_area = system_memory_ap_share_area,
	.system_memory_ap_share_mem  = system_memory_ap_share_mem,
	.system_memory_ap_close      = system_memory_ap_close,
/* display */
	.screen_display_new          = screen_display_new,
	.screen_display_start_hdmi   = screen_display_start_hdmi,
	.screen_display_stop_hdmi    = screen_display_stop_hdmi,
	.screen_display_delete       = screen_display_delete,
/* overlay */
	.screen_overlay_new            = screen_overlay_new,
	.screen_overlay_initialize     = screen_overlay_initialize,
	.screen_overlay_set_parameters = screen_overlay_set_parameters,
	.screen_overlay_start_notify   = screen_overlay_start_notify,
	.screen_overlay_write_image    = screen_overlay_write_image,
	.screen_overlay_quit           = screen_overlay_quit,
	.screen_overlay_delete         = screen_overlay_delete,
};

struct rtv4l2overlay_func *ape5r_v4l2_get_rtapi_interface()
{
	return &func;
}

int ape5r_check_rtapi_interface(void)
{
	return (func.system_memory_info_new == NULL) || \
		(func.system_memory_info_delete == NULL) || \
		(func.system_memory_ap_share_area == NULL) || \
		(func.system_memory_ap_share_mem == NULL) || \
		(func.system_memory_ap_close == NULL) || \
		(func.screen_display_new == NULL) || \
		(func.screen_display_start_hdmi == NULL) || \
		(func.screen_display_stop_hdmi == NULL) || \
		(func.screen_display_delete == NULL) || \
		(func.screen_overlay_new == NULL) || \
		(func.screen_overlay_initialize == NULL) || \
		(func.screen_overlay_set_parameters == NULL) || \
		(func.screen_overlay_start_notify == NULL) || \
		(func.screen_overlay_write_image == NULL) || \
		(func.screen_overlay_quit == NULL) || \
		(func.screen_overlay_delete == NULL);
}


