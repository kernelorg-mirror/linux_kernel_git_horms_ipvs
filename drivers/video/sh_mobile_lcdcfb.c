/*
 * SuperH Mobile LCDC Framebuffer
 *
 * Copyright (c) 2008 Magnus Damm
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

/*
 * drivers/video/sh_mobile_lcdcfb.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <video/sh_mobile_lcdc.h>
#include <asm/atomic.h>

#include <rtapi/screen_display.h>
#include <rtapi/screen_common.h>

#define CHAN_NUM 2

#define PALETTE_NR 16
#define SIDE_B_OFFSET 0x1000
#define MIRROR_OFFSET 0x2000

struct TN_PAKET_DATA{
	unsigned long	data1;
	unsigned long	data2;
};

struct TN_PAKET_DATA mcap_off = {
	0x29000200, 0xB2000000
};

struct TN_PAKET_DATA mcap_on = {
	0x29000200, 0xB2030000
};

struct TN_PAKET_DATA long_paket_data1[] = {
	{ 0x29000300, 0xEF010100 },
	{ 0x29000300, 0xEF606700 },
	{ 0x29000200, 0x6E460000 },
	{ 0, 0 }
};
struct TN_PAKET_DATA long_paket_data2[] = {
	{ 0x29000200, 0x53000000 },
	{ 0x29000200, 0x540B0000 },
	{ 0x29000200, 0x55030000 },
	{ 0x29000200, 0x56100000 },
	{ 0x29000200, 0x57000000 },
	{ 0x29000200, 0x58030000 },
	{ 0x29000200, 0x593F0000 },
	{ 0x29000200, 0x5AFD0000 },
	{ 0x29000200, 0x5B000000 },
	{ 0x29000200, 0x5C000000 },
	{ 0x29000200, 0x5D000000 },
	{ 0x29000200, 0x5E160000 },
	{ 0x29000200, 0x5F030000 },
	{ 0x29000200, 0x60010000 },
	{ 0x29000200, 0x61000000 },
	{ 0x29000200, 0x62040000 },
	{ 0x29000200, 0x67710000 },
	{ 0x29000200, 0x681E0000 },
	{ 0x29000200, 0x692D0000 },
	{ 0x29000200, 0x6A060000 },
	{ 0x29000200, 0x6B010000 },
	{ 0x29000200, 0x6C070000 },
	{ 0x29000200, 0x6D050000 },
	{ 0x29000200, 0x6E460000 },
	{ 0x29000200, 0x6F140000 },
	{ 0x29000200, 0x70170000 },
	{ 0x29000200, 0x71330000 },
	{ 0x29000200, 0x720F0000 },
	{ 0x29000200, 0x73000000 },
	{ 0x29000200, 0x740F0000 },
	{ 0x29000200, 0x75000000 },
	{ 0x29000200, 0x760B0000 },
	{ 0x29000200, 0x77530000 },
	{ 0x29000200, 0x78350000 },
	{ 0x29000200, 0x79410000 },
	{ 0x29000200, 0x7A000000 },
	{ 0x29000200, 0x7BFC0000 },
	{ 0x29000200, 0x7C190000 },
	{ 0x29000200, 0x7D1E0000 },
	{ 0x29000200, 0x7E230000 },
	{ 0x29000200, 0x7F200000 },
	{ 0x29000200, 0x80000000 },
	{ 0x29000200, 0x81FC0000 },
	{ 0x29000200, 0x82000000 },
	{ 0x29000200, 0x83000000 },
	{ 0x29000200, 0x84000000 },
	{ 0x29000200, 0x85000000 },
	{ 0x29000200, 0x86000000 },
	{ 0x29000200, 0x87FC0000 },
	{ 0x29000200, 0x88050000 },
	{ 0x29000200, 0x89B80000 },
	{ 0x29000200, 0x8A920000 },
	{ 0x29000200, 0x8B010000 },
	{ 0x29000200, 0x8C000000 },
	{ 0x29000200, 0x8D0F0000 },
	{ 0x29000200, 0x8E660000 },
	{ 0x29000200, 0x8F6B0000 },
	{ 0x29000200, 0x90700000 },
	{ 0x29000200, 0x91770000 },
	{ 0x29000200, 0x92840000 },
	{ 0x29000200, 0x93930000 },
	{ 0x29000200, 0x94A20000 },
	{ 0x29000200, 0x95B20000 },
	{ 0x29000200, 0x96C10000 },
	{ 0x29000200, 0x97CE0000 },
	{ 0x29000200, 0x98DA0000 },
	{ 0x29000200, 0x99E40000 },
	{ 0x29000200, 0x9AEC0000 },
	{ 0x29000200, 0x9BF30000 },
	{ 0x29000200, 0x9CF90000 },
	{ 0x29000200, 0x9DFF0000 },
	{ 0x29000200, 0x9E000000 },
	{ 0x29000200, 0x9F030000 },
	{ 0x29000200, 0xA0550000 },
	{ 0x29000200, 0xB3000000 },
	{ 0x29000200, 0xB44F0000 },
	{ 0x29000200, 0xC7000000 },
	{ 0x29000200, 0xC8010000 },
	{ 0x29000200, 0xC9000000 },
	{ 0x29000200, 0xCA000000 },
	{ 0, 0 }
};

struct sh_mobile_lcdc_priv;
struct sh_mobile_lcdc_chan {
	struct sh_mobile_lcdc_priv *lcdc;
	struct sh_mobile_lcdc_chan_cfg cfg;
	u32 pseudo_palette[PALETTE_NR];
	struct fb_info *info;
	dma_addr_t dma_handle;
	unsigned long pan_offset;
};

struct sh_mobile_lcdc_priv {
	struct device *dev;
	struct sh_mobile_lcdc_chan ch[CHAN_NUM];
};

struct sh_mobile_lcdc_ext_param {
	struct semaphore sem_lcd;
	int lcd_type;
	void *aInfo;
	unsigned short o_mode;
	unsigned int phy_addr;
	unsigned int vir_addr;
	unsigned short rect_x;
	unsigned short rect_y;
	unsigned short rect_width;
	unsigned short rect_height;
	unsigned short alpha;
	unsigned short key_clr;
	unsigned short v4l2_state;
};

struct sh_mobile_lcdc_ext_param lcd_ext_param[CHAN_NUM];

static int sh_mobile_lcdc_setcolreg(u_int regno,
				    u_int red, u_int green, u_int blue,
				    u_int transp, struct fb_info *info)
{
	/* No. of hw registers */
	if (regno >= 256)
		return 1;

	/* grayscale works only partially under directcolor */
	if (info->var.grayscale) {
		/* grayscale = 0.30*R + 0.59*G + 0.11*B */
		red = green = blue = (red * 77 + green * 151 + blue * 28) >> 8;
	}

#define CNVT_TOHW(val, width) ((((val)<<(width))+0x7FFF-(val))>>16)
	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:	/* FALL THROUGH */
	case FB_VISUAL_PSEUDOCOLOR:
	{
		red = CNVT_TOHW(red, info->var.red.length);
		green = CNVT_TOHW(green, info->var.green.length);
		blue = CNVT_TOHW(blue, info->var.blue.length);
		transp = CNVT_TOHW(transp, info->var.transp.length);
		break;
	}
	case FB_VISUAL_DIRECTCOLOR:
	{
		red = CNVT_TOHW(red, 8);	/* expect 8 bit DAC */
		green = CNVT_TOHW(green, 8);
		blue = CNVT_TOHW(blue, 8);
		/* hey, there is bug in transp handling... */
		transp = CNVT_TOHW(transp, 8);
		break;
	}
	}
#undef CNVT_TOHW
	/* Truecolor has hardware independent palette */
	if (info->fix.visual == FB_VISUAL_TRUECOLOR) {
		u32 v;

		if (regno >= 16)
			return 1;

		v = (red << info->var.red.offset) |
		    (green << info->var.green.offset) |
		    (blue << info->var.blue.offset) |
		    (transp << info->var.transp.offset);
		switch (info->var.bits_per_pixel) {
		case 16:	/* FALL THROUGH */
		case 24:	/* FALL THROUGH */
		case 32:
			((u32 *) (info->pseudo_palette))[regno] = v;
			break;
		case 8:		/* FALL THROUGH */
		default:
			break;
		}
	}

	return 0;
}

static struct fb_fix_screeninfo sh_mobile_lcdc_fix  = {
	.id		= "SH Mobile LCDC",
	.type		= FB_TYPE_PACKED_PIXELS,
	.visual		= FB_VISUAL_TRUECOLOR,
	.accel		= FB_ACCEL_NONE,
	.xpanstep	= 0,
	.ypanstep	= 1,
	.ywrapstep	= 0,
};

struct rtdisp_func rtdisp = {NULL, NULL, NULL};

void register_disp_func(struct rtdisp_func *pfunc)
{
	rtdisp.rtdisp_new		= pfunc->rtdisp_new;
	rtdisp.rtdisp_set_parameters	= pfunc->rtdisp_set_parameters;
	rtdisp.rtdisp_get_address	= pfunc->rtdisp_get_address;
	rtdisp.rtdisp_draw		= pfunc->rtdisp_draw;
	rtdisp.rtdisp_start_lcd		= pfunc->rtdisp_start_lcd;
	rtdisp.rtdisp_stop_lcd		= pfunc->rtdisp_stop_lcd;
	rtdisp.rtdisp_set_lcd_refresh	= pfunc->rtdisp_set_lcd_refresh;
	rtdisp.rtdisp_write_dsi_short_packet
		= pfunc->rtdisp_write_dsi_short_packet;
	rtdisp.rtdisp_write_dsi_long_packet
		= pfunc->rtdisp_write_dsi_long_packet;
	rtdisp.rtdisp_set_lcd_if_parameters
		= pfunc->rtdisp_set_lcd_if_parameters;
	return;
}
EXPORT_SYMBOL(register_disp_func);

static int display_initialize(int lcd_num)
{
	screen_disp_param disp_param;
	screen_disp_get_address disp_addr;
	screen_disp_write_dsi_long write_dsi_l;
	screen_disp_write_dsi_short write_dsi_s;

	int ret = 0;
	struct TN_PAKET_DATA *par;
	unsigned char	cmd[4];
	unsigned int i;

	lcd_ext_param[lcd_num].aInfo = rtdisp.rtdisp_new();
	if (lcd_ext_param[lcd_num].aInfo == NULL) {
		printk(KERN_ALERT "disp_new err!\n");
		return -1;
	}

	disp_param.handle = lcd_ext_param[lcd_num].aInfo;
	disp_param.output_mode = lcd_ext_param[lcd_num].o_mode;
	disp_param.key_color = lcd_ext_param[lcd_num].key_clr;
	disp_param.alpha = lcd_ext_param[lcd_num].alpha;

	ret = rtdisp.rtdisp_set_parameters(&disp_param);
	if (ret != 0) {
		printk(KERN_ALERT "disp_set_parameters err!\n");
		return -1;
	}

	disp_addr.handle = lcd_ext_param[lcd_num].aInfo;
	disp_addr.output_mode = lcd_ext_param[lcd_num].o_mode;
	ret = rtdisp.rtdisp_get_address(&disp_addr);
	if (ret != 0) {
		printk(KERN_ALERT "disp_get_address err!\n");
		return -1;
	}

	par = &mcap_off;
	cmd[0] = (par->data2 >> 24) & 0xFF;
	cmd[1] = (par->data2 >> 16) & 0xFF;
	cmd[2] = (par->data2 >> 8) & 0xFF;
	cmd[3] = (par->data2) & 0xFF;
	write_dsi_l.handle = lcd_ext_param[lcd_num].aInfo;
	write_dsi_l.data_id = (par->data1 & 0xFF000000) >> 24;
	write_dsi_l.data_count = (par->data1 & 0x0000FF00) >> 8;
	write_dsi_l.write_data = (unsigned char *)&cmd[0];
	ret = rtdisp.rtdisp_write_dsi_long_packet(&write_dsi_l);
	if (ret != 0) {
		printk(KERN_ALERT "disp_write_dsi_long err!\n");
		return -1;
	}

	for (i = 0; long_paket_data1[i].data1 != 0; i++) {
		par = &(long_paket_data1[i]);
		cmd[0] = (par->data2 >> 24) & 0xFF;
		cmd[1] = (par->data2 >> 16) & 0xFF;
		cmd[2] = (par->data2 >> 8) & 0xFF;
		cmd[3] = (par->data2) & 0xFF;
		write_dsi_l.handle = lcd_ext_param[lcd_num].aInfo;
		write_dsi_l.data_id = (par->data1 & 0xFF000000) >> 24;
		write_dsi_l.data_count = (par->data1 & 0x0000FF00) >> 8;
		write_dsi_l.write_data = (unsigned char *)&cmd[0];
		ret = rtdisp.rtdisp_write_dsi_long_packet(&write_dsi_l);
		if (ret != 0) {
			printk(KERN_ALERT "disp_write_dsi_long err!\n");
			return -1;
		}
	}
	/* LCD MCAP ON */
	par = &mcap_on;
	cmd[0] = (par->data2 >> 24) & 0xFF;
	cmd[1] = (par->data2 >> 16) & 0xFF;
	cmd[2] = (par->data2 >> 8) & 0xFF;
	cmd[3] = (par->data2) & 0xFF;
	write_dsi_l.handle = lcd_ext_param[lcd_num].aInfo;
	write_dsi_l.data_id = (par->data1 & 0xFF000000) >> 24;
	write_dsi_l.data_count = (par->data1 & 0x0000FF00) >> 8;
	write_dsi_l.write_data = (unsigned char *)&cmd[0];
	ret = rtdisp.rtdisp_write_dsi_long_packet(&write_dsi_l);
	if (ret != 0) {
		printk(KERN_ALERT "disp_write_dsi_long err!\n");
		return -1;
	}

	write_dsi_s.handle = lcd_ext_param[lcd_num].aInfo;
	write_dsi_s.data_id = 0x05;
	write_dsi_s.reg_address = 0x11;
	write_dsi_s.write_data = 0x00;
	ret = rtdisp.rtdisp_write_dsi_short_packet(&write_dsi_s);
	if (ret != 0) {
		printk(KERN_ALERT "disp_write_dsi_short err!\n");
		return -1;
	}

	/* LCD MCAP OFF */
	par = &mcap_off;
	cmd[0] = (par->data2 >> 24) & 0xFF;
	cmd[1] = (par->data2 >> 16) & 0xFF;
	cmd[2] = (par->data2 >> 8) & 0xFF;
	cmd[3] = (par->data2) & 0xFF;
	write_dsi_l.handle = lcd_ext_param[lcd_num].aInfo;
	write_dsi_l.data_id = (par->data1 & 0xFF000000) >> 24;
	write_dsi_l.data_count = (par->data1 & 0x0000FF00) >> 8;
	write_dsi_l.write_data = (unsigned char *)&cmd[0];
	ret = rtdisp.rtdisp_write_dsi_long_packet(&write_dsi_l);
	if (ret != 0) {
		printk(KERN_ALERT "disp_write_dsi_long err!\n");
		return -1;
	}

	for (i = 0; long_paket_data2[i].data1 != 0; i++) {
		par = &(long_paket_data2[i]);
		cmd[0] = (par->data2 >> 24) & 0xFF;
		cmd[1] = (par->data2 >> 16) & 0xFF;
		cmd[2] = (par->data2 >> 8) & 0xFF;
		cmd[3] = (par->data2) & 0xFF;
		write_dsi_l.handle = lcd_ext_param[lcd_num].aInfo;
		write_dsi_l.data_id = (par->data1 & 0xFF000000) >> 24;
		write_dsi_l.data_count = (par->data1 & 0x0000FF00) >> 8;
		write_dsi_l.write_data = (unsigned char *)&cmd[0];
		ret = rtdisp.rtdisp_write_dsi_long_packet(&write_dsi_l);
		if (ret != 0) {
			printk(KERN_ALERT "disp_write_dsi_long err!\n");
			return -1;
		}
	}
	/* LCD MCAP ON */
	par = &mcap_on;
	cmd[0] = (par->data2 >> 24) & 0xFF;
	cmd[1] = (par->data2 >> 16) & 0xFF;
	cmd[2] = (par->data2 >> 8) & 0xFF;
	cmd[3] = (par->data2) & 0xFF;
	write_dsi_l.handle = lcd_ext_param[lcd_num].aInfo;
	write_dsi_l.data_id = (par->data1 & 0xFF000000) >> 24;
	write_dsi_l.data_count = (par->data1 & 0x0000FF00) >> 8;
	write_dsi_l.write_data = (unsigned char *)&cmd[0];
	ret = rtdisp.rtdisp_write_dsi_long_packet(&write_dsi_l);
	if (ret != 0) {
		printk(KERN_ALERT "disp_write_dsi_long err!\n");
		return -1;
	}

	write_dsi_s.handle = lcd_ext_param[lcd_num].aInfo;
	write_dsi_s.data_id = 0x05;
	write_dsi_s.reg_address = 0x29;
	write_dsi_s.write_data = 0x00;
	ret = rtdisp.rtdisp_write_dsi_short_packet(&write_dsi_s);
	if (ret != 0) {
		printk(KERN_ALERT "disp_write_dsi_short err!\n");
		return -1;
	}

	return 0;
}

int sh_mobile_lcdc_keyclr_set(unsigned short s_key_clr,
			      unsigned short output_mode)
{
	int i, ret;
	screen_disp_param disp_param;

	for (i = 0 ; i < CHAN_NUM ; i++) {
		if (output_mode == lcd_ext_param[i].o_mode)
			break;
	}
	if (i >= CHAN_NUM) {
		printk(KERN_ALERT "lcdc_key_clr_set param ERR\n");
		up(&lcd_ext_param[i].sem_lcd);
		return -1;
	}
	if (down_interruptible(&lcd_ext_param[i].sem_lcd)) {
		printk(KERN_ALERT "down_interruptible failed\n");
		up(&lcd_ext_param[i].sem_lcd);
		return -1;
	}

	lcd_ext_param[i].key_clr = s_key_clr;

	if (rtdisp.rtdisp_new != NULL) {
		if (lcd_ext_param[i].aInfo == NULL) {
			ret = display_initialize(i);
			if (ret != 0) {
				up(&lcd_ext_param[i].sem_lcd);
				return -1;
			}
		} else {
			disp_param.handle = lcd_ext_param[i].aInfo;
			disp_param.output_mode = lcd_ext_param[i].o_mode;
			disp_param.key_color = lcd_ext_param[i].key_clr;
			disp_param.alpha = lcd_ext_param[i].alpha;
			ret = rtdisp.rtdisp_set_parameters(&disp_param);
			if (ret != 0) {
				up(&lcd_ext_param[i].sem_lcd);
				return -1;
			}
		}
	} else {
		printk(KERN_ALERT "nothing MFI driver\n");
	}

	up(&lcd_ext_param[i].sem_lcd);

	return 0;

}
EXPORT_SYMBOL(sh_mobile_lcdc_keyclr_set);

int sh_mobile_lcdc_alpha_set(unsigned short s_alpha,
			      unsigned short output_mode)
{
	int i, ret;
	screen_disp_param disp_param;

	for (i = 0 ; i < CHAN_NUM ; i++) {
		if (output_mode == lcd_ext_param[i].o_mode)
			break;
	}
	if (i >= CHAN_NUM) {
		printk(KERN_ALERT "lcdc_key_clr_set param ERR\n");
		up(&lcd_ext_param[i].sem_lcd);
		return -1;
	}
	if (down_interruptible(&lcd_ext_param[i].sem_lcd)) {
		printk(KERN_ALERT "down_interruptible failed\n");
		up(&lcd_ext_param[i].sem_lcd);
		return -1;
	}

	lcd_ext_param[i].alpha = s_alpha;

	if (rtdisp.rtdisp_new != NULL) {
		if (lcd_ext_param[i].aInfo == NULL) {
			ret = display_initialize(i);
			if (ret != 0) {
				up(&lcd_ext_param[i].sem_lcd);
				return -1;
			}
		} else {
			disp_param.handle = lcd_ext_param[i].aInfo;
			disp_param.output_mode = lcd_ext_param[i].o_mode;
			disp_param.key_color = lcd_ext_param[i].key_clr;
			disp_param.alpha = lcd_ext_param[i].alpha;
			ret = rtdisp.rtdisp_set_parameters(&disp_param);
			if (ret != 0) {
				up(&lcd_ext_param[i].sem_lcd);
				return -1;
			}
		}
	} else {
		printk(KERN_ALERT "nothing MFI driver\n");
	}


	up(&lcd_ext_param[i].sem_lcd);

	return 0;

}
EXPORT_SYMBOL(sh_mobile_lcdc_alpha_set);


int sh_mobile_lcdc_refresh(unsigned short set_state,
			      unsigned short output_mode)
{
	int i, ret;
	screen_disp_set_lcd_refresh disp_refresh;

	for (i = 0 ; i < CHAN_NUM ; i++) {
		if (output_mode == lcd_ext_param[i].o_mode)
			break;
	}
	if (i >= CHAN_NUM) {
		printk(KERN_ALERT "lcdc_key_clr_set param ERR\n");
		up(&lcd_ext_param[i].sem_lcd);
		return -1;
	}
	if (down_interruptible(&lcd_ext_param[i].sem_lcd)) {
		printk(KERN_ALERT "down_interruptible failed\n");
		up(&lcd_ext_param[i].sem_lcd);
		return -1;
	}

	lcd_ext_param[i].v4l2_state = set_state;
	if (rtdisp.rtdisp_new != NULL) {
		if (lcd_ext_param[i].aInfo != NULL) {
			disp_refresh.handle = lcd_ext_param[i].aInfo;
			disp_refresh.output_mode =
				lcd_ext_param[i].o_mode;
			disp_refresh.refresh_mode =
				set_state;
			ret = rtdisp.rtdisp_set_lcd_refresh(
				&disp_refresh);
			if (ret != 0) {
				up(&lcd_ext_param[i].sem_lcd);
				return -1;
			}
		}
	} else {
		printk(KERN_ALERT "nothing MFI driver\n");
	}

	up(&lcd_ext_param[i].sem_lcd);

	return 0;

}
EXPORT_SYMBOL(sh_mobile_lcdc_refresh);

static int sh_mobile_fb_pan_display(struct fb_var_screeninfo *var,
				     struct fb_info *info)
{
	struct sh_mobile_lcdc_chan *ch = info->par;
	unsigned long new_pan_offset;

	int ret = 0;

/* onscreen buffer 2 */
	unsigned int i;
	unsigned short set_format;
	unsigned char  lcd_num;
	screen_disp_draw disp_draw;
#if 0
	screen_disp_set_lcd_refresh disp_refresh;
#endif
	new_pan_offset = (var->yoffset * info->fix.line_length) +
		(var->xoffset * (info->var.bits_per_pixel / 8));

	for (i = 0 ; i < CHAN_NUM ; i++) {
		if (ch->cfg.chan == lcd_ext_param[i].lcd_type)
			break;
	}
	lcd_num = i;
	if (lcd_num >= CHAN_NUM)
		return -EINVAL;

	if (down_interruptible(&lcd_ext_param[lcd_num].sem_lcd)) {
		printk(KERN_ALERT "down_interruptible failed\n");
		return -ERESTARTSYS;
	}

	/* Set the source address for the next refresh */

	if (rtdisp.rtdisp_new != NULL) {
		if (lcd_ext_param[lcd_num].aInfo == NULL) {

			ret = display_initialize(lcd_num);
			if (ret != 0) {
				up(&lcd_ext_param[lcd_num].sem_lcd);
				return -EIO;
			}
		}
		if (lcd_ext_param[lcd_num].aInfo != NULL) {

#if 0
			if (lcd_ext_param[lcd_num].v4l2_state
			    == RT_DISPLAY_REFRESH_ON){
				disp_refresh.handle =
					lcd_ext_param[lcd_num].aInfo;
				disp_refresh.output_mode =
					lcd_ext_param[lcd_num].o_mode;
				disp_refresh.refresh_mode =
					RT_DISPLAY_REFRESH_OFF;
				ret = rtdisp.rtdisp_set_lcd_refresh(
					&disp_refresh);
				if (ret != 0) {
					up(&lcd_ext_param[lcd_num].sem_lcd);
					return -EIO;
				}
			}
#endif
			if (var->bits_per_pixel == 16)
				set_format = RT_DISPLAY_FORMAT_RGB565;
			else
				set_format = RT_DISPLAY_FORMAT_ARGB8888;

#ifdef CONFIG_FB_SH_MOBILE_DOUBLE_BUF
			disp_draw.handle = lcd_ext_param[lcd_num].aInfo;
			disp_draw.output_mode = lcd_ext_param[lcd_num].o_mode;
			disp_draw.draw_rect.x = lcd_ext_param[lcd_num].rect_x;
			disp_draw.draw_rect.y = lcd_ext_param[lcd_num].rect_y;
			disp_draw.draw_rect.width =
				lcd_ext_param[lcd_num].rect_width;
			disp_draw.draw_rect.height =
				lcd_ext_param[lcd_num].rect_height;
			disp_draw.format = set_format;
			disp_draw.buffer_offset = new_pan_offset;
			ret = rtdisp.rtdisp_draw(&disp_draw);
			if (ret != 0) {
				up(&lcd_ext_param[lcd_num].sem_lcd);
				return -EIO;
			}
#else

			memcpy((void *)lcd_ext_param[lcd_num].vir_addr,
			       (void *)(info->screen_base + new_pan_offset),
			       (lcd_ext_param[lcd_num].rect_width *
				lcd_ext_param[lcd_num].rect_height *
				var->bits_per_pixel / 8));

			disp_draw.handle = lcd_ext_param[lcd_num].aInfo;
			disp_draw.output_mode = lcd_ext_param[lcd_num].o_mode;
			disp_draw.draw_rect.x = lcd_ext_param[lcd_num].rect_x;
			disp_draw.draw_rect.y = lcd_ext_param[lcd_num].rect_y;
			disp_draw.draw_rect.width =
				lcd_ext_param[lcd_num].rect_width;
			disp_draw.draw_rect.height =
				lcd_ext_param[lcd_num].rect_height;
			disp_draw.format = set_format;
			ret = rtdisp.rtdisp_draw(&disp_draw);
			if (ret != 0) {
				up(&lcd_ext_param[lcd_num].sem_lcd);
				return -EIO;
			}
#endif

#if 0
			if (lcd_ext_param[lcd_num].v4l2_state
			    == RT_DISPLAY_REFRESH_ON){
				disp_refresh.handle =
					lcd_ext_param[lcd_num].aInfo;
				disp_refresh.output_mode =
					lcd_ext_param[lcd_num].o_mode;
				disp_refresh.refresh_mode =
					RT_DISPLAY_REFRESH_ON;
				ret = rtdisp.rtdisp_set_lcd_refresh(
					&disp_refresh);
				if (ret != 0) {
					up(&lcd_ext_param[lcd_num].sem_lcd);
					return -EIO;
				}
			}
#endif
		}
	} else {

		printk(KERN_ALERT "nothing MFI driver\n");

	}

	ch->pan_offset = new_pan_offset;

	up(&lcd_ext_param[lcd_num].sem_lcd);

	return 0;
}

static int sh_mobile_ioctl(struct fb_info *info, unsigned int cmd,
		       unsigned long arg)
{
	int retval;

	switch (cmd) {
	case FBIO_WAITFORVSYNC:
		retval = 0;
		break;

	default:
		retval = -ENOIOCTLCMD;
		break;
	}
	return retval;
}

static int sh_mobile_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	unsigned long start;
	unsigned long off;
	u32 len;

	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;

	off = vma->vm_pgoff << PAGE_SHIFT;
	start = info->fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);

	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;

	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;

	/* Accessing memory will be done non-cached. */
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	/* To stop the swapper from even considering these pages */
	vma->vm_flags |= (VM_IO | VM_RESERVED);

	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			    vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}
static int sh_mobile_fb_check_var(struct fb_var_screeninfo *var,
				  struct fb_info *info)
{
	switch (var->bits_per_pixel) {
	case 16: /* RGB 565 */
		var->red.offset    = 11;
		var->red.length    = 5;
		var->green.offset  = 5;
		var->green.length  = 6;
		var->blue.offset   = 0;
		var->blue.length   = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 32: /* ARGB 8888*/
		var->red.offset    = 16;
		var->red.length    = 8;
		var->green.offset  = 8;
		var->green.length  = 8;
		var->blue.offset   = 0;
		var->blue.length   = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		break;
	default:
		return -EINVAL;

	}
	return 0;

}

static struct fb_ops sh_mobile_lcdc_ops = {
	.owner          = THIS_MODULE,
	.fb_setcolreg	= sh_mobile_lcdc_setcolreg,
	.fb_check_var	= sh_mobile_fb_check_var,
	.fb_read        = fb_sys_read,
	.fb_write       = fb_sys_write,
	.fb_fillrect	= sys_fillrect,
	.fb_copyarea	= sys_copyarea,
	.fb_imageblit	= sys_imageblit,
	.fb_pan_display = sh_mobile_fb_pan_display,
	.fb_ioctl       = sh_mobile_ioctl,
	.fb_mmap	= sh_mobile_mmap,
};

static int sh_mobile_lcdc_set_bpp(struct fb_var_screeninfo *var, int bpp)
{
	switch (bpp) {
	case 16: /* RGB 565 */
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;

	case 32: /* ARGB 8888 */
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		break;
	default:
		return -EINVAL;
	}
	var->bits_per_pixel = bpp;
	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;
	return 0;
}

#if 0 /* this function is implemented in future. */
static int sh_mobile_lcdc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	sh_mobile_lcdc_stop(platform_get_drvdata(pdev));
	return 0;
}

static int sh_mobile_lcdc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return sh_mobile_lcdc_start(platform_get_drvdata(pdev));
}

static int sh_mobile_lcdc_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sh_mobile_lcdc_priv *p = platform_get_drvdata(pdev);
	struct sh_mobile_lcdc_chan *ch;
	int k, n;

	/* save per-channel registers */
	for (k = 0; k < ARRAY_SIZE(p->ch); k++) {
		ch = &p->ch[k];
		if (!ch->enabled)
			continue;
		for (n = 0; n < NR_CH_REGS; n++)
			ch->saved_ch_regs[n] = lcdc_read_chan(ch, n);
	}

	/* save shared registers */
	for (n = 0; n < NR_SHARED_REGS; n++)
		p->saved_shared_regs[n] = lcdc_read(p, lcdc_shared_regs[n]);

	/* turn off LCDC hardware */
	lcdc_write(p, _LDCNT1R, 0);
	return 0;
}

static int sh_mobile_lcdc_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sh_mobile_lcdc_priv *p = platform_get_drvdata(pdev);
	struct sh_mobile_lcdc_chan *ch;
	int k, n;

	/* restore per-channel registers */
	for (k = 0; k < ARRAY_SIZE(p->ch); k++) {
		ch = &p->ch[k];
		if (!ch->enabled)
			continue;
		for (n = 0; n < NR_CH_REGS; n++)
			lcdc_write_chan(ch, n, ch->saved_ch_regs[n]);
	}

	/* restore shared registers */
	for (n = 0; n < NR_SHARED_REGS; n++)
		lcdc_write(p, lcdc_shared_regs[n], p->saved_shared_regs[n]);

	return 0;
}

static const struct dev_pm_ops sh_mobile_lcdc_dev_pm_ops = {
	.suspend = sh_mobile_lcdc_suspend,
	.resume = sh_mobile_lcdc_resume,
	.runtime_suspend = sh_mobile_lcdc_runtime_suspend,
	.runtime_resume = sh_mobile_lcdc_runtime_resume,
};
#endif

static int sh_mobile_lcdc_remove(struct platform_device *pdev);

static unsigned long RoundUpToMultiple(unsigned long x, unsigned long y)
{
	unsigned long div = x / y;
	unsigned long rem = x % y;

	return (div + ((rem == 0) ? 0 : 1)) * y;
}

static unsigned long GCD(unsigned long x, unsigned long y)
{
	while (y != 0) {
		unsigned long r = x % y;
		x = y;
		y = r;
	}
	return x;
}

static unsigned long LCM(unsigned long x, unsigned long y)
{
	unsigned long gcd = GCD(x, y);

	return (gcd == 0) ? 0 : ((x / gcd) * y);
}

static int __devinit sh_mobile_lcdc_probe(struct platform_device *pdev)
{
	struct fb_info *info;
	struct sh_mobile_lcdc_priv *priv;
	struct sh_mobile_lcdc_info *pdata;
	struct sh_mobile_lcdc_chan_cfg *cfg;
	struct resource *res;
	int error = 0;
	int i, j;
	unsigned long ulLCM;
	void *temp = NULL;

#ifndef CONFIG_FB_SH_MOBILE_DOUBLE_BUF
	void *buf = NULL;
#endif

	printk(KERN_ALERT "sh_mobile_lcdc_probe\n");

	if (!pdev->dev.platform_data) {
		dev_err(&pdev->dev, "no platform data defined\n");
		error = -EINVAL;
		goto err0;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "cannot allocate device data\n");
		error = -ENOMEM;
		goto err0;
	}

	priv->dev = &pdev->dev;
	platform_set_drvdata(pdev, priv);
	pdata = pdev->dev.platform_data;

	j = 0;
	for (i = 0; i < ARRAY_SIZE(pdata->ch); i++) {
		priv->ch[j].lcdc = priv;
		memcpy(&priv->ch[j].cfg, &pdata->ch[i], sizeof(pdata->ch[i]));

		priv->ch[j].pan_offset = 0;
		switch (pdata->ch[i].chan) {
		case LCDC_CHAN_MAINLCD:
			lcd_ext_param[i].o_mode = RT_DISPLAY_LCD1;
			lcd_ext_param[i].rect_x = SH_MLCD_RECTX;
			lcd_ext_param[i].rect_y = SH_MLCD_RECTY;
			lcd_ext_param[i].rect_width = SH_MLCD_WIDTH;
			lcd_ext_param[i].rect_height = SH_MLCD_HEIGHT;
			lcd_ext_param[i].alpha = 0xFF;
			lcd_ext_param[i].key_clr = SH_MLCD_TRCOLOR;
			lcd_ext_param[i].v4l2_state = RT_DISPLAY_REFRESH_ON;
			lcd_ext_param[i].phy_addr = SCREEN_DISPLAY_BUFF_ADDR;
			j++;
			break;
		case LCDC_CHAN_SUBLCD:
			lcd_ext_param[i].o_mode = RT_DISPLAY_LCD2;
			lcd_ext_param[i].rect_x = SH_SLCD_RECTX;
			lcd_ext_param[i].rect_y = SH_SLCD_RECTY;
			lcd_ext_param[i].rect_width = SH_SLCD_WIDTH;
			lcd_ext_param[i].rect_height = SH_SLCD_HEIGHT;
			lcd_ext_param[i].alpha = 0xFF;
			lcd_ext_param[i].key_clr = SH_SLCD_TRCOLOR;
			lcd_ext_param[i].v4l2_state = RT_DISPLAY_REFRESH_ON;
			/* SUBLCD undefined */
			lcd_ext_param[i].phy_addr = 0;
			j++;
			break;
		}
	}

	if (!j) {
		dev_err(&pdev->dev, "no channels defined\n");
		error = -EINVAL;
		goto err1;
	}

	for (i = 0; i < j; i++) {
		cfg = &priv->ch[i].cfg;

		priv->ch[i].info = framebuffer_alloc(0, &pdev->dev);
		if (!priv->ch[i].info) {
			dev_err(&pdev->dev, "unable to allocate fb_info\n");
			error = -ENOMEM;
			break;
		}

		info = priv->ch[i].info;
		info->fbops = &sh_mobile_lcdc_ops;
		info->var.xres = info->var.xres_virtual = cfg->lcd_cfg.xres;
		info->var.yres = cfg->lcd_cfg.yres;
		/* Default Y virtual resolution is 2x panel size */
		info->var.width = cfg->lcd_size_cfg.width;
		info->var.height = cfg->lcd_size_cfg.height;
		info->var.activate = FB_ACTIVATE_NOW;
		error = sh_mobile_lcdc_set_bpp(&info->var, cfg->bpp);
		if (error)
			break;
		info->fix = sh_mobile_lcdc_fix;
		info->fix.line_length = cfg->lcd_cfg.xres * (cfg->bpp / 8);

		/* 4kbyte align */
		ulLCM = LCM(info->fix.line_length, 0x1000);
		info->fix.smem_len = RoundUpToMultiple(
			info->fix.line_length*info->var.yres, ulLCM);
		info->fix.smem_len *= 2;

		info->var.yres_virtual = info->fix.smem_len
			/ info->fix.line_length;

#ifdef CONFIG_FB_SH_MOBILE_DOUBLE_BUF
/* onscreen buffer 2 */
		temp = ioremap(lcd_ext_param[i].phy_addr,
			       info->fix.smem_len);
		if (NULL == temp) {
			error = -ENOMEM;
			break;
		} else {
			lcd_ext_param[i].vir_addr = (unsigned int)temp;
		}
#else
		buf = dma_alloc_coherent(&pdev->dev, info->fix.smem_len,
					 &priv->ch[i].dma_handle, GFP_KERNEL);
		if (!buf) {
			dev_err(&pdev->dev, "unable to allocate buffer\n");
			error = -ENOMEM;
			break;
		}
		temp = ioremap(lcd_ext_param[i].phy_addr,
			       info->fix.smem_len);
		if (NULL == temp) {
			error = -ENOMEM;
			break;
		} else {
			lcd_ext_param[i].vir_addr = (unsigned int)temp;
		}
#endif
		info->pseudo_palette = &priv->ch[i].pseudo_palette;
		info->flags = FBINFO_FLAG_DEFAULT;

		error = fb_alloc_cmap(&info->cmap, PALETTE_NR, 0);
		if (error < 0) {
			dev_err(&pdev->dev, "unable to allocate cmap\n");
			break;
		}
		fb_set_cmap(&info->cmap, info);

#ifdef CONFIG_FB_SH_MOBILE_DOUBLE_BUF
/* onscreen buffer 2 */
		info->fix.smem_start = lcd_ext_param[i].phy_addr;
		info->screen_base = (char __iomem *)lcd_ext_param[i].vir_addr;
#else
		memset(buf, 0, info->fix.smem_len);
		info->fix.smem_start = priv->ch[i].dma_handle;
		info->screen_base = buf;
#endif
		info->device = &pdev->dev;
		info->par = &priv->ch[i];
	}

	if (error)
		goto err1;

	for (i = 0; i < j; i++) {
		struct sh_mobile_lcdc_chan *ch = priv->ch + i;

		info = ch->info;

		error = register_framebuffer(info);
		if (error < 0)
			goto err1;

		dev_info(info->dev,
			 "registered %s/%s as %dx%d %dbpp.\n",
			 pdev->name,
			 (ch->cfg.chan == LCDC_CHAN_MAINLCD) ?
			 "mainlcd" : "sublcd",
			 (int) ch->cfg.lcd_cfg.xres,
			 (int) ch->cfg.lcd_cfg.yres,
			 ch->cfg.bpp);

		lcd_ext_param[i].aInfo = NULL;
		lcd_ext_param[i].lcd_type = ch->cfg.chan;
		sema_init(&lcd_ext_param[i].sem_lcd, 1);
	}

	return 0;
err1:
	sh_mobile_lcdc_remove(pdev);
err0:
	return error;
}

static int sh_mobile_lcdc_remove(struct platform_device *pdev)
{
	struct sh_mobile_lcdc_priv *priv = platform_get_drvdata(pdev);
	struct fb_info *info;
	int i;

	for (i = 0; i < ARRAY_SIZE(priv->ch); i++)
		if (priv->ch[i].info->dev)
			unregister_framebuffer(priv->ch[i].info);

	for (i = 0; i < ARRAY_SIZE(priv->ch); i++) {
		info = priv->ch[i].info;

		if (!info || !info->device)
			continue;

#ifdef CONFIG_FB_SH_MOBILE_DOUBLE_BUF
		if (lcd_ext_param[i].vir_addr != 0)
			iounmap((void __iomem *)lcd_ext_param[i].vir_addr);
#else
		if (info->screen_base != NULL)
			dma_free_coherent(&pdev->dev, info->fix.smem_len,
					  info->screen_base,
					  priv->ch[i].dma_handle);

		if (lcd_ext_param[i].vir_addr != 0)
			iounmap((void __iomem *)lcd_ext_param[i].vir_addr);

#endif
		fb_dealloc_cmap(&info->cmap);
		framebuffer_release(info);
	}

	kfree(priv);
	return 0;
}

static struct platform_driver sh_mobile_lcdc_driver = {
	.driver		= {
		.name		= "sh_mobile_lcdc_fb",
		.owner		= THIS_MODULE,
#if 0
		.pm		= &sh_mobile_lcdc_dev_pm_ops,
#endif
	},
	.probe		= sh_mobile_lcdc_probe,
	.remove		= sh_mobile_lcdc_remove,
};

static int __init sh_mobile_lcdc_init(void)
{
	return platform_driver_register(&sh_mobile_lcdc_driver);
}

static void __exit sh_mobile_lcdc_exit(void)
{
	platform_driver_unregister(&sh_mobile_lcdc_driver);
}

module_init(sh_mobile_lcdc_init);
module_exit(sh_mobile_lcdc_exit);

MODULE_DESCRIPTION("SuperH Mobile LCDC Framebuffer driver");
MODULE_AUTHOR("Magnus Damm <damm@opensource.se>");
MODULE_LICENSE("GPL v2");
