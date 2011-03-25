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

#include <rtapi/screen_display.h>

#ifndef _APE5R_V4L2_DISPLAY_H
#define _APE5R_V4L2_DISPLAY_H

/*===============================================================*/
/* external functions                                            */
/*===============================================================*/
extern int ape5r_v4l2_display_stopHDMI(unsigned int resource);
extern int ape5r_v4l2_display_startHDMI(
	unsigned int resource, unsigned int format, unsigned int bg_color);
extern int ape5r_v4l2_display_getstateHDMI(
	unsigned int *format, unsigned int *bg_color);

/*===============================================================*/
/* define for resource                                           */
/*===============================================================*/
#define RESOURCE_FB      0
#define RESOURCE_V4L2    1

#endif
