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

static struct r8a66597_platdata usb_data = {
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

static struct platform_device usb_func_device = {
	.name	= "r8a66597_udc",
	.id	= 0,
	.dev = {
		.dma_mask		= NULL,         /*  not use dma */
		.coherent_dma_mask	= 0xffffffff,
		.platform_data		= &usb_data,
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
		.start		= gic_spi(33), /* PINT1 */
		.flags		= IORESOURCE_IRQ,
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
		KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, 0,
		KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, 0,
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

static struct i2c_board_info i2c2_devices[] = {
	{
		I2C_BOARD_INFO("kodoh", 0x63),
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

static struct sh_mmcif_plat_data sh_mmcif_plat = {
	.sup_pclk	= 0,
	.ocr		= MMC_VDD_165_195 | MMC_VDD_32_33 | MMC_VDD_33_34,
	.caps		= MMC_CAP_4_BIT_DATA |
			  MMC_CAP_8_BIT_DATA,
	.dma_slave_tx	= SHDMA_SLAVE_MMCIF_TX,
	.dma_slave_rx	= SHDMA_SLAVE_MMCIF_RX,
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

static struct platform_device *ag5evm_devices[] __initdata = {
	&usb_func_device,
	&eth_device,
	&keysc_device,
	&sdhi0_device,
	&fsi_device,
	&sh_mmcif_device,

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
};

static void __init ag5evm_map_io(void)
{
	iotable_init(ag5evm_io_desc, ARRAY_SIZE(ag5evm_io_desc));

	/* setup early devices and console here as well */
	sh73a0_add_early_devices();
	shmobile_setup_console();
}

static irqreturn_t sdhi0_mpx_interrupt(int irq, void *dev_id)
{
	generic_handle_irq(gic_spi(83));
	return IRQ_HANDLED;
}

#define PINTC_ADDR	0xe6900000
#define PINTER0A	(PINTC_ADDR + 0xa0)
#define PINTCR0A	(PINTC_ADDR + 0xb0)

void __init ag5evm_init_irq(void)
{
	/* setup PINT: enable PINTA2 as active low */
	__raw_writel(__raw_readl(PINTER0A) | (1<<29), PINTER0A);
	__raw_writew(__raw_readw(PINTCR0A) | (2<<10), PINTCR0A);

	gic_dist_init(0, __io(0xf0001000), 29);
	gic_cpu_init(0, __io(0xf0000100));

	sh73a0_init_irq();
}

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

	/* enable SDHI0 */
	gpio_request(GPIO_FN_SDHICD0, NULL);
	gpio_request(GPIO_FN_SDHIWP0, NULL);
	gpio_request(GPIO_FN_SDHICMD0, NULL);
	gpio_request(GPIO_FN_SDHICLK0, NULL);
	gpio_request(GPIO_FN_SDHID0_3, NULL);
	gpio_request(GPIO_FN_SDHID0_2, NULL);
	gpio_request(GPIO_FN_SDHID0_1, NULL);
	gpio_request(GPIO_FN_SDHID0_0, NULL);

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

	/* enable IC2 2 and 3 */
	gpio_request(GPIO_FN_PORT236_I2C_SDA2, NULL);
	gpio_request(GPIO_FN_PORT237_I2C_SCL2, NULL);
	gpio_request(GPIO_FN_PORT248_I2C_SCL3, NULL);
	gpio_request(GPIO_FN_PORT249_I2C_SDA3, NULL);

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

#ifdef CONFIG_CACHE_L2X0
	/* Shared attribute override enable, 64K*8way */
	l2x0_init(__io(0xf0100000), 0x00460000, 0xc2000fff);
#endif
	/* multiplex irqs to sdhi0 */
	if (request_irq(gic_spi(84), sdhi0_mpx_interrupt, IRQF_DISABLED,
		"mpx", 0))
		pr_warning("Failed to get multiplex irq.");

	/* Clear software reset bit on SY-DMAC module */
	__raw_writel(__raw_readl(SRCR2) & ~(1 << 18), SRCR2);

	sh73a0_add_standard_devices();

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
