#ifndef __ASM_SH_MOBILE_LCDC_H__
#define __ASM_SH_MOBILE_LCDC_H__

#include <linux/fb.h>

/* Header Section */
struct lcdrt_sectioninfo {
	u32 variableareaaddr;	/* Address of Variable Area */
	u32 variableareasize;	/* Size of Variable Area */
	u32 fixedareaaddr;	/* Address of Fixed Area */
	u32 fixedareasize;	/* Size of Fixed Area */
	u32 commandareaaddr;	/* Address of Command Transfer Area */
	u32 commandareasize;	/* Size of Command Transfer Area */
	u32 onscrnaddr;		/* Address of OnScreen Buffer */
	u32 onscrnsize;		/* Size of OnScreen Buffer */
};

enum { RGB8,   /* 24bpp, 8:8:8 */
       RGB9,   /* 18bpp, 9:9 */
       RGB12A, /* 24bpp, 12:12 */
       RGB12B, /* 12bpp */
       RGB16,  /* 16bpp */
       RGB18,  /* 18bpp */
       RGB24,  /* 24bpp */
       SYS8A,  /* 24bpp, 8:8:8 */
       SYS8B,  /* 18bpp, 8:8:2 */
       SYS8C,  /* 18bpp, 2:8:8 */
       SYS8D,  /* 16bpp, 8:8 */
       SYS9,   /* 18bpp, 9:9 */
       SYS12,  /* 24bpp, 12:12 */
       SYS16A, /* 16bpp */
       SYS16B, /* 18bpp, 16:2 */
       SYS16C, /* 18bpp, 2:16 */
       SYS18,  /* 18bpp */
       SYS24 };/* 24bpp */

enum { LCDC_CHAN_DISABLED = 0,
       LCDC_CHAN_MAINLCD,
       LCDC_CHAN_SUBLCD };

enum { LCDC_CLK_BUS, LCDC_CLK_PERIPHERAL, LCDC_CLK_EXTERNAL };

#define LCDC_FLAGS_DWPOL (1 << 0) /* Rising edge dot clock data latch */
#define LCDC_FLAGS_DIPOL (1 << 1) /* Active low display enable polarity */
#define LCDC_FLAGS_DAPOL (1 << 2) /* Active low display data polarity */
#define LCDC_FLAGS_HSCNT (1 << 3) /* Disable HSYNC during VBLANK */
#define LCDC_FLAGS_DWCNT (1 << 4) /* Disable dotclock during blanking */

/*Main display*/
#define SH_MLCD_WIDTH		480
#define SH_MLCD_HEIGHT		854

#define SH_MLCD_TRCOLOR		0
#define SH_MLCD_REPLACECOLOR	0
#define SH_MLCD_RECTX		0
#define SH_MLCD_RECTY		0

/*Sub display*/
#define SH_SLCD_WIDTH		480
#define SH_SLCD_HEIGHT		854

#define SH_SLCD_TRCOLOR		0
#define SH_SLCD_REPLACECOLOR	0
#define SH_SLCD_RECTX		0
#define SH_SLCD_RECTY		0

#define FBIO_WAITFORVSYNC _IOW('F', 0x20, __u32)

struct sh_mobile_lcdc_lcd_size_cfg { /* width and height of panel in mm */
	unsigned long width;
	unsigned long height;
};

struct sh_mobile_lcdc_chan_cfg {
	int chan;
	int bpp;
	int interface_type; /* selects RGBn or SYSn I/F, see above */
	int clock_divider;
	unsigned long flags; /* LCDC_FLAGS_... */
	struct fb_videomode lcd_cfg;
	struct sh_mobile_lcdc_lcd_size_cfg lcd_size_cfg;
};

struct sh_mobile_lcdc_info {
	int clock_source;
	struct sh_mobile_lcdc_chan_cfg ch[2];
};

#include <rtapi/screen_display.h>
struct rtdisp_func {
	void* (*rtdisp_new)
	(
		void
	);

	int (*rtdisp_set_parameters)
	(
		screen_disp_param *disp_param
	);

	/* Get the initial address of on-screen */
	int (*rtdisp_get_address)
	(
		screen_disp_get_address *address
	);

	/* On-screen display update instruction */
	int (*rtdisp_draw)
	(
		screen_disp_draw *disp_draw
	);

	int (*rtdisp_start_lcd)
	(
		screen_disp_start_lcd *start_lcd
	);

	int (*rtdisp_stop_lcd)
	(
		screen_disp_stop_lcd *stop_lcd
	);

	int (*rtdisp_set_lcd_refresh)
	(
		screen_disp_set_lcd_refresh *set_lcd_refresh
	);

	int (*rtdisp_write_dsi_short_packet)
	(
		screen_disp_write_dsi_short *write_dsi_s
	);

	int (*rtdisp_write_dsi_long_packet)
	(
		screen_disp_write_dsi_long *write_dsi_l
	);

	int (*rtdisp_set_lcd_if_parameters)
	(
		screen_disp_set_lcd_if_param *set_lcd_if_param
	);
};

extern void register_disp_func(struct rtdisp_func *pfunc);
extern int sh_mobile_lcdc_keyclr_set(unsigned short s_key_clr,
				     unsigned short output_mode);
extern int sh_mobile_lcdc_alpha_set(unsigned short s_alpha,
				     unsigned short output_mode);
extern int sh_mobile_lcdc_refresh(unsigned short set_state,
				     unsigned short output_mode);

#endif /* __ASM_SH_MOBILE_LCDC_H__ */
