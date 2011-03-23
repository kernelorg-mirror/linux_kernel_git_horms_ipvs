/*
 * sh73a0 processor support
 *
 * Copyright (C) 2010  Takashi Yoshii
 * Copyright (C) 2010  Magnus Damm
 * Copyright (C) 2008  Yoshihiro Shimoda
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
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/serial_sci.h>
#include <linux/sh_dma.h>
#include <linux/sh_intc.h>
#include <linux/sh_timer.h>
#include <linux/i2c-sh_mobile.h>
#include <mach/hardware.h>
#include <mach/sh73a0.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

static struct platform_device dma_device;

static struct plat_sci_port scif0_platform_data = {
	.mapbase	= 0xe6c40000,
	.flags		= UPF_BOOT_AUTOCONF,
	.type		= PORT_SCIFA,
	.irqs		= { gic_spi(72), gic_spi(72),
			    gic_spi(72), gic_spi(72) },
#ifdef CONFIG_SERIAL_SH_SCI_DMA
	.dma_dev	= &dma_device.dev,
	.dma_slave_tx	= SHDMA_SLAVE_SCIF0_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SCIF0_RX,
#endif
};

static struct platform_device scif0_device = {
	.name		= "sh-sci",
	.id		= 0,
	.dev		= {
		.platform_data	= &scif0_platform_data,
		.dma_mask	= NULL,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct plat_sci_port scif1_platform_data = {
	.mapbase	= 0xe6c50000,
	.flags		= UPF_BOOT_AUTOCONF,
	.type		= PORT_SCIFA,
	.irqs		= { gic_spi(73), gic_spi(73),
			    gic_spi(73), gic_spi(73) },
#ifdef CONFIG_SERIAL_SH_SCI_DMA
	.dma_dev	= &dma_device.dev,
	.dma_slave_tx	= SHDMA_SLAVE_SCIF1_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SCIF1_RX,
#endif
};

static struct platform_device scif1_device = {
	.name		= "sh-sci",
	.id		= 1,
	.dev		= {
		.platform_data	= &scif1_platform_data,
		.dma_mask	= NULL,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct plat_sci_port scif2_platform_data = {
	.mapbase	= 0xe6c60000,
	.flags		= UPF_BOOT_AUTOCONF,
	.type		= PORT_SCIFA,
	.irqs		= { gic_spi(74), gic_spi(74),
			    gic_spi(74), gic_spi(74) },
#ifdef CONFIG_SERIAL_SH_SCI_DMA
	.dma_dev	= &dma_device.dev,
	.dma_slave_tx	= SHDMA_SLAVE_SCIF2_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SCIF2_RX,
#endif
};

static struct platform_device scif2_device = {
	.name		= "sh-sci",
	.id		= 2,
	.dev		= {
		.platform_data	= &scif2_platform_data,
		.dma_mask	= NULL,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct plat_sci_port scif3_platform_data = {
	.mapbase	= 0xe6c70000,
	.flags		= UPF_BOOT_AUTOCONF,
	.type		= PORT_SCIFA,
	.irqs		= { gic_spi(75), gic_spi(75),
			    gic_spi(75), gic_spi(75) },
#ifdef CONFIG_SERIAL_SH_SCI_DMA
	.dma_dev	= &dma_device.dev,
	.dma_slave_tx	= SHDMA_SLAVE_SCIF3_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SCIF3_RX,
#endif
};

static struct platform_device scif3_device = {
	.name		= "sh-sci",
	.id		= 3,
	.dev		= {
		.platform_data	= &scif3_platform_data,
		.dma_mask	= NULL,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct plat_sci_port scif4_platform_data = {
	.mapbase	= 0xe6c80000,
	.flags		= UPF_BOOT_AUTOCONF,
	.type		= PORT_SCIFA,
	.irqs		= { gic_spi(78), gic_spi(78),
			    gic_spi(78), gic_spi(78) },
#ifdef CONFIG_SERIAL_SH_SCI_DMA
	.dma_dev	= &dma_device.dev,
	.dma_slave_tx	= SHDMA_SLAVE_SCIF4_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SCIF4_RX,
#endif
};

static struct platform_device scif4_device = {
	.name		= "sh-sci",
	.id		= 4,
	.dev		= {
		.platform_data	= &scif4_platform_data,
		.dma_mask	= NULL,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct plat_sci_port scif5_platform_data = {
	.mapbase	= 0xe6cb0000,
	.flags		= UPF_BOOT_AUTOCONF,
	.type		= PORT_SCIFA,
	.irqs		= { gic_spi(79), gic_spi(79),
			    gic_spi(79), gic_spi(79) },
#ifdef CONFIG_SERIAL_SH_SCI_DMA
	.dma_dev	= &dma_device.dev,
	.dma_slave_tx	= SHDMA_SLAVE_SCIF5_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SCIF5_RX,
#endif
};

static struct platform_device scif5_device = {
	.name		= "sh-sci",
	.id		= 5,
	.dev		= {
		.platform_data	= &scif5_platform_data,
		.dma_mask	= NULL,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct plat_sci_port scif6_platform_data = {
	.mapbase	= 0xe6cc0000,
	.flags		= UPF_BOOT_AUTOCONF,
	.type		= PORT_SCIFA,
	.irqs		= { gic_spi(156), gic_spi(156),
			    gic_spi(156), gic_spi(156) },
#ifdef CONFIG_SERIAL_SH_SCI_DMA
	.dma_dev	= &dma_device.dev,
	.dma_slave_tx	= SHDMA_SLAVE_SCIF6_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SCIF6_RX,
#endif
};

static struct platform_device scif6_device = {
	.name		= "sh-sci",
	.id		= 6,
	.dev		= {
		.platform_data	= &scif6_platform_data,
		.dma_mask	= NULL,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct plat_sci_port scif7_platform_data = {
	.mapbase	= 0xe6cd0000,
	.flags		= UPF_BOOT_AUTOCONF,
	.type		= PORT_SCIFA,
	.irqs		= { gic_spi(143), gic_spi(143),
			    gic_spi(143), gic_spi(143) },
#ifdef CONFIG_SERIAL_SH_SCI_DMA
	.dma_dev	= &dma_device.dev,
	.dma_slave_tx	= SHDMA_SLAVE_SCIF7_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SCIF7_RX,
#endif
};

static struct platform_device scif7_device = {
	.name		= "sh-sci",
	.id		= 7,
	.dev		= {
		.platform_data	= &scif7_platform_data,
		.dma_mask	= NULL,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct plat_sci_port scif8_platform_data = {
	.mapbase	= 0xe6c30000,
	.flags		= UPF_BOOT_AUTOCONF,
	.type		= PORT_SCIFB,
	.irqs		= { gic_spi(80), gic_spi(80),
			    gic_spi(80), gic_spi(80) },
#ifdef CONFIG_SERIAL_SH_SCI_DMA
	.dma_dev	= &dma_device.dev,
	.dma_slave_tx	= SHDMA_SLAVE_SCIF8_TX,
	.dma_slave_rx	= SHDMA_SLAVE_SCIF8_RX,
#endif
};

static struct platform_device scif8_device = {
	.name		= "sh-sci",
	.id		= 8,
	.dev		= {
		.platform_data	= &scif8_platform_data,
		.dma_mask	= NULL,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

static struct sh_timer_config cmt10_platform_data = {
	.name = "CMT10",
	.channel_offset = 0x10,
	.clk = "r_clk",
	.timer_bit = 0,
	.clockevent_rating = 125,
	.clocksource_rating = 125,
};

static struct resource cmt10_resources[] = {
	[0] = {
		.name	= "CMT10",
		.start	= 0xe6138010,
		.end	= 0xe613801b,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_spi(65),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device cmt10_device = {
	.name		= "sh_cmt",
	.id		= 10,
	.dev = {
		.platform_data	= &cmt10_platform_data,
	},
	.resource	= cmt10_resources,
	.num_resources	= ARRAY_SIZE(cmt10_resources),
};

/* Transmit sizes and respective CHCR register values */
enum {
	XMIT_SZ_8BIT		= 0,
	XMIT_SZ_16BIT		= 1,
	XMIT_SZ_32BIT		= 2,
	XMIT_SZ_64BIT		= 7,
	XMIT_SZ_128BIT		= 3,
	XMIT_SZ_256BIT		= 4,
	XMIT_SZ_512BIT		= 5,
};

/* log2(size / 8) - used to calculate number of transfers */
#define TS_SHIFT {			\
	[XMIT_SZ_8BIT]		= 0,	\
	[XMIT_SZ_16BIT]		= 1,	\
	[XMIT_SZ_32BIT]		= 2,	\
	[XMIT_SZ_64BIT]		= 3,	\
	[XMIT_SZ_128BIT]	= 4,	\
	[XMIT_SZ_256BIT]	= 5,	\
	[XMIT_SZ_512BIT]	= 6,	\
}

#define TS_INDEX2VAL(i)		((((i) & 3) << 3) | (((i) & 0xc) << (20 - 2)))

static const struct sh_dmae_slave_config sh73a0_dmae_slaves[] = {
	{
		.slave_id	= SHDMA_SLAVE_SCIF0_TX,
		.addr		= 0xe6c40020,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x21,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF0_RX,
		.addr		= 0xe6c40024,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x22,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF1_TX,
		.addr		= 0xe6c50020,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x25,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF1_RX,
		.addr		= 0xe6c50024,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x26,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF2_TX,
		.addr		= 0xe6c60020,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x29,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF2_RX,
		.addr		= 0xe6c60024,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x2a,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF3_TX,
		.addr		= 0xe6c70020,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x2d,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF3_RX,
		.addr		= 0xe6c70024,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x2e,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF4_TX,
		.addr		= 0xe6c80020,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x39,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF4_RX,
		.addr		= 0xe6c80024,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x3a,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF5_TX,
		.addr		= 0xe6cb0020,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x35,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF5_RX,
		.addr		= 0xe6cb0024,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x36,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF6_TX,
		.addr		= 0xe6cc0020,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x1d,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF6_RX,
		.addr		= 0xe6cc0024,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x1e,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF7_TX,
		.addr		= 0xe6cd0020,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x19,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF7_RX,
		.addr		= 0xe6cd0024,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x1a,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF8_TX,
		.addr		= 0xe6c30040,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x3d,
	}, {
		.slave_id	= SHDMA_SLAVE_SCIF8_RX,
		.addr		= 0xe6c30060,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_8BIT),
		.mid_rid	= 0x3e,
	}, {
		.slave_id	= SHDMA_SLAVE_SDHI0_TX,
		.addr		= 0xee100030,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_16BIT),
		.mid_rid	= 0xc1,
	}, {
		.slave_id	= SHDMA_SLAVE_SDHI0_RX,
		.addr		= 0xee100030,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_16BIT),
		.mid_rid	= 0xc2,
	}, {
		.slave_id	= SHDMA_SLAVE_SDHI1_TX,
		.addr		= 0xee120030,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_16BIT),
		.mid_rid	= 0xc9,
	}, {
		.slave_id	= SHDMA_SLAVE_SDHI1_RX,
		.addr		= 0xee120030,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_16BIT),
		.mid_rid	= 0xca,
	}, {
		.slave_id	= SHDMA_SLAVE_SDHI2_TX,
		.addr		= 0xee140030,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_16BIT),
		.mid_rid	= 0xcd,
	}, {
		.slave_id	= SHDMA_SLAVE_SDHI2_RX,
		.addr		= 0xee140030,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_16BIT),
		.mid_rid	= 0xce,
	}, {
		.slave_id	= SHDMA_SLAVE_MMCIF_TX,
		.addr		= 0xe6bd0034,
		.chcr		= DM_FIX | SM_INC | 0x800 | TS_INDEX2VAL(XMIT_SZ_32BIT),
		.mid_rid	= 0xd1,
	}, {
		.slave_id	= SHDMA_SLAVE_MMCIF_RX,
		.addr		= 0xe6bd0034,
		.chcr		= DM_INC | SM_FIX | 0x800 | TS_INDEX2VAL(XMIT_SZ_32BIT),
		.mid_rid	= 0xd2,
	},
};

#define DMAE_CHANNEL(_offset, _dmars, _dmars_bit)	\
	{						\
		.offset		= _offset,		\
		.dmars		= _dmars,		\
		.dmars_bit	= _dmars_bit,		\
	}

static const struct sh_dmae_channel sh73a0_dmae_channels[] = {
	DMAE_CHANNEL(0, 0x40, 0),
	DMAE_CHANNEL(0x80, 0x40, 0),
	DMAE_CHANNEL(0x100, 0x40, 0),
	DMAE_CHANNEL(0x180, 0x40, 0),
	DMAE_CHANNEL(0x200, 0x40, 0),
	DMAE_CHANNEL(0x280, 0x40, 0),
	DMAE_CHANNEL(0x300, 0x40, 0),
	DMAE_CHANNEL(0x380, 0x40, 0),
	DMAE_CHANNEL(0x400, 0x40, 0),
	DMAE_CHANNEL(0x480, 0x40, 0),
	DMAE_CHANNEL(0x500, 0x40, 0),
	DMAE_CHANNEL(0x580, 0x40, 0),
	DMAE_CHANNEL(0x600, 0x40, 0),
	DMAE_CHANNEL(0x680, 0x40, 0),
	DMAE_CHANNEL(0x700, 0x40, 0),
	DMAE_CHANNEL(0x780, 0x40, 0),
	DMAE_CHANNEL(0x800, 0x40, 0),
#ifndef CONFIG_MTD_NAND_SH_FLCTL
	DMAE_CHANNEL(0x880, 0x40, 0),
#endif
	DMAE_CHANNEL(0x900, 0x40, 0),
	DMAE_CHANNEL(0x980, 0x40, 0),
};

static const unsigned int ts_shift[] = TS_SHIFT;

static struct sh_dmae_pdata dma_platform_data = {
	.slave		= sh73a0_dmae_slaves,
	.slave_num	= ARRAY_SIZE(sh73a0_dmae_slaves),
	.channel	= sh73a0_dmae_channels,
	.channel_num	= ARRAY_SIZE(sh73a0_dmae_channels),
	.ts_low_shift	= 3,
	.ts_low_mask	= 0x18,
	.ts_high_shift	= (20 - 2),     /* 2 bits for shifted low TS */
	.ts_high_mask	= 0x00300000,
	.ts_shift	= ts_shift,
	.ts_shift_num	= ARRAY_SIZE(ts_shift),
	.dmaor_init	= DMAOR_DME,
};

/* Resource order important! */
static struct resource sh73a0_dmae_resources[] = {
	/*
	 * TODO: DMA extended resource selector (DMARS) is used to be
	 * part of auxiliary functions, and placed a little distance away.
	 * In AG5 system, however, DMA registers are re-arranged and DMARS
	 * is now merged into per-channel registers.  Platform resources
	 * need to be adapted accordingly.
	 */
	{
		/* Channel registers including DMARSx */
		.start	= 0xfe008000,
		.end	= 0xfe008a00 - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		/* Operational registers including DMAOR */
		.start	= 0xfe000000,
		.end	= 0xfe000080 - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		/* DMA error IRQ */
		.start	= gic_spi(129),
		.end	= gic_spi(129),
		.flags	= IORESOURCE_IRQ,
	},
	{
		/* IRQ for channels 0-19 */
		.start	= gic_spi(109),
		.end	= gic_spi(128),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dma_device = {
	.name		= "sh-dma-engine",
	.id		= 0,
	.resource	= sh73a0_dmae_resources,
	.num_resources	= ARRAY_SIZE(sh73a0_dmae_resources),
	.dev		= {
		.platform_data	= &dma_platform_data,
	},
};

static struct sh_i2c_plat_data i2c0_platform_data = {
	.clkrate	= 400000,
};

static struct resource i2c0_resources[] = {
	[0] = {
		.name	= "IIC0",
		.start	= 0xe6820000,
		.end	= 0xe6820425 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_spi(167),
		.end	= gic_spi(170),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct sh_i2c_plat_data i2c1_platform_data = {
	.clkrate	= 400000,
};

static struct resource i2c1_resources[] = {
	[0] = {
		.name	= "IIC1",
		.start	= 0xe6822000,
		.end	= 0xe6822425 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_spi(51),
		.end	= gic_spi(54),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct sh_i2c_plat_data i2c2_platform_data = {
	.clkrate	= 400000,
};

static struct resource i2c2_resources[] = {
	[0] = {
		.name	= "IIC2",
		.start	= 0xe6824000,
		.end	= 0xe6824425 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_spi(171),
		.end	= gic_spi(174),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct sh_i2c_plat_data i2c3_platform_data = {
	.clkrate	= 400000,
};

static struct resource i2c3_resources[] = {
	[0] = {
		.name	= "IIC3",
		.start	= 0xe6826000,
		.end	= 0xe6826425 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= gic_spi(183),
		.end	= gic_spi(186),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device i2c0_device = {
	.name		= "i2c-sh_mobile",
	.id		= 0,
	.resource	= i2c0_resources,
	.num_resources	= ARRAY_SIZE(i2c0_resources),
	.dev		= {
		.platform_data	= &i2c0_platform_data,
	},
};

static struct platform_device i2c1_device = {
	.name		= "i2c-sh_mobile",
	.id		= 1,
	.resource	= i2c1_resources,
	.num_resources	= ARRAY_SIZE(i2c1_resources),
	.dev		= {
		.platform_data	= &i2c1_platform_data,
	},
};

static struct platform_device i2c2_device = {
	.name		= "i2c-sh_mobile",
	.id		= 2,
	.resource	= i2c2_resources,
	.num_resources	= ARRAY_SIZE(i2c2_resources),
	.dev		= {
		.platform_data	= &i2c2_platform_data,
	},
};

static struct platform_device i2c3_device = {
	.name		= "i2c-sh_mobile",
	.id		= 3,
	.resource	= i2c3_resources,
	.num_resources	= ARRAY_SIZE(i2c3_resources),
	.dev		= {
		.platform_data	= &i2c3_platform_data,
	},
};

static struct platform_device *sh73a0_early_devices[] __initdata = {
	&scif0_device,
	&scif1_device,
	&scif2_device,
	&scif3_device,
	&scif4_device,
	&scif5_device,
	&scif6_device,
	&scif7_device,
	&scif8_device,
	&cmt10_device,
	&dma_device,
};

static struct platform_device *sh73a0_late_devices[] __initdata = {
	&i2c0_device,
	&i2c1_device,
	&i2c2_device,
	&i2c3_device,
};

void __init sh73a0_add_standard_devices(void)
{
	platform_add_devices(sh73a0_early_devices,
			    ARRAY_SIZE(sh73a0_early_devices));
	platform_add_devices(sh73a0_late_devices,
			    ARRAY_SIZE(sh73a0_late_devices));
}

void __init sh73a0_add_early_devices(void)
{
	early_platform_add_devices(sh73a0_early_devices,
				   ARRAY_SIZE(sh73a0_early_devices));
}
