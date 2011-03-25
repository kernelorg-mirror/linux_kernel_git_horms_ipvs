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

#include <linux/videodev2.h>
#include <linux/semaphore.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>

#include <media/v4l2-common.h>

#include <mach/hardware.h>
#include <media/v4l2-ioctl.h>

#include "ape5r_overlay.h"
#include "ape5r_v4l2_disp.h"
#include "ape5r_entry.h"

#undef  DEV_NAME
#define DEV_NAME "v4l2-disp"

#define _HDMI_DBG   0 /* debug disp */         /* 0: disable  1: enable */


#define printk_err(fmt, arg...) \
	do { \
		printk(KERN_ERR DEV_NAME ": %s: " fmt, __func__, ## arg); \
	} while (0)

#define printk_dbg(level, fmt, arg...) \
	do { \
		if (level > 0) \
			printk(KERN_INFO DEV_NAME ": %s: " fmt, \
				__func__, ## arg); \
	} while (0)




/*===============================================================*/
/* define structure                                              */
/*===============================================================*/

struct HDMIstate {
	unsigned int format;
	unsigned int bg_color;
	int          active;
};

static int                init_HDMIstate;
static struct semaphore   sem_hdmi;
static struct HDMIstate   state_hdmi[2];

static void               *handleHDMI;

static int check_interface(void)
{
	return !ape5r_check_rtapi_interface();
};


static void initialize_HDMIstate(void)
{
	int i;

	if (init_HDMIstate) {
		/* already initialized. */
		return;
	}
	printk_dbg(_HDMI_DBG, "initialize\n");

	handleHDMI = NULL;
	sema_init(&sem_hdmi, 1);
	for (i = 0; i < 2; i++) {
		state_hdmi[i].active   = 0;
		state_hdmi[i].format   = RT_DISPLAY_720_480P60;
		state_hdmi[i].bg_color = 0x000000;
	}
	init_HDMIstate = 1;
}

static int procedure_stop(int resource)
{
	int ret = 0;

	if (!state_hdmi[resource].active) {
		/* nothing to do */
		return ret;
	}

	if     (!state_hdmi[RESOURCE_V4L2].active &&
		!state_hdmi[RESOURCE_FB].active) {
		/* all resource stop */

		screen_disp_stop_hdmi stop;

		stop.handle = handleHDMI;
		if (ape5r_v4l2_get_rtapi_interface()  \
			->screen_display_stop_hdmi(&stop)) {
			printk_err("stop failed.\n");
			ret = -EINVAL;
		} else {
			screen_disp_delete    del;
			del.handle = handleHDMI;

			ape5r_v4l2_get_rtapi_interface()  \
			->screen_display_delete(&del);
			handleHDMI = NULL;
		}
	}

	if (ret == 0) {
		state_hdmi[resource].active   = 0;
		state_hdmi[resource].format   = 0;
		state_hdmi[resource].bg_color = 0;
	}

	printk_dbg(_HDMI_DBG, "curretn state:%p id0:%d  id1:%d\n", handleHDMI, \
		state_hdmi[RESOURCE_V4L2].active,                      \
		state_hdmi[RESOURCE_FB].active);
	return ret;
}

static int procedure_start(int resource, unsigned int format, \
	unsigned int bg_color)
{
	int ret = 0;

	if (state_hdmi[resource].active) {
		/* nothing to do */
		return ret;
	}

	if     (!state_hdmi[RESOURCE_V4L2].active &&
		!state_hdmi[RESOURCE_FB].active) {
		/* all resource stop */

		screen_disp_start_hdmi start;

		/* get handle */
		handleHDMI = ape5r_v4l2_get_rtapi_interface()  \
			->screen_display_new();

		/* start */
		start.handle = handleHDMI;
		start.format = format;
		start.background_color = bg_color;
		if (ape5r_v4l2_get_rtapi_interface()  \
			->screen_display_start_hdmi(&start)) {
			screen_disp_delete    del;

			printk_err("start failed.\n");
			ret = -EINVAL;
			del.handle = handleHDMI;
			ape5r_v4l2_get_rtapi_interface()  \
				->screen_display_delete(&del);
			handleHDMI = NULL;
		}
	}

	if (ret == 0) {
		state_hdmi[resource].active = 1;
		state_hdmi[resource].format = format;
		state_hdmi[resource].bg_color = bg_color;
	}
	printk_dbg(_HDMI_DBG, "curretn state:%p id0:%d  id1:%d\n", handleHDMI, \
		state_hdmi[RESOURCE_V4L2].active,                      \
		state_hdmi[RESOURCE_FB].active);
	return ret;
}



int ape5r_v4l2_display_stopHDMI(unsigned int resource)
{
	int ret = 0;

	if (!check_interface()) {
		printk_err("RTAPI not registered.\n");
		return -EINVAL;
	}
	/*********************************/
	/* initialize state if necessary */
	/*********************************/
	initialize_HDMIstate();

	/*********************************/
	/* argument check                */
	/*********************************/
	switch (resource) {
	case RESOURCE_FB:
	case RESOURCE_V4L2:
		break;
	default:
		printk_err("invalid argument.\n");
		ret = -EINVAL;
		return ret;
	}

	down(&sem_hdmi);
	printk_dbg(_HDMI_DBG, "HDMI stop request from %d\n", resource);
	switch (resource) {
	case RESOURCE_FB:
	case RESOURCE_V4L2:
		if (!state_hdmi[resource].active)
			ret = -EBUSY;
		else
			ret = procedure_stop(resource);
		break;
	default:
		break;
	}
	up(&sem_hdmi);
	return ret;
}
EXPORT_SYMBOL(ape5r_v4l2_display_stopHDMI);

int ape5r_v4l2_display_startHDMI(unsigned int resource,  \
	unsigned int format, unsigned int bg_color)
{
	int ret = 0;
	struct HDMIstate *cur = NULL;

	if (!check_interface()) {
		printk_err("RTAPI not registered.\n");
		return -EINVAL;
	}
	/*********************************/
	/* initialize state if necessary */
	/*********************************/
	initialize_HDMIstate();

	/*********************************/
	/* argument check                */
	/*********************************/
	switch (resource) {
	case RESOURCE_FB:
	case RESOURCE_V4L2:
		break;
	default:
		printk_err("invalid argument resouce.\n");
		ret = -EINVAL;
		return ret;
	}
	switch (format) {
	case RT_DISPLAY_720_480P60:
	case RT_DISPLAY_1280_720P60:
	case RT_DISPLAY_1920_1080I60:
	case RT_DISPLAY_1920_1080P24:
		break;
	default:
		printk_err("invalid argument format.\n");
		ret = -EINVAL;
		return ret;
	}
	if (bg_color & ~0xffffff) {
		printk_err("invalid argument bg_color.\n");
		ret = -EINVAL;
		return ret;
	}

	down(&sem_hdmi);
	printk_dbg(_HDMI_DBG, "HDMI start request from %d\n", resource);

	/* handle conflict with V4L2 */
	if (state_hdmi[resource].active) {
		/* busy */
		ret = -EBUSY;
	} else if (resource == RESOURCE_FB &&       \
		state_hdmi[RESOURCE_V4L2].active) {

		if (state_hdmi[RESOURCE_V4L2].format   != format ||
		    state_hdmi[RESOURCE_V4L2].bg_color != bg_color) {
			/*---------------------------------
			 * stop V4L2 HDMI output.
			 *-------------------------------*/
			if (ape5r_v4l2_HDMIstate_comflict()) {
				printk_err("control error.\n");
				ret = -EINVAL;
			} else {
				if (procedure_stop(RESOURCE_V4L2)) {
					printk_err("stop failed.\n");
					ret = -EINVAL;
				}
			}
		}
	}

	if (ret == 0) {
		/* search active configuration. */
		int i;
		int conflict;
		for (i = 0 ; i < 2 ; i++) {
			if (state_hdmi[i].active) {
				cur = &state_hdmi[i];
				break;
			}
		}

		conflict = 0;
		if (cur) {
			conflict = (cur->format != format) || \
				(cur->bg_color != bg_color);
		}
		if (conflict) {
			printk_err("already start HDMI.\n");
			ret = -EBUSY;
		} else {
			if (procedure_start(resource, format, bg_color)) {
				printk_err("start failed error.\n");
				ret = -EINVAL;
			}
		}
	}
	up(&sem_hdmi);
	return ret;
}
EXPORT_SYMBOL(ape5r_v4l2_display_startHDMI);

extern int ape5r_v4l2_display_getstateHDMI(unsigned int *format, \
	unsigned int *bg_color)
{
	int ret = 0;
	int i;
	struct HDMIstate *cur = NULL;

	if (!check_interface()) {
		printk_err("RTAPI not registered.\n");
		return -EINVAL;
	}
	/*********************************/
	/* initialize state if necessary */
	/*********************************/
	initialize_HDMIstate();

	/*********************************/
	/* argument check                */
	/*********************************/
	if (format == NULL || bg_color == NULL) {
		ret = -1;
		return ret;
	}

	down(&sem_hdmi);
	for (i = 0 ; i < 2 ; i++) {
		if (state_hdmi[i].active) {
			cur = &state_hdmi[i];
			break;
		}
	}
	if (cur == NULL) {
		ret = -1;
	} else {
		*format   = cur->format;
		*bg_color = cur->bg_color;
	}
	up(&sem_hdmi);

	return ret;
}
EXPORT_SYMBOL(ape5r_v4l2_display_getstateHDMI);

