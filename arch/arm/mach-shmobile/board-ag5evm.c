/*
 * arch/arm/mach-shmobile/board-ag5evm.c
 *
 * Copyright (C) 2010  Takashi Yoshii <yoshii.takashi.zj@renesas.com>
 * Copyright (C) 2009  Yoshihiro Shimoda <shimoda.yoshihiro@renesas.com>
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
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/sh_clk.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>

#include <linux/serial_sci.h>
#include <linux/usb/r8a66597.h>
#include <linux/smsc911x.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/input/sh_keysc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sh_mmcif.h>
#include <linux/mfd/sh_mobile_sdhi.h>
#include <linux/mfd/tmio.h>
#include <linux/mtd/sh_flctl.h>
#include <linux/usb/android_composite.h>

#include <mach/hardware.h>
#include <mach/sh73a0.h>
#include <mach/common.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/hardware/gic.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/traps.h>

#include <sound/sh_fsi.h>
#include <video/sh_mobile_lcdc.h>

static struct mtd_partition nand_partition_info0[] = {
	{
		.name	= "SDA0",
		.offset	= 0,
		.size	= 1 << 20,
	},
	{
		.name	= "SDA1",
		.offset	= 1 << 20,
		.size	= LBA_NAND_SDA_SIZE - (1 << 20) - (2 << 20),
	},
};

static struct mtd_partition nand_partition_info1[] = {
	{
		.name	= "MDA0",
		.offset	= 0,
		.size	= 1 << 20,
	},
	{
		.name	= "MDA1",
		.offset	= 1 << 20,
		.size	= 31 << 20,
	},
	{
		.name	= "MDA2",
		.offset	= (1 << 20) + (31 << 20),
		/*
		 * Assign all the rest as "MDA2", and the total size is
		 * calculated by the following formula.  Make sure to double
		 * the number of sectors assigned to "Boot Block" and "SDA".
		 *
		 *  Total       Boot   SDA                   MDA0 MDA1
		 * {7920640 - ((4096 + 65536) * 2)} * 512 - {(1 + 32) << 20}
		 */
		.size	= 0xEB780000,
	},
};

static struct resource sh_flctl_resources[] = {
	[0] = {
		.start	= 0xee000000,
		.end	= 0xee00006f,
		.flags	= IORESOURCE_MEM,
	}
};

static struct sh_flctl_platform_data nand_flash_data0 = {
	.parts		= nand_partition_info0,
	.nr_parts	= ARRAY_SIZE(nand_partition_info0),
	.flcmncr_val	= BUSYON | SHBUSSEL | SEL_16BIT | SNAND_E |
			  ENDIAN | PULSE0 | CE1_ENABLE | TYPESEL_SET,
};

static struct sh_flctl_platform_data nand_flash_data1 = {
	.parts		= nand_partition_info1,
	.nr_parts	= ARRAY_SIZE(nand_partition_info1),
	.flcmncr_val	= BUSYON | SHBUSSEL | SEL_16BIT | SNAND_E |
			  ENDIAN | PULSE0 | CE1_ENABLE | TYPESEL_SET,
};

static struct platform_device sh_flctl_device[] = {
	[0] = {
		.name		= "sh_flctl",
		.id		= 0,
		.resource	= sh_flctl_resources,
		.num_resources	= ARRAY_SIZE(sh_flctl_resources),
		.dev		= {
			.platform_data = &nand_flash_data0,
		},
	},
	[1] = {
		.name		= "sh_flctl",
		.id		= 1,
		.resource	= sh_flctl_resources,
		.num_resources	= ARRAY_SIZE(sh_flctl_resources),
		.dev		= {
			.platform_data = &nand_flash_data1,
		},
	},
};

static struct r8a66597_platdata usb_host_data = {
	.on_chip	= 1,
	.dma_trans_byte	= 32,
};

static struct r8a66597_platdata usb_func_data = {
	.on_chip	= 1,
	.dma_trans_byte = 32,
};

static struct resource usb_resources[] = {
	[0] = {
		.name	= "USBHS",
		.start	= 0xe6890000,
		.end	= 0xe68900e6 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_spi(62) /* USBHS_USHI0 */,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.name	= "USBHS-DMA",
		.start	= 0xe68a0000,
		.end	= 0xe68a0064 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.start	= gic_spi(61) /* USBHS_DMAC1 */,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device usb_host_device = {
	.name		= "r8a66597_hcd",
	.id		= 0,
	.dev = {
		.platform_data		= &usb_host_data,
		.dma_mask		= NULL,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(usb_resources),
	.resource	= usb_resources,
};

static struct platform_device usb_func_device = {
	.name	= "r8a66597_udc",
	.id	= 0,
	.dev = {
		.dma_mask		= NULL,         /*  not use dma */
		.coherent_dma_mask	= 0xffffffff,
		.platform_data		= &usb_func_data,
	},
	.num_resources	= ARRAY_SIZE(usb_resources),
	.resource	= usb_resources,
};

static struct resource smsc9220_resources[] = {
	[0] = {
		.start		= 0x14000000,
		.end		= 0x14000000 + SZ_64K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= pint2irq(2), /* PINTA2 */
		.flags		= IORESOURCE_IRQ | IRQ_TYPE_LEVEL_LOW,
	},
};

static struct smsc911x_platform_config smsc9220_platdata = {
	.flags		= SMSC911X_USE_32BIT | SMSC911X_SAVE_MAC_ADDRESS,
	.phy_interface	= PHY_INTERFACE_MODE_MII,
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
};

static struct platform_device eth_device = {
	.name		= "smsc911x",
	.id		= 0,
	.dev  = {
		.platform_data = &smsc9220_platdata,
	},
	.resource	= smsc9220_resources,
	.num_resources	= ARRAY_SIZE(smsc9220_resources),
};

static struct sh_keysc_info keysc_platdata = {
	.mode		= SH_KEYSC_MODE_6_8x8,
	.scan_timing	= 3,
	.delay		= 100,
	.keycodes	= {
		KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_VOLUMEDOWN,
		KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_VOLUMEUP,
		KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, 0,
		KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_HOME, KEY_SLEEP, 0,
		KEY_SPACE, KEY_9, KEY_6, KEY_3, KEY_WAKEUP, KEY_RIGHT, \
		KEY_COFFEE, 0,
		KEY_0, KEY_8, KEY_5, KEY_2, KEY_DOWN, KEY_ENTER, KEY_UP, KEY_F1,
		KEY_KPASTERISK, KEY_7, KEY_4, KEY_1, KEY_STOP, KEY_LEFT, \
		KEY_COMPUTER, KEY_BACK,
	},
};

static struct resource keysc_resources[] = {
	[0] = {
		.name	= "KEYSC",
		.start	= 0xe61b0000,
		.end	= 0xe61b0098 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_spi(71),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device keysc_device = {
	.name		= "sh_keysc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(keysc_resources),
	.resource	= keysc_resources,
	.dev		= {
		.platform_data	= &keysc_platdata,
	},
};

static struct sh_mobile_sdhi_info sdhi0_info = {
	.dma_slave_tx	= SHDMA_SLAVE_SDHI0_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SDHI0_RX,
	.tmio_caps	= MMC_CAP_SD_HIGHSPEED,
};

static struct resource sdhi0_resources[] = {
	[0] = {
		.name	= "SDHI0",
		.start	= 0xee100000,
		.end	= 0xee100fff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_spi(83),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device sdhi0_device = {
	.name		= "sh_mobile_sdhi",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(sdhi0_resources),
	.resource	= sdhi0_resources,
	.dev	= {
		.platform_data	= &sdhi0_info,
	},
};

void ag5evm_sdhi1_set_pwr(struct platform_device *pdev, int state)
{
	gpio_set_value(GPIO_PORT114, state);
}

static struct sh_mobile_sdhi_info sh_sdhi1_platdata = {
	.tmio_flags	= TMIO_MMC_WRPROTECT_DISABLE,
	.tmio_caps	= MMC_CAP_NONREMOVABLE,
	.tmio_ocr_mask	= MMC_VDD_32_33 | MMC_VDD_33_34,
	.set_pwr	= ag5evm_sdhi1_set_pwr,
};

static struct resource sdhi1_resources[] = {
	[0] = {
		.name	= "SDHI1",
		.start	= 0xee120000,
		.end	= 0xee120fff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= soft_irq(0),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device sdhi1_device = {
	.name		= "sh_mobile_sdhi",
	.id		= 1,
	.dev		= {
		.platform_data	= &sh_sdhi1_platdata,
	},
	.num_resources	= ARRAY_SIZE(sdhi1_resources),
	.resource	= sdhi1_resources,
};

/* FSI A */
/*
 * FSI-A use external clock which came from da7210.
 * So, we should change parent of fsi
 */
#define FSIACKCR	0xE6150018
static void fsiack_init(struct clk *clk)
{
	u32 status;
	void __iomem *reg;

	reg = ioremap_nocache(FSIACKCR, 4);
	if (!reg) {
		pr_err("ioremap failed for FSI A\n");
		return;
	}

	status = __raw_readl(reg);

	/* use external clock */
	status &= ~0x000000ff;
	status |= 0x00000080;

	__raw_writel(status, reg);

	iounmap(reg);
}

static struct clk_ops fsiack_clk_ops = {
	.init = fsiack_init,
};

static struct clk fsiack_clk = {
	.ops		= &fsiack_clk_ops,
	.rate		= 0, /* unknown */
};

static struct sh_fsi_platform_info fsi_info = {
	.porta_flags = SH_FSI_OUT_SLAVE_MODE	|
		       SH_FSI_IN_SLAVE_MODE	|
		       SH_FSI_OFMT(I2S)		|
		       SH_FSI_IFMT(I2S),
};

static struct resource fsi_resources[] = {
	[0] = {
		.name	= "FSI",
		.start	= 0xEC230000,
		.end	= 0xEC230400 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start  = gic_spi(146),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device fsi_device = {
	.name		= "sh_fsi2",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(fsi_resources),
	.resource	= fsi_resources,
	.dev	= {
		.platform_data	= &fsi_info,
	},
};

static struct i2c_board_info i2c0_devices[] = {
	{
		I2C_BOARD_INFO("ag5evm_ts", 0x20),
		.irq	= pint2irq(19),	/* PINTC3 */
	},
};

static struct i2c_board_info i2c1_devices[] = {
	{
		I2C_BOARD_INFO("led", 0x6d),
	},
};

static struct i2c_board_info i2c2_devices[] = {
	{
		I2C_BOARD_INFO("kodoh-codec", 0x63),
	},
	{
		I2C_BOARD_INFO("kodoh-rtc", 0x32),
	},
};

static struct resource sh_mmcif_resources[] = {
	[0] = {
		.name	= "MMCIF",
		.start	= 0xe6bd0000,
		.end	= 0xe6bd00ff,
		.flags	= IORESOURCE_MEM,
	},
	/*
	 * XXX: INTC spec (7.7 SPI) has a document error on MMC interrupts.
	 * Normal interrupt (MMC NOR) is supposed to be assigned to spi(141),
	 * and error interrupt (MMC ERR) is to spi(140), respectively.
	 */
	[1] = {
		/* MMC ERR */
		.start	= gic_spi(140),
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		/* MMC NOR */
		.start	= gic_spi(141),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct sh_mmcif_dma sh_mmcif_dma = {
	.chan_priv_rx	= {
		.slave_id	= SHDMA_SLAVE_MMCIF_RX,
	},
	.chan_priv_tx	= {
		.slave_id	= SHDMA_SLAVE_MMCIF_TX,
	},
};

static struct sh_mmcif_plat_data sh_mmcif_plat = {
	.sup_pclk	= 0,
	.ocr		= MMC_VDD_165_195,
	.caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_NONREMOVABLE,
	.dma		= &sh_mmcif_dma,
};

static struct platform_device sh_mmcif_device = {
	.name		= "sh_mmcif",
	.id		= 0,
	.dev		= {
		.platform_data	= &sh_mmcif_plat,
	},
	.num_resources	= ARRAY_SIZE(sh_mmcif_resources),
	.resource	= sh_mmcif_resources,
};

/* Android USB gadget  */
static char *usb_functions_ums[] = { "usb_mass_storage" };
static char *usb_functions_ums_adb[] = { "usb_mass_storage", "adb" };

static struct android_usb_product usb_products[] = {
	{
		.product_id	= 0x0001,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= 0x0002,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x18d1,
	.product_id	= 0x0001,
	.version	= 0x0100,
	.product_name		= "AG5EVM",
	.manufacturer_name	= "Renesas",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_ums_adb),
	.functions = usb_functions_ums_adb,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data	= &android_usb_pdata,
	},
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "Renesas",
	.product	= "AG5EVM",
	.release	= 0x0100,
};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

static struct sh_mobile_lcdc_info lcdc_info = {
	.clock_source	= 0x01,

	/* LCDC0 */
	.ch[0] = {
		.chan = LCDC_CHAN_MAINLCD,
#ifdef CONFIG_FB_SH_MOBILE_ARGB8888
		.bpp = 32,
#else
		.bpp = 16,
#endif
		.interface_type		= RGB24,
		.clock_divider		= 1,
		.flags			= LCDC_FLAGS_DWPOL,
		.lcd_cfg = {
			.name		= "WVGA",
			.xres		= SH_MLCD_WIDTH,
			.yres		= SH_MLCD_HEIGHT,
			.left_margin	= 64,
			.right_margin	= 8,
			.hsync_len	= 16,
			.upper_margin	= 1,
			.lower_margin	= 168,
			.vsync_len	= 1,
			.sync		= 0,
		},
		.lcd_size_cfg = {
			.width	= 44,
			.height	= 79,
		},
	},
};

static struct resource lcdc_resources[] = {
	[0] = {
		.name	= "LCDC",
		.start	= 0xee940000,
		.end	= 0xee943fff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= intcs_evt2irq(0x580),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device lcdc_device = {
	.name		= "sh_mobile_lcdc_fb",
	.num_resources	= ARRAY_SIZE(lcdc_resources),
	.resource	= lcdc_resources,
	.dev	= {
		.platform_data  = &lcdc_info,
		.coherent_dma_mask = ~0,
	},
};

static struct resource mfis_resources[] = {
	[0] = {
		.name   = "MFIS",
		.start  = gic_spi(58),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device mfis_device = {
	.name           = "mfis",
	.id                     = 0,
	.resource       = mfis_resources,
	.num_resources  = ARRAY_SIZE(mfis_resources),
};

static struct platform_device *ag5evm_devices[] __initdata = {
	&usb_host_device,
	&usb_func_device,
	&eth_device,
	&keysc_device,
	&sdhi0_device,
	&sdhi1_device,
	&fsi_device,
	&sh_mmcif_device,
	&sh_flctl_device[0],
	&sh_flctl_device[1],
	&lcdc_device,
	&mfis_device,

	&usb_mass_storage_device,
	&android_usb_device,
};

static struct map_desc ag5evm_io_desc[] __initdata = {
	/* create a 1:1 entity map for 0xe6xxxxxx
	 * used by CPGA, INTC and PFC.
	 */
	{
		.virtual	= 0xe6000000,
		.pfn		= __phys_to_pfn(0xe6000000),
		.length		= 256 << 20,
		.type		= MT_DEVICE_NONSHARED
	},
	{
		/*
		 * Create 4MB of virtual address hole within a big 1:1 map
		 * requested above, which is dedicated for display drivers.
		 *
		 * According to the hardware manuals, physical 0xefc00000
		 * space is reserved for Router and a data abort error will
		 * be generated if access is made there.  So this partial
		 * mapping change won't be a problem.
		 */
		.virtual        = 0xefc00000,
		.pfn            = __phys_to_pfn(0xffc00000),
		.length         = 0x00400000,
		.type           = MT_DEVICE_NONSHARED
	},
};

static void __init ag5evm_map_io(void)
{
	iotable_init(ag5evm_io_desc, ARRAY_SIZE(ag5evm_io_desc));

	/* setup early devices and console here as well */
	sh73a0_add_early_devices();
	shmobile_setup_console();
}

static void mpx_enable_irq(unsigned int irq)
{
	irq_to_desc(gic_spi(87))->chip->enable(gic_spi(87));
	irq_to_desc(gic_spi(88))->chip->enable(gic_spi(88));
	irq_to_desc(gic_spi(89))->chip->enable(gic_spi(89));
}

static void mpx_disable_irq(unsigned int irq)
{
	irq_to_desc(gic_spi(87))->chip->disable(gic_spi(87));
	irq_to_desc(gic_spi(88))->chip->disable(gic_spi(88));
	irq_to_desc(gic_spi(89))->chip->disable(gic_spi(89));
}

static unsigned int mpx_startup_irq(unsigned int irq)
{
	/* enable them formerly started with NOAUTOEN */
	irq_to_desc(gic_spi(87))->depth = 0;
	irq_to_desc(gic_spi(87))->status &= ~IRQ_DISABLED;
	irq_to_desc(gic_spi(87))->chip->startup(gic_spi(87));

	irq_to_desc(gic_spi(88))->depth = 0;
	irq_to_desc(gic_spi(88))->status &= ~IRQ_DISABLED;
	irq_to_desc(gic_spi(88))->chip->startup(gic_spi(88));

	irq_to_desc(gic_spi(89))->depth = 0;
	irq_to_desc(gic_spi(89))->status &= ~IRQ_DISABLED;
	/* Disable this for now
	irq_to_desc(gic_spi(89))->chip->startup(gic_spi(89));
	*/
	return 0;
}

static struct irq_chip mpx_chip = {
	.name	= "sdhi-mpx",
	.enable = mpx_enable_irq,
	.disable = mpx_disable_irq,
	.startup = mpx_startup_irq,
};

/* reroute SDHI0.SPI(84) to TMIO_MMC.0(SPI(83))) */
static irqreturn_t sdhi0_mpx_interrupt(int irq, void *dev_id)
{
	generic_handle_irq(gic_spi(83));
	return IRQ_HANDLED;
}

/**************************************/
/* Control LCD backlight (default on) */
#include <linux/leds.h>

#define HW_MAX_BRIGHTNESS 0x80

static struct i2c_client *led_client;

static void
led_backlight_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	int hw_val = (value * HW_MAX_BRIGHTNESS) / LED_FULL;

	if (hw_val > HW_MAX_BRIGHTNESS)
		hw_val = HW_MAX_BRIGHTNESS;

	i2c_smbus_write_byte_data(led_client, 0x23, hw_val & 0xff);

	if (value == LED_OFF)
		i2c_smbus_write_byte_data(led_client, 0x03, 0x00);
	else
		i2c_smbus_write_byte_data(led_client, 0x03, 0x01);
}

static struct led_classdev led_backlight = {
	.name = "lcd-backlight",
	.brightness = LED_FULL,
	.brightness_set = led_backlight_set,
};

static int led_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	led_client = client;

	/* Unreset LED controler Reset */
	gpio_request(GPIO_PORT235, NULL);
	gpio_direction_output(GPIO_PORT235, 0);
	udelay(1);
	gpio_set_value(GPIO_PORT235, 1);

	i2c_smbus_write_byte_data(client, 0x04, 0x07);
	i2c_smbus_write_byte_data(client, 0x23, 0x80);
	i2c_smbus_write_byte_data(client, 0x03, 0x01);
	i2c_smbus_write_byte_data(client, 0x00, 0x01); /* ts power-on */

	return led_classdev_register(&client->dev, &led_backlight);
}

static struct i2c_device_id led_idtable[] = {
	{"led", 0},
	{ },
};

static struct i2c_driver led_drv = {
	.driver		= {
		.name	= "led driver",
	},
	.probe		= led_probe,
	.id_table	= led_idtable,
};

static __init int led_init(void)
{
	return i2c_add_driver(&led_drv);
}
device_initcall(led_init);
/**************************************/

/* reroute SDHI1(SPI(87-89)) to TMIO_MMC.1(soft_irq(0)) */
static irqreturn_t sdhi1_mpx_interrupt(int irq, void *dev_id)
{
	generic_handle_irq(soft_irq(0));
	return IRQ_HANDLED;
}

void __init ag5evm_init_irq(void)
{
	gic_dist_init(0, __io(0xf0001000), 29);
	gic_cpu_init(0, __io(0xf0000100));

	sh73a0_init_irq();

	/* chip enable/disable multiplyer
	 * do gang enable/disable by soft_irq(0) */
	irq_to_desc_alloc_node(soft_irq(0), smp_processor_id());
	dynamic_irq_init(soft_irq(0));
	set_irq_chip_and_handler(soft_irq(0), &mpx_chip, handle_simple_irq);
	set_irq_flags(soft_irq(0), IRQF_VALID);

	/* disable them until TMIO_MMC startup */
	irq_to_desc(gic_spi(87))->status |= IRQ_NOAUTOEN;
	irq_to_desc(gic_spi(88))->status |= IRQ_NOAUTOEN;
	irq_to_desc(gic_spi(89))->status |= IRQ_NOAUTOEN;
	/* to reroute SDHI1 to TMIO_MMC */
	if (request_irq(gic_spi(87), sdhi1_mpx_interrupt, 0, "sdhi1_0", 0))
		pr_warning("Failed to get sdhi1_0 irq\n");
	if (request_irq(gic_spi(88), sdhi1_mpx_interrupt, 0, "sdhi1_1", 0))
		pr_warning("Failed to get sdhi1_1 irq\n");
	if (request_irq(gic_spi(89), sdhi1_mpx_interrupt, 0, "sdhi1_2", 0))
		pr_warning("Failed to get sdhi1_2 irq\n");
}

#define FLCKCR		0xe6150014
#define SUBCKCR		0xe6150080
#define SRCR2		0xe61580b0

static void __init ag5evm_init(void)
{
	struct clk *sub_clk = clk_get(NULL, "sub_clk");
	struct clk *extal2_clk = clk_get(NULL, "extal2");
	struct clk *fsia_clk = clk_get(NULL, "fsia_clk");
	clk_set_parent(sub_clk, extal2_clk);

	__raw_writel(__raw_readl(SUBCKCR) & ~(1<<9), SUBCKCR);
	__raw_writel(__raw_readl(SUBCKCR) | (1<<7), SUBCKCR);

	sh73a0_pinmux_init();

	/* enable SCIFA2 */
	gpio_request(GPIO_FN_SCIFA2_TXD1, NULL);
	gpio_request(GPIO_FN_SCIFA2_RXD1, NULL);
	gpio_request(GPIO_FN_SCIFA2_RTS1_, NULL);
	gpio_request(GPIO_FN_SCIFA2_CTS1_, NULL);

	/* enable SCIFB */
	gpio_request(GPIO_FN_PORT159_SCIFB_SCK, NULL);
	gpio_request(GPIO_FN_PORT160_SCIFB_TXD, NULL);
	gpio_request(GPIO_FN_PORT161_SCIFB_CTS_, NULL);
	gpio_request(GPIO_FN_PORT162_SCIFB_RXD, NULL);
	gpio_request(GPIO_FN_PORT163_SCIFB_RTS_, NULL);

	/* enable SDHI0 */
	gpio_request(GPIO_FN_SDHICD0, NULL);
	gpio_request(GPIO_FN_SDHIWP0, NULL);
	gpio_request(GPIO_FN_SDHICMD0, NULL);
	gpio_request(GPIO_FN_SDHICLK0, NULL);
	gpio_request(GPIO_FN_SDHID0_3, NULL);
	gpio_request(GPIO_FN_SDHID0_2, NULL);
	gpio_request(GPIO_FN_SDHID0_1, NULL);
	gpio_request(GPIO_FN_SDHID0_0, NULL);

	/* enable SDHI1 */
	gpio_request(GPIO_FN_SDHICLK1, NULL);
	gpio_request(GPIO_FN_SDHICMD1_PU, NULL);
	gpio_request(GPIO_FN_SDHID1_3_PU, NULL);
	gpio_request(GPIO_FN_SDHID1_2_PU, NULL);
	gpio_request(GPIO_FN_SDHID1_1_PU, NULL);
	gpio_request(GPIO_FN_SDHID1_0_PU, NULL);
	gpio_request(GPIO_PORT114, "sdhi1_power");
	gpio_direction_output(GPIO_PORT114, 0);

	/* enable MMCIF */
	gpio_request(GPIO_FN_MMCCLK0, NULL);
	gpio_request(GPIO_FN_MMCD0_0, NULL);
	gpio_request(GPIO_FN_MMCD0_1, NULL);
	gpio_request(GPIO_FN_MMCD0_2, NULL);
	gpio_request(GPIO_FN_MMCD0_3, NULL);
	gpio_request(GPIO_FN_MMCD0_4, NULL);
	gpio_request(GPIO_FN_MMCD0_5, NULL);
	gpio_request(GPIO_FN_MMCD0_6, NULL);
	gpio_request(GPIO_FN_MMCD0_7, NULL);
	gpio_request(GPIO_FN_MMCCMD0, NULL);
	gpio_request(GPIO_PORT208, NULL); /* Reset */
	gpio_direction_output(GPIO_PORT208, 1);

	/* enable KEYSC */
	clk_enable(clk_get(NULL, "keysc0"));
	gpio_request(GPIO_FN_KEYIN0_PU, NULL);
	gpio_request(GPIO_FN_KEYIN1_PU, NULL);
	gpio_request(GPIO_FN_KEYIN2_PU, NULL);
	gpio_request(GPIO_FN_KEYIN3_PU, NULL);
	gpio_request(GPIO_FN_KEYIN4_PU, NULL);
	gpio_request(GPIO_FN_KEYIN5_PU, NULL);
	gpio_request(GPIO_FN_KEYIN6_PU, NULL);
	gpio_request(GPIO_FN_KEYIN7_PU, NULL);
	gpio_request(GPIO_FN_KEYOUT0, NULL);
	gpio_request(GPIO_FN_KEYOUT1, NULL);
	gpio_request(GPIO_FN_KEYOUT2, NULL);
	gpio_request(GPIO_FN_KEYOUT3, NULL);
	gpio_request(GPIO_FN_KEYOUT4, NULL);
	gpio_request(GPIO_FN_KEYOUT5, NULL);
	gpio_request(GPIO_FN_PORT59_KEYOUT6, NULL);
	gpio_request(GPIO_FN_PORT58_KEYOUT7, NULL);
	gpio_request(GPIO_FN_KEYOUT8, NULL);
	gpio_request(GPIO_FN_PORT149_KEYOUT9, NULL);

	/* enable I2C channel 2 and 3 */
	gpio_request(GPIO_FN_PORT236_I2C_SDA2, NULL);
	gpio_request(GPIO_FN_PORT237_I2C_SCL2, NULL);
	gpio_request(GPIO_FN_PORT248_I2C_SCL3, NULL);
	gpio_request(GPIO_FN_PORT249_I2C_SDA3, NULL);

	/* enable USBHS host/function */
	gpio_request(GPIO_FN_VBUS_0, NULL);

	/* wake usb phy on */
	gpio_request(GPIO_PORT24, NULL);
	gpio_direction_output(GPIO_PORT24, 0);
	udelay(10);
	gpio_set_value(GPIO_PORT24, 1);

	/* enable SMSC911X */
	gpio_request(GPIO_PORT144, NULL); /* PINTA2 */
	gpio_direction_input(GPIO_PORT144);
	gpio_request(GPIO_PORT145, NULL); /* RESET */
	gpio_direction_output(GPIO_PORT145, 1);

	/* FSI A */
	clk_register(&fsiack_clk);
	clk_set_parent(fsia_clk, &fsiack_clk);
	clk_put(fsia_clk);
	clk_enable(clk_get(NULL, "fsi"));
	gpio_request(GPIO_FN_FSIACK, NULL);
	gpio_request(GPIO_FN_FSIAILR_PU, NULL);
	gpio_request(GPIO_FN_FSIAIBT_PU, NULL);
	gpio_request(GPIO_FN_FSIAISLD_PU, NULL);
	gpio_request(GPIO_FN_FSIAOSLD, NULL);

#ifdef CONFIG_MTD_NAND_SH_FLCTL
	gpio_request(GPIO_FN_FCE1_, NULL);
	__raw_writel(0x05, FLCKCR);
#endif

	/* Unreset LCD Panel */
	gpio_request(GPIO_PORT217, NULL);
	gpio_direction_output(GPIO_PORT217, 0);
	udelay(1);
	gpio_set_value(GPIO_PORT217, 1);

	/* enable touchscreen */
	gpio_request(GPIO_PORT12, NULL); /* RESET */
	gpio_direction_output(GPIO_PORT12, 0);
	gpio_request(GPIO_FN_SCIFA0_RTS_, NULL); /* PINTC3 */

#ifdef CONFIG_CACHE_L2X0
	/* Early BRESP enable, Shared attribute override enable, 64K*8way */
	l2x0_init(__io(0xf0100000), 0x40460000, 0x82000fff);
#endif
	/* multiplex irqs to sdhi0 */
	if (request_irq(gic_spi(84), sdhi0_mpx_interrupt, IRQF_DISABLED,
		"mpx", 0))
		pr_warning("Failed to get multiplex irq.");

	/* Clear software reset bit on SY-DMAC module */
	__raw_writel(__raw_readl(SRCR2) & ~(1 << 18), SRCR2);

	sh73a0_add_standard_devices();

	i2c_register_board_info(0, i2c0_devices, ARRAY_SIZE(i2c0_devices));
	i2c_register_board_info(1, i2c1_devices, ARRAY_SIZE(i2c1_devices));
	i2c_register_board_info(2, i2c2_devices, ARRAY_SIZE(i2c2_devices));
	platform_add_devices(ag5evm_devices, ARRAY_SIZE(ag5evm_devices));
}

static void __init ag5evm_timer_init(void)
{
	sh73a0_clock_init();
	shmobile_timer.init();
	return;
}

struct sys_timer ag5evm_timer = {
	.init	= ag5evm_timer_init,
};

MACHINE_START(AG5EVM, "ag5evm")
	.phys_io	= 0xe5580000,
	.io_pg_offst	= ((0xe5580000) >> 18) & 0xfffc,
	.map_io		= ag5evm_map_io,
	.init_irq	= ag5evm_init_irq,
	.init_machine	= ag5evm_init,
	.timer		= &ag5evm_timer,
MACHINE_END
