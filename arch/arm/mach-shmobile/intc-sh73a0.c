/*
 * sh73a0 processor support - INTC hardware block
 *
 * Copyright (C) 2010  Renesas Electronics Corporation
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
#include <linux/io.h>
#include <linux/sh_intc.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

enum {
	UNUSED_INTCS = 0,

	INTCS,

	/* interrupt sources INTCS */

	/* IRQ0S - IRQ31S */
	/* PINTCS */
	RTDMAC_0_DEI0, RTDMAC_0_DEI1, RTDMAC_0_DEI2, RTDMAC_0_DEI3,
	CEU,
	MFI,
	/* BBIF2 */
	VPU,
	TSIF1,
	/* 3DG_SGX543, */
	_2DDMAC,
	RTDMAC_1_DEI4, RTDMAC_1_DEI5, RTDMAC_1_DADERR,
	/* KEYSC */
	VINT,
	MSIOF,
	TMU0_TUNI0, TMU0_TUNI1, TMU0_TUNI2,
	CMT0,
	TSIF0,
	/* CMT2 */
	LMB,
	/* MSU */
	CTI,
	/* RWDT0 */
	ICB,
	PEP,
	ASA,
	JPU_JPEG,
	LCDC,
	LCRC,
	RTDMAC_2_DEI6, RTDMAC_2_DEI7, RTDMAC_2_DEI8, RTDMAC_2_DEI9,
	RTDMAC_3_DEI10, RTDMAC_3_DEI11,
	FRC,
	GCU,
	LCDC1,
	CSIRX,
	DSITX0_DSITX00, DSITX0_DSITX01,
	/* SPU2 */
	/* FSI */
	TMU1_TUNI0, TMU1_TUNI1, TMU1_TUNI2,
	TSIF2,
	CMT4,
	/* MFIS2 */
	CPORTS2R,
	TSG,
	DMASCH1,
	SCUW,
	VIO60,
	VIO61,
	CEU21,
	CSI21,
	DSITX1_DSITX10, DSITX1_DSITX11,
	DISP,
	DSRV,
	EMUX2_EMUX20I, EMUX2_EMUX21I,
	MSTIF0_MST00I, MSTIF0_MST01I,
	MSTIF1_MST10I, MSTIF1_MST11I,
	/* SPUV, */
	/* INTCA0 - INTCA1 */

	/* interrupt groups INTCS */
	RTDMAC_0, RTDMAC_1, RTDMAC_2, RTDMAC_3, DSITX0, DSITX1,
	TMU1, EMUX2, MSTIF0, MSTIF1,
};

static struct intc_vect intcs_vectors[] = {
	/* IRQ0S - IRQ31S */
	/* PINTCS */
	INTCS_VECT(RTDMAC_0_DEI0, 0x800), INTCS_VECT(RTDMAC_0_DEI1, 0x820),
	INTCS_VECT(RTDMAC_0_DEI2, 0x840), INTCS_VECT(RTDMAC_0_DEI3, 0x860),
	INTCS_VECT(CEU, 0x880), INTCS_VECT(MFI, 0x900),
	/* BBIF2 */
	INTCS_VECT(VPU, 0x980),
	INTCS_VECT(TSIF1, 0x9a0),
	/* 3DG_SGX543, */
	INTCS_VECT(_2DDMAC, 0xa00),
	INTCS_VECT(RTDMAC_1_DEI4, 0xb80), INTCS_VECT(RTDMAC_1_DEI5, 0xba0),
	INTCS_VECT(RTDMAC_1_DADERR, 0xbc0),
	/* KEYSC */
	INTCS_VECT(VINT, 0xc80),
	INTCS_VECT(MSIOF, 0xd20),
	INTCS_VECT(TMU0_TUNI0, 0xe80), INTCS_VECT(TMU0_TUNI1, 0xea0),
	INTCS_VECT(TMU0_TUNI2, 0xec0),
	INTCS_VECT(CMT0, 0xf00),
	INTCS_VECT(TSIF0, 0xf20),
	/* CMT2 */
	INTCS_VECT(LMB, 0xf60),
	/* MSU */
	INTCS_VECT(CTI, 0x400),
	/* RWDT0 */
	INTCS_VECT(ICB, 0x480),
	INTCS_VECT(PEP, 0x4a0),
	INTCS_VECT(ASA, 0x4c0),
	INTCS_VECT(JPU_JPEG, 0x560),
	INTCS_VECT(LCDC, 0x580),
	INTCS_VECT(LCRC, 0x5a0),
	INTCS_VECT(RTDMAC_2_DEI6, 0x1300), INTCS_VECT(RTDMAC_2_DEI7, 0x1320),
	INTCS_VECT(RTDMAC_2_DEI8, 0x1340), INTCS_VECT(RTDMAC_2_DEI9, 0x1360),
	INTCS_VECT(RTDMAC_3_DEI10, 0x1380), INTCS_VECT(RTDMAC_3_DEI11, 0x13a0),
	INTCS_VECT(FRC, 0x1700),
	INTCS_VECT(GCU, 0x1760),
	INTCS_VECT(LCDC1, 0x1780),
	INTCS_VECT(CSIRX, 0x17a0),
	INTCS_VECT(DSITX0_DSITX00, 0x17c0), INTCS_VECT(DSITX0_DSITX01, 0x17e0),
	/* SPU2 */
	/* FSI */
	INTCS_VECT(TMU1_TUNI0, 0x1900), INTCS_VECT(TMU1_TUNI1, 0x1920),
	INTCS_VECT(TMU1_TUNI2, 0x1940),
	INTCS_VECT(TSIF2, 0x1960),
	INTCS_VECT(CMT4, 0x1980),
	/* MFIS2 */
	INTCS_VECT(CPORTS2R, 0x1a20),
	INTCS_VECT(TSG, 0x1ae0),
	INTCS_VECT(DMASCH1, 0x1b00),
	INTCS_VECT(SCUW, 0x1b40),
	INTCS_VECT(VIO60, 0x1b60),
	INTCS_VECT(VIO61, 0x1b80),
	INTCS_VECT(CEU21, 0x1ba0),
	INTCS_VECT(CSI21, 0x1be0),
	INTCS_VECT(DSITX1_DSITX10, 0x1c00), INTCS_VECT(DSITX1_DSITX11, 0x1c20),
	INTCS_VECT(DISP, 0x1c40),
	INTCS_VECT(DSRV, 0x1c40),
	INTCS_VECT(EMUX2_EMUX20I, 0x1c80), INTCS_VECT(EMUX2_EMUX21I, 0x1ca0),
	INTCS_VECT(MSTIF0_MST00I, 0x1cc0), INTCS_VECT(MSTIF0_MST01I, 0x1ce0),
	INTCS_VECT(MSTIF1_MST10I, 0x1d00), INTCS_VECT(MSTIF1_MST11I, 0x1d20),
	/* SPUV, */
	/* INTCA0 - INTCA1 */
};

static struct intc_group intcs_groups[] __initdata = {
	INTC_GROUP(RTDMAC_0, RTDMAC_0_DEI0, RTDMAC_0_DEI1,
		   RTDMAC_0_DEI2, RTDMAC_0_DEI3),
	INTC_GROUP(RTDMAC_1, RTDMAC_1_DEI4,
		   RTDMAC_1_DEI5, RTDMAC_1_DADERR),
	INTC_GROUP(RTDMAC_2, RTDMAC_2_DEI6, RTDMAC_2_DEI7,
		   RTDMAC_2_DEI8, RTDMAC_2_DEI9),
	INTC_GROUP(RTDMAC_3, RTDMAC_3_DEI10, RTDMAC_3_DEI11),
	INTC_GROUP(DSITX0, DSITX0_DSITX00, DSITX0_DSITX01),
	INTC_GROUP(TMU1, TMU1_TUNI2, TMU1_TUNI1, TMU1_TUNI0),
	INTC_GROUP(DSITX1, DSITX1_DSITX10, DSITX1_DSITX11),
	INTC_GROUP(EMUX2, EMUX2_EMUX20I, EMUX2_EMUX21I),
	INTC_GROUP(MSTIF0, MSTIF0_MST00I, MSTIF0_MST01I),
	INTC_GROUP(MSTIF1, MSTIF1_MST10I, MSTIF1_MST11I),
};

static struct intc_mask_reg intcs_mask_registers[] = {
	{ 0xffd20184, 0xffd201c4, 8, /* IMR1SA / IMCR1SA */
	  { 0, 0, 0, CEU,
	    0, 0, 0, 0 } },
	{ 0xffd20188, 0xffd201c8, 8, /* IMR2SA / IMCR2SA */
	  { 0, 0, 0, VPU,
	    0, 0, 0, MFI } },
	{ 0xffd2018c, 0xffd201cc, 8, /* IMR3SA / IMCR3SA */
	  { 0, 0, 0, _2DDMAC,
	    0, ASA, PEP, ICB } },
	{ 0xffd20190, 0xffd201d0, 8, /* IMR4SA / IMCR4SA */
	  { 0, 0, 0, CTI,
	    JPU_JPEG, 0, LCRC, LCDC } },
	{ 0xffd20194, 0xffd201d4, 8, /* IMR5SA / IMCR5SA */
	  { 0, RTDMAC_1_DADERR, RTDMAC_1_DEI5, RTDMAC_1_DEI4,
	    RTDMAC_0_DEI3, RTDMAC_0_DEI2, RTDMAC_0_DEI1, RTDMAC_0_DEI0 } },
	{ 0xffd20198, 0xffd201d8, 8, /* IMR6SA / IMCR6SA */
	  { 0, 0, MSIOF, 0,
	    0, 0, VINT, 0 } },
	{ 0xffd2019c, 0xffd201dc, 8, /* IMR7SA / IMCR7SA */
	  { 0, TMU0_TUNI2, TMU0_TUNI1, TMU0_TUNI0,
	    0, 0, 0, 0 } },
	{ 0xffd201a4, 0xffd201e4, 8, /* IMR9SA / IMCR9SA */
	  { 0, 0, 0, CMT0,
	    0, 0, 0, 0 } },
	{ 0xffd201ac, 0xffd201ec, 8, /* IMR11SA / IMCR11SA */
	  { 0, 0, 0, 0,
	    0, TSIF1, LMB, TSIF0 } },
	{ 0xffd50180, 0xffd501c0, 8, /* IMR0SA3 / IMCR0SA3 */
	  { RTDMAC_2_DEI6, RTDMAC_2_DEI7, RTDMAC_2_DEI8, RTDMAC_2_DEI9,
	    RTDMAC_3_DEI10, RTDMAC_3_DEI11, 0, 0 } },
	{ 0xffd50190, 0xffd501d0, 8, /* IMR4SA3 / IMCR4SA3 */
	  { FRC, 0, 0, GCU,
	    LCDC1, CSIRX, DSITX0_DSITX00, DSITX0_DSITX01 } },
	{ 0xffd50198, 0xffd501d8, 8, /* IMR6SA3 / IMCR6SA3 */
	  { TMU1_TUNI0, TMU1_TUNI1, TMU1_TUNI2, TSIF2,
	    CMT4, 0, 0, 0 } },
	{ 0xffd5019c, 0xffd501dc, 8, /* IMR7SA3 / IMCR7SA3 */
	  { 0, CPORTS2R, 0, 0,
	    0, 0, 0, TSG } },
	{ 0xffd501a0, 0xffd501e0, 8, /* IMR8SA3 / IMCR8SA3 */
	  { DMASCH1, 0, SCUW, VIO60,
	    VIO61, CEU21, 0, CSI21 } },
	{ 0xffd501a4, 0xffd501e4, 8, /* IMR9SA3 / IMCR9SA3 */
	  { DSITX1_DSITX10, DSITX1_DSITX11, DISP, DSRV,
	    EMUX2_EMUX20I, EMUX2_EMUX21I, MSTIF0_MST00I, MSTIF0_MST01I } },
	{ 0xffd501a8, 0xffd501e8, 8, /* IMR10SA3 / IMCR10SA3 */
	  { MSTIF1_MST10I, MSTIF1_MST11I, 0, 0,
	    0, 0, 0, 0 } },
};

/* Priority is needed for INTCA to receive the INTCS interrupt */
static struct intc_prio_reg intcs_prio_registers[] = {
	{ 0xffd20000, 0, 16, 4, /* IPRAS */ { CTI, 0, _2DDMAC, ICB } },
	{ 0xffd20004, 0, 16, 4, /* IPRBS */ { JPU_JPEG, LCDC, 0, LCRC } },
	{ 0xffd20010, 0, 16, 4, /* IPRES */ { RTDMAC_0, CEU, MFI, VPU } },
	{ 0xffd20014, 0, 16, 4, /* IPRFS */ { 0, RTDMAC_1, 0, CMT0 } },
	{ 0xffd20018, 0, 16, 4, /* IPRGS */ { TMU0_TUNI0, TMU0_TUNI1,
					      TMU0_TUNI2, TSIF1 } },
	{ 0xffd2001c, 0, 16, 4, /* IPRHS */ { VINT, 0, 0, 0 } },
	{ 0xffd20020, 0, 16, 4, /* IPRIS */ { 0, MSIOF, TSIF0, 0 } },
	{ 0xffd20028, 0, 16, 4, /* IPRKS */ { 0, ASA, LMB, PEP } },
	{ 0xffd50000, 0, 16, 4, /* IPRAS3 */ { RTDMAC_2, 0, 0, 0 } },
	{ 0xffd50004, 0, 16, 4, /* IPRBS3 */ { RTDMAC_3, 0, 0, 0 } },
	{ 0xffd50020, 0, 16, 4, /* IPRIS3 */ { FRC, 0, 0, GCU } },
	{ 0xffd50024, 0, 16, 4, /* IPRJS3 */ { LCDC1, CSIRX, DSITX0, 0 } },
	{ 0xffd50030, 0, 16, 4, /* IPRMS3 */ { TMU1, 0, 0, TSIF2 } },
	{ 0xffd50034, 0, 16, 4, /* IPRNS3 */ { CMT4, 0, 0, 0 } },
	{ 0xffd50038, 0, 16, 4, /* IPROS3 */ { 0, CPORTS2R, 0, 0 } },
	{ 0xffd5003c, 0, 16, 4, /* IPRPS3 */ { 0, 0, 0, TSG } },
	{ 0xffd50040, 0, 16, 4, /* IPRQS3 */ { DMASCH1, 0, SCUW, VIO60 } },
	{ 0xffd50044, 0, 16, 4, /* IPRRS3 */ { VIO61, CEU21, 0, CSI21 } },
	{ 0xffd50048, 0, 16, 4, /* IPRSS3 */ { DSITX1_DSITX10, DSITX1_DSITX11,
					       DISP, DSRV } },
	{ 0xffd5004c, 0, 16, 4, /* IPRTS3 */ { EMUX2_EMUX20I, EMUX2_EMUX21I,
					       MSTIF0_MST00I, MSTIF0_MST01I } },
	{ 0xffd50050, 0, 16, 4, /* IPRUS3 */ { MSTIF1_MST10I, MSTIF1_MST11I,
					       0, 0 } },
};

static struct resource intcs_resources[] __initdata = {
	[0] = {
		.start	= 0xffd20000,
		.end	= 0xffd201ff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= 0xffd50000,
		.end	= 0xffd501ff,
		.flags	= IORESOURCE_MEM,
	}
};

static struct intc_desc intcs_desc __initdata = {
	.name = "sh73a0-intcs",
	.resource = intcs_resources,
	.num_resources = ARRAY_SIZE(intcs_resources),
	.hw = INTC_HW_DESC(intcs_vectors, intcs_groups, intcs_mask_registers,
			   intcs_prio_registers, NULL, NULL),
};

static void intcs_demux(unsigned int irq, struct irq_desc *desc)
{
	struct irq_chip *chip = get_irq_chip(irq);
	void __iomem *reg = (void *)get_irq_data(irq);
	unsigned int evtcodeas = ioread32(reg);

	/* primary controller ack'ing */
	chip->ack(irq);

	generic_handle_irq(intcs_evt2irq(evtcodeas));

	/* primary controller unmasking */
	chip->unmask(irq);
}

void __init sh73a0_init_irq(void)
{
	void __iomem *intevtsa = ioremap_nocache(0xffd20100, PAGE_SIZE);

	register_intc_controller(&intcs_desc);

	/* demux using INTEVTSA */
	set_irq_data(gic_spi(50), (void *)intevtsa);
	set_irq_chained_handler(gic_spi(50), intcs_demux);
}
