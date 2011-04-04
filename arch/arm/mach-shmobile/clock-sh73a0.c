/*
 * sh73a0 clock framework support
 *
 * Copyright (C) 2010 Magnus Damm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/sh_clk.h>
#include <mach/common.h>
#include <asm/clkdev.h>

/* SH73A0 registers */
#define FRQCRA		0xe6150000
#define FRQCRB		0xe6150004
#define FRQCRD		0xe61500e4
#define VCLKCR1		0xe6150008
#define VCLKCR2		0xe615000c
#define VCLKCR3		0xe615001c

#define ZBCKCR		0xe6150010
#define FLCKCR		0xe6150014

#define SD0CKCR		0xe6150074
#define SD1CKCR		0xe6150078
#define SD2CKCR		0xe615007c

#define FSIACKCR	0xe6150018
#define FSIBCKCR	0xe6150090

#define SUBCKCR		0xe6150080
#define SPUACKCR	0xe6150084
#define SPUVCKCR	0xe6150094

#define MSUCKCR		0xe6150088
#define HSICKCR		0xe615008c
#define MFCK1CR		0xe6150098
#define MFCK2CR		0xe615009c

#define DSITCKCR	0xe6150060
#define DSI0PCKCR	0xe6150064
#define DSI1PCKCR	0xe6150068

#define PLLECR		0xe61500d0
#define PLL0CR		0xe61500d8
#define PLL1CR		0xe6150028
#define PLL2CR		0xe615002c
#define PLL3CR		0xe61500dc

#define SMSTPCR0	0xe6150130
#define SMSTPCR1	0xe6150134
#define SMSTPCR2	0xe6150138
#define SMSTPCR3	0xe615013c
#define SMSTPCR4	0xe6150140
#define SMSTPCR5	0xe6150144

/* Fixed 32 KHz root clock from EXTALR pin */
static struct clk r_clk = {
	.rate           = 32768,
};

/*
 * 26MHz default rate for the EXTAL1 root input clock.
 * If needed, reset this with clk_set_rate() from the platform code.
 */
static struct clk extal1_clk = {
	.rate		= 26000000,
};

/*
 * 48MHz default rate for the EXTAL2 root input clock.
 * If needed, reset this with clk_set_rate() from the platform code.
 */
static struct clk extal2_clk = {
	.rate		= 48000000,
};

static unsigned long div_recalc(struct clk *clk)
{
	return clk->parent->rate / (int)(clk->priv);
}

static struct clk_ops div_clk_ops = {
	.recalc	= div_recalc,
};

static struct clk extal1_div2_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)2,
	.parent	= &extal1_clk,
};

static struct clk extal2_div2_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)2,
	.parent	= &extal2_clk,
};

static struct clk extal2_div4_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)4,
	.parent	= &extal2_clk,
};

static struct clk_ops followparent_clk_ops = {
	.recalc	= followparent_recalc,
};

static struct clk main_clk = {
	.ops	= &followparent_clk_ops,
	.parent	= &extal1_clk,
};

static struct clk main_div2_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)2,
	.parent	= &main_clk,
};

/*
 * PLL{0,1,2,3}CR recalc.
 * Using .enalbe_reg for PLL{0,1,2,3}CR but PLLECR.
 *
 * PLL0CR.THRUCKSEL should be handled by switching the parent.
 * There is no .CFG on PLL{0,3}CR.24 but is 0 according to the HW manual.
 * This code utilizes it.
 */
static unsigned long pllc_recalc(struct clk *clk)
{
	unsigned long mult = 1;
	unsigned int reg = __raw_readl(clk->enable_reg);

	if (__raw_readl(PLLECR) & (1 << clk->enable_bit))
		mult = (((reg >> 24) & 0x3f) + 1) * (((reg >> 20) & 1) + 1);

	return clk->parent->rate * mult;
}

static struct clk_ops pllc_clk_ops = {
	.recalc		= pllc_recalc,
};

static struct clk pllc0_clk = {
	.ops		= &pllc_clk_ops,
	.flags		= CLK_ENABLE_ON_INIT,
	.parent		= &main_clk,
	.enable_reg	= (void __iomem *)PLL0CR,
	.enable_bit	= 0,
};

static struct clk pllc1_clk = {
	.ops		= &pllc_clk_ops,
	.flags		= CLK_ENABLE_ON_INIT,
	.parent		= &main_clk,
	.enable_reg	= (void __iomem *)PLL1CR,
	.enable_bit	= 1,
};

static struct clk pllc1_div2_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)2,
	.parent	= &pllc1_clk,
};

static struct clk pllc1_div7_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)7,
	.parent	= &pllc1_clk,
};

static struct clk pllc1_div13_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)13,
	.parent	= &pllc1_clk,
};

static struct clk pllc2_clk = {
	.ops		= &pllc_clk_ops,
	.flags		= CLK_ENABLE_ON_INIT,
	.parent		= &main_clk,
	.enable_reg	= (void __iomem *)PLL2CR,
	.enable_bit	= 2,
};

static struct clk pllc3_clk = {
	.ops		= &pllc_clk_ops,
	.flags		= CLK_ENABLE_ON_INIT,
	.parent		= &main_clk,
	.enable_reg	= (void __iomem *)PLL3CR,
	.enable_bit	= 3,
};

struct clk *main_clks[] = {
	&r_clk,
	&extal1_clk,
	&extal2_clk,
	&extal1_div2_clk,
	&extal2_div2_clk,
	&extal2_div4_clk,
	&main_clk,
	&main_div2_clk,
	&pllc0_clk,
	&pllc1_clk,
	&pllc1_div2_clk,
	&pllc1_div7_clk,
	&pllc1_div13_clk,
	&pllc2_clk,
	&pllc3_clk,
};

static void div4_kick(struct clk *clk)
{
	unsigned long value;

	/* set KICK bit in FRQCRB to update hardware setting */
	value = __raw_readl(FRQCRB);
	value |= (1 << 31);
	__raw_writel(value, FRQCRB);
}

static int divisors[] = { 2, 3, 4, 6, 8, 12, 16, 18,
			  24, 32, 36, 48, 7, 72, 96, 0 };

static struct clk_div_mult_table div4_div_mult_table = {
	.divisors = divisors,
	.nr_divisors = ARRAY_SIZE(divisors),
};

static struct clk_div4_table div4_table = {
	.div_mult_table = &div4_div_mult_table,
	.kick = div4_kick,
};

enum { DIV4_I, DIV4_ZG, DIV4_M3, DIV4_B, DIV4_M1, DIV4_M2,
       DIV4_Z, DIV4_ZTR, DIV4_ZT, DIV4_ZX, DIV4_ZS, DIV4_HP,
       DIV4_ZB31, DIV4_ZB30,
       DIV4_NR };

struct clk div4_clks[DIV4_NR] = {
	[DIV4_I] = SH_CLK_DIV4(&pllc1_clk, FRQCRA, 20, 0x0dff,
		CLK_ENABLE_ON_INIT),
	[DIV4_ZG] = SH_CLK_DIV4(&pllc0_clk, FRQCRA, 16, 0x097f, 0),
	[DIV4_M3] = SH_CLK_DIV4(&pllc1_clk, FRQCRA, 12, 0x1dff, 0),
	[DIV4_B] = SH_CLK_DIV4(&pllc1_clk, FRQCRA, 8, 0x0dff,
		CLK_ENABLE_ON_INIT),
	[DIV4_M1] = SH_CLK_DIV4(&pllc1_clk, FRQCRA, 4, 0x1dff,
		CLK_ENABLE_ON_INIT),
	[DIV4_M2] = SH_CLK_DIV4(&pllc1_clk, FRQCRA, 0, 0x1dff, 0),
	[DIV4_Z] = SH_CLK_DIV4(&pllc0_clk, FRQCRB, 24, 0x097f, 0),
	[DIV4_ZTR] = SH_CLK_DIV4(&pllc1_clk, FRQCRB, 20, 0x0dff, 0),
	[DIV4_ZT] = SH_CLK_DIV4(&pllc1_clk, FRQCRB, 16, 0x0dff, 0),
	[DIV4_ZX] = SH_CLK_DIV4(&pllc1_clk, FRQCRB, 12, 0x0dff, 0),
	[DIV4_ZS] = SH_CLK_DIV4(&pllc1_clk, FRQCRB, 8, 0x0dff, 0),
	[DIV4_HP] = SH_CLK_DIV4(&pllc1_clk, FRQCRB, 4, 0x0dff, 0),
	[DIV4_ZB31] = SH_CLK_DIV4(&pllc3_clk, FRQCRD, 8, 0x097f, 0),
	[DIV4_ZB30] = SH_CLK_DIV4(&pllc3_clk, FRQCRD, 0, 0x097f, 0),
};

enum { DIV6_ZB, DIV6_SD0, DIV6_SD1, DIV6_SD2, DIV6_FL,
       DIV6_VCK1, DIV6_VCK2, DIV6_VCK3, DIV6_FSIA, DIV6_FSIB,
       DIV6_SUB, DIV6_SPUA, DIV6_SPUV, DIV6_MSU, DIV6_HSI,
       DIV6_MF1, DIV6_MF2, DIV6_DSIT, DIV6_DSI0P, DIV6_DSI1P,
       DIV6_NR };

struct clk div6_clks[] = {
	[DIV6_ZB] = SH_CLK_DIV6(&pllc1_div2_clk, ZBCKCR, 0),
	[DIV6_SD0] = SH_CLK_DIV6(&pllc1_div13_clk, SD0CKCR, 0),
	[DIV6_SD1] = SH_CLK_DIV6(&pllc1_div13_clk, SD1CKCR, 0),
	[DIV6_SD2] = SH_CLK_DIV6(&pllc1_div13_clk, SD2CKCR, 0),
	[DIV6_FL] = SH_CLK_DIV6(&pllc1_div2_clk, FLCKCR, 0),
	[DIV6_VCK1] = SH_CLK_DIV6(&pllc1_div2_clk, VCLKCR1, 0),
	[DIV6_VCK2] = SH_CLK_DIV6(&pllc1_div2_clk, VCLKCR2, 0),
	[DIV6_VCK3] = SH_CLK_DIV6(&pllc1_div2_clk, VCLKCR3, 0),
	[DIV6_FSIA] = SH_CLK_DIV6(&pllc1_div2_clk, FSIACKCR, 0),
	[DIV6_FSIB] = SH_CLK_DIV6(&pllc1_div2_clk, FSIBCKCR, 0),
	[DIV6_SUB] = SH_CLK_DIV6(&pllc1_div2_clk, SUBCKCR, 0),
	[DIV6_SPUA] = SH_CLK_DIV6(&pllc1_div2_clk, SPUACKCR, 0),
	[DIV6_SPUV] = SH_CLK_DIV6(&pllc1_div2_clk, SPUVCKCR, 0),
	[DIV6_MSU] = SH_CLK_DIV6(&pllc1_div2_clk, MSUCKCR, 0),
	[DIV6_HSI] = SH_CLK_DIV6(&pllc1_div7_clk, HSICKCR, 0),
	[DIV6_MF1] = SH_CLK_DIV6(&pllc1_div2_clk, MFCK1CR, 0),
	[DIV6_MF2] = SH_CLK_DIV6(&pllc1_div2_clk, MFCK2CR, 0),
	[DIV6_DSIT] = SH_CLK_DIV6(&pllc1_div2_clk, DSITCKCR, 0),
	[DIV6_DSI0P] = SH_CLK_DIV6(&pllc1_div2_clk, DSI0PCKCR, 0),
	[DIV6_DSI1P] = SH_CLK_DIV6(&pllc1_div2_clk, DSI1PCKCR, 0),
};

static struct clk sub_clk = {
	.ops	= &followparent_clk_ops,
	.parent	= &div6_clks[DIV6_SUB],
};

static struct clk spua_clk = {
	.ops	= &followparent_clk_ops,
	.parent	= &div6_clks[DIV6_SPUA],
};

static struct clk spuv_clk = {
	.ops	= &followparent_clk_ops,
	.parent	= &div6_clks[DIV6_SPUV],
};

static struct clk fsia_clk = {
	.ops	= &followparent_clk_ops,
	.parent	= &div6_clks[DIV6_FSIA],
};

static struct clk fsib_clk = {
	.ops	= &followparent_clk_ops,
	.parent	= &div6_clks[DIV6_FSIB],
};

static struct clk cp_clk = {
	.ops	= &followparent_clk_ops,
	.parent	= &main_div2_clk,
};

static struct clk zb1_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)2,
	.parent	= &div6_clks[DIV6_ZB],
};

/*
 * HW manual says DDR div.(for zb3,3s) does always 1/2 div,
 * explicitly when ZB3xSEL=0, implicily by ZB3xFC table otherwise.
 */
static struct clk zb3_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)2,
	.parent	= &pllc3_clk,
	/* or &div4_clks[DIV4_ZB30] according to FRQCRD.ZB30SEL */
};

static struct clk zb3s_clk = {
	.ops	= &div_clk_ops,
	.priv	= (void *)2,
	.parent	= &pllc3_clk,
	/* or &div4_clks[DIV4_ZB31] according to FRQCRD.ZB31SEL */
};

static struct clk z_clk = {
	.ops	= &followparent_clk_ops,
	.parent	= &pllc0_clk,
	/* or &div4_clks[DIV4_Z] according to FRQCRB.ZSEL */
};

struct clk *sub_clks[] = {
	&sub_clk,
	&spua_clk,
	&spuv_clk,
	&fsia_clk,
	&fsib_clk,
	&cp_clk,
	&zb1_clk,
	&zb3_clk,
	&zb3s_clk,
	&z_clk,
};

enum {
	MSTP031,
	MSTP030,
	MSTP029,
	MSTP026,
	MSTP022,
	MSTP021,
	MSTP019,
	MSTP018,
	MSTP017,
	MSTP016,
	MSTP015,
	MSTP007,
	MSTP001,
	MSTP000,

	MSTP131,
	MSTP130,
	MSTP129,
	MSTP128,
	MSTP127,
	MSTP126,
	MSTP125,
	MSTP124,
	/**/
	MSTP122,
	MSTP121,
	MSTP120,
	MSTP119,
	MSTP118,
	MSTP117,
	MSTP116,
	MSTP115,
	MSTP114,
	MSTP113,
	MSTP112,
	MSTP111,
	MSTP110,
	MSTP109,
	MSTP108,
	MSTP107,
	MSTP106,
	MSTP105,
	MSTP104,
	MSTP103,
	MSTP102,
	MSTP101,
	MSTP100,

	/**/
	MSTP230,
	/**/
	MSTP228,
	/**/
	/**/
	MSTP225,
	MSTP224,
	MSTP223,
	MSTP222,
	MSTP221,
	MSTP220,
	MSTP219,
	MSTP218,
	MSTP217,
	MSTP216,
	MSTP215,
	MSTP214,
	MSTP213,
	MSTP212,
	MSTP211,
	MSTP210,
	MSTP209,
	MSTP208,
	MSTP207,
	MSTP206,
	MSTP205,
	MSTP204,
	MSTP203,
	MSTP202,
	MSTP201,
	MSTP200,

	MSTP331,
	MSTP330,
	MSTP329,
	MSTP328,
	/**/
	MSTP326,
	MSTP325,
	MSTP324,
	MSTP323,
	MSTP322,
	MSTP321,
	/**/
	MSTP319,
	MSTP318,
	MSTP317,
	MSTP316,
	MSTP315,
	MSTP314,
	MSTP313,
	MSTP312,
	MSTP311,
	MSTP310,
	MSTP304,
	MSTP303,
	MSTP302,
	MSTP301,
	MSTP300,

	MSTP423,
	MSTP411,
	MSTP410,
	MSTP408,
	MSTP405,
	MSTP404,
	MSTP403,
	MSTP402,
	MSTP400,

	MSTP509,
	MSTP508,
	MSTP507,
	MSTP504,
	MSTP501,
	MSTP500,
	MSTP_NR };

#define MSTP(_parent, _reg, _bit, _flags) \
  SH_CLK_MSTP32(_parent, _reg, _bit, _flags)

static struct clk mstp_clks[MSTP_NR] = {
	[MSTP031] /*RT TLB*/	= MSTP(&div4_clks[DIV4_I], SMSTPCR0, 31, 0),
	[MSTP030] /*RT IC*/	= MSTP(&div4_clks[DIV4_I], SMSTPCR0, 30, 0),
	[MSTP029] /*RT OC*/	= MSTP(&div4_clks[DIV4_I], SMSTPCR0, 29, 0),
	[MSTP026] /*RT X/Y*/	= MSTP(&div4_clks[DIV4_I], SMSTPCR0, 26, 0),
	[MSTP022] /*INTCS*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR0, 22, 0),
	[MSTP021] /*RT-DMAC*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR0, 21, 0),
	[MSTP019] /*H-UDI*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR0, 19, 0),
	[MSTP018] /*RT DBG1*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR0, 18, 0),
	[MSTP017] /*UBC*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR0, 17, 0),
	[MSTP016] /*RT DBG2*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR0, 16, 0),
	[MSTP015] /*ILRAM*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR0, 15, 0),
	[MSTP007] /*ICB*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR0, 7, 0),
	[MSTP001] /*IIC2*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR0, 1, 0),
	[MSTP000] /*MSIOF0*/	= MSTP(&sub_clk, SMSTPCR0, 0, 0),

	[MSTP131] /*VIO61*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 31, 0),
	[MSTP130] /*VIO60*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 30, 0),
	[MSTP129] /*CEU1*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 29, 0),
	[MSTP128] /*CSI2-RX1*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 28, 0),
	[MSTP127] /*CEU0*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 27, 0),
	[MSTP126] /*CSI2-RX0*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 26, 0),
	[MSTP125] /*TMU0*/	= MSTP(&sub_clk, SMSTPCR1, 25, 0),
	[MSTP124] /*CMT0*/	= MSTP(&cp_clk, SMSTPCR1, 24, 0),
	/**/
	[MSTP122] /*TSG*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 22, 0),
	[MSTP121] /*PEPG*/	= MSTP(&div4_clks[DIV4_B], SMSTPCR1, 21, 0),
	[MSTP120] /*PEP*/	= MSTP(&div4_clks[DIV4_B], SMSTPCR1, 20, 0),
	[MSTP119] /*FRC*/	= MSTP(&div4_clks[DIV4_B], SMSTPCR1, 19, 0),
	[MSTP118] /*DSI-TX0*/	= MSTP(&div4_clks[DIV4_B], SMSTPCR1, 18, 0),
	[MSTP117] /*LCDC1*/	= MSTP(&div4_clks[DIV4_B], SMSTPCR1, 17, 0),
	[MSTP116] /*IIC0*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR1, 16, 0),
	[MSTP115] /*2D-dMAC*/	= MSTP(&div4_clks[DIV4_B], SMSTPCR1, 15, 0),
	[MSTP114] /*ASA*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 14, 0),
	[MSTP113] /*MERAM*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 13, 0),
	[MSTP112] /*SGX543*/	= MSTP(&div4_clks[DIV4_ZG], SMSTPCR1, 12, 0),
	[MSTP111] /*TMU1*/	= MSTP(&sub_clk, SMSTPCR1, 11, 0),
	[MSTP110] /*DISP*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR1, 10, 0),
	[MSTP109] /*TSIF1*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR1, 9, 0),
	[MSTP108] /*TSIF0*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR1, 8, 0),
	[MSTP107] /*JPU6E*/	= MSTP(&div4_clks[DIV4_B], SMSTPCR1, 7, 0),
	[MSTP106] /*JPU*/	= MSTP(&div4_clks[DIV4_B], SMSTPCR1, 6, 0),
	[MSTP105] /*TSIF2*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR1, 5, 0),
	[MSTP104] /*MSTIF1*/	= MSTP(&div4_clks[DIV4_M2], SMSTPCR1, 4, 0),
	[MSTP103] /*MSTIF0*/	= MSTP(&div4_clks[DIV4_M2], SMSTPCR1, 3, 0),
	[MSTP102] /*EMUX*/	= MSTP(&div4_clks[DIV4_M2], SMSTPCR1, 2, 0),
	[MSTP101] /*VPU*/	= MSTP(&div4_clks[DIV4_M1], SMSTPCR1, 1, 0),
	[MSTP100] /*LCDC0*/	= MSTP(&div4_clks[DIV4_M3], SMSTPCR1, 0, 0),

	/**/
	[MSTP230] /*L3*/	= MSTP(&div4_clks[DIV4_ZX], SMSTPCR2, 30, 0),
	/**/
	[MSTP228] /*Crypt*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR2, 28, 0),
	/**/
	/**/
	[MSTP225] /*BBIF2*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR2, 25, 0),
	[MSTP224] /*CLKGEN*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR2, 24, 0),
	[MSTP223] /*SPUA*/	= MSTP(&spua_clk, SMSTPCR2, 23, 0),
	[MSTP222] /*Misty*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR2, 22, 0),
	[MSTP221] /*AEXRAM*/	= MSTP(&sub_clk, SMSTPCR2, 21, 0),
	[MSTP220] /*SPUV*/	= MSTP(&spuv_clk, SMSTPCR2, 20, 0),
	[MSTP219] /*SCIFA7*/	= MSTP(&sub_clk, SMSTPCR2, 19, 0),
	[MSTP218] /*SY-DMAC*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR2, 18, 0),
	[MSTP217] /*MP-DMAC*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR2, 17, 0),
	[MSTP216] /*EFRAM*/	= MSTP(&sub_clk, SMSTPCR2, 16, 0),
	[MSTP215] /*MSIOF3*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR2, 15, 0),
	[MSTP214] /*USB-DMAC*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR2, 14, 0),
	[MSTP213] /*MFIS*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR2, 13, 0),
	[MSTP212] /*MFIM*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR2, 12, 0),
	[MSTP211] /*MMFRAM*/	= MSTP(NULL, SMSTPCR2, 11, 0),
	[MSTP210] /*MMFROM*/	= MSTP(NULL, SMSTPCR2, 10, 0),
	[MSTP209] /*SMFRAM*/	= MSTP(NULL, SMSTPCR2, 9, 0),
	[MSTP208] /*MSIOF1*/	= MSTP(&sub_clk, SMSTPCR2, 8, 0),
	[MSTP207] /*SCIFA5*/	= MSTP(&sub_clk, SMSTPCR2, 7, 0),
	[MSTP206] /*SCIFB*/	= MSTP(&sub_clk, SMSTPCR2, 6, 0),
	[MSTP205] /*MSIOF2*/	= MSTP(&sub_clk, SMSTPCR2, 5, 0),
	[MSTP204] /*SCIFA0*/	= MSTP(&sub_clk, SMSTPCR2, 4, 0),
	[MSTP203] /*SCIFA1*/	= MSTP(&sub_clk, SMSTPCR2, 3, 0),
	[MSTP202] /*SCIFA2*/	= MSTP(&sub_clk, SMSTPCR2, 2, 0),
	[MSTP201] /*SCIFA3*/	= MSTP(&sub_clk, SMSTPCR2, 1, 0),
	[MSTP200] /*SCIFA4*/	= MSTP(&sub_clk, SMSTPCR2, 0, 0),

	[MSTP331] /*SCIFA6*/	= MSTP(&sub_clk, SMSTPCR3, 31, 0),
	[MSTP330] /*MSU*/	= MSTP(&div6_clks[DIV6_MSU], SMSTPCR3, 30, 0),
	[MSTP329] /*CMT1*/	= MSTP(&cp_clk, SMSTPCR3, 29, 0),
	[MSTP328] /*FSI*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR3, 28, 0),
	/**/
	[MSTP326] /*SCUW*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR3, 26, 0),
	[MSTP325] /*IrDA*/	= MSTP(&sub_clk, SMSTPCR3, 25, 0),
	[MSTP324] /*IrREM*/	= MSTP(&sub_clk, SMSTPCR3, 24, 0),
	[MSTP323] /*IIC1*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR3, 23, 0),
	[MSTP322] /*USB*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR3, 22, 0),
	[MSTP321] /*SBSC*/	= MSTP(&div4_clks[DIV4_ZB30], SMSTPCR3, 21, 0),
	/**/
	[MSTP319] /*RTDMAC SCH*/= MSTP(NULL, SMSTPCR3, 19, 0),
	[MSTP318] /*SYDMAC SCH*/= MSTP(NULL, SMSTPCR3, 18, 0),
	[MSTP317] /*HSI*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR3, 17, 0),
	[MSTP316] /*SHWYSTAT*/	= MSTP(&div4_clks[DIV4_ZS], SMSTPCR3, 16, 0),
	[MSTP315] /*FLCTl*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR3, 15, 0),
	[MSTP314] /*SDHI0*/	= MSTP(&div6_clks[DIV6_SD0], SMSTPCR3, 14, 0),
	[MSTP313] /*SDHI1*/	= MSTP(&div6_clks[DIV6_SD1], SMSTPCR3, 13, 0),
	[MSTP312] /*MMCIf*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR3, 12, 0),
	[MSTP311] /*SDHI2*/	= MSTP(&div6_clks[DIV6_SD2], SMSTPCR3, 11, 0),
	[MSTP310] /*SIM*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR3, 10, 0),
	[MSTP304] /*TPU0*/	= MSTP(&cp_clk, SMSTPCR3, 4, 0),
	[MSTP303] /*TPU1*/	= MSTP(&cp_clk, SMSTPCR3, 3, 0),
	[MSTP302] /*TPU2*/	= MSTP(&cp_clk, SMSTPCR3, 2, 0),
	[MSTP301] /*TPU3*/	= MSTP(&cp_clk, SMSTPCR3, 1, 0),
	[MSTP300] /*TPU4*/	= MSTP(&cp_clk, SMSTPCR3, 0, 0),

	[MSTP423] /*DSI TX1*/	= MSTP(&sub_clk, SMSTPCR4, 23, 0),
	[MSTP411] /*IIC3*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR4, 11, 0),
	[MSTP410] /*IIC4*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR4, 10, 0),
	[MSTP408] /*MFIM2*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR4, 8, 0),
	[MSTP405] /*CMT4*/	= MSTP(&cp_clk, SMSTPCR4, 5, 0),
	[MSTP404] /*CMT3*/	= MSTP(&cp_clk, SMSTPCR4, 4, 0),
	[MSTP403] /*KEYSC*/	= MSTP(&cp_clk, SMSTPCR4, 3, 0),
	[MSTP402] /*RWDT0*/	= MSTP(&cp_clk, SMSTPCR4, 2, 0),
	[MSTP400] /*CMT2*/	= MSTP(&cp_clk, SMSTPCR4, 0, 0),

	[MSTP509] /*INTCA1*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR5, 9, 0),
	[MSTP508] /*INTCA0*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR5, 8, 0),
	[MSTP507] /*INTCAG*/	= MSTP(&div4_clks[DIV4_HP], SMSTPCR5, 7, 0),
	[MSTP504] /*ACP*/	= MSTP(NULL, SMSTPCR5, 4, 0),
	[MSTP501] /*SPUA 1*/	= MSTP(&spua_clk, SMSTPCR5, 1, 0),
	[MSTP500] /*SPUA 0*/	= MSTP(&spua_clk, SMSTPCR5, 0, 0),
};

#define CLKDEV_CON_ID(_id, _clk) { .con_id = _id, .clk = _clk }
#define CLKDEV_DEV_ID(_id, _clk) { .dev_id = _id, .clk = _clk }

static struct clk_lookup lookups[] = {
	/* main clocks */
	CLKDEV_CON_ID("r_clk", &r_clk),
	CLKDEV_CON_ID("extal1", &extal1_clk),
	CLKDEV_CON_ID("extal1_div2_clk", &extal1_div2_clk),
	CLKDEV_CON_ID("extal2", &extal2_clk),
	CLKDEV_CON_ID("extal2_div2_clk", &extal2_div2_clk),
	CLKDEV_CON_ID("extal2_div4_clk", &extal2_div4_clk),
	CLKDEV_CON_ID("main_clk", &main_clk),
	CLKDEV_CON_ID("main_div2_clk", &main_div2_clk),
	CLKDEV_CON_ID("pllc0_clk", &pllc0_clk),
	CLKDEV_CON_ID("pllc1_clk", &pllc1_clk),
	CLKDEV_CON_ID("pllc1_div2_clk", &pllc1_div2_clk),
	CLKDEV_CON_ID("pllc1_div7_clk", &pllc1_div7_clk),
	CLKDEV_CON_ID("pllc1_div13_clk", &pllc1_div13_clk),
	CLKDEV_CON_ID("pllc2_clk", &pllc2_clk),
	CLKDEV_CON_ID("pllc3_clk", &pllc3_clk),

	/* DIV4 clocks */
	CLKDEV_CON_ID("i_clk", &div4_clks[DIV4_I]),
	CLKDEV_CON_ID("zg_clk", &div4_clks[DIV4_ZG]),
	CLKDEV_CON_ID("m3_clk", &div4_clks[DIV4_M3]),
	CLKDEV_CON_ID("b_clk", &div4_clks[DIV4_B]),
	CLKDEV_CON_ID("m1_clk", &div4_clks[DIV4_M1]),
	CLKDEV_CON_ID("m2_clk", &div4_clks[DIV4_M2]),
	CLKDEV_CON_ID("zsel_clk", &div4_clks[DIV4_Z]),
	CLKDEV_CON_ID("ztr_clk", &div4_clks[DIV4_ZTR]),
	CLKDEV_CON_ID("zt_clk", &div4_clks[DIV4_ZT]),
	CLKDEV_CON_ID("zx_clk", &div4_clks[DIV4_ZX]),
	CLKDEV_CON_ID("zs_clk", &div4_clks[DIV4_ZS]),
	CLKDEV_CON_ID("hp_clk", &div4_clks[DIV4_HP]),
	CLKDEV_CON_ID("zb31_clk", &div4_clks[DIV4_ZB31]),
	CLKDEV_CON_ID("zb30_clk", &div4_clks[DIV4_ZB30]),

	/* DIV6 clocks */
	CLKDEV_CON_ID("zb_clk", &div6_clks[DIV6_ZB]),
	CLKDEV_CON_ID("sd0_clk", &div6_clks[DIV6_SD0]),
	CLKDEV_CON_ID("sd1_clk", &div6_clks[DIV6_SD1]),
	CLKDEV_CON_ID("sd2_clk", &div6_clks[DIV6_SD2]),
	CLKDEV_CON_ID("fl_clk", &div6_clks[DIV6_FL]),
	CLKDEV_CON_ID("vck1_clk", &div6_clks[DIV6_VCK1]),
	CLKDEV_CON_ID("vck2_clk", &div6_clks[DIV6_VCK2]),
	CLKDEV_CON_ID("vck3_clk", &div6_clks[DIV6_VCK3]),
	CLKDEV_CON_ID("msu_clk", &div6_clks[DIV6_MSU]),
	CLKDEV_CON_ID("hsi_clk", &div6_clks[DIV6_HSI]),
	CLKDEV_CON_ID("mf1_clk", &div6_clks[DIV6_MF1]),
	CLKDEV_CON_ID("mf2_clk", &div6_clks[DIV6_MF2]),
	CLKDEV_CON_ID("dsit_clk", &div6_clks[DIV6_DSIT]),
	CLKDEV_CON_ID("dsi0p_clk", &div6_clks[DIV6_DSI0P]),
	CLKDEV_CON_ID("dsi1p_clk", &div6_clks[DIV6_DSI1P]),

	/* sub clocks */
	CLKDEV_CON_ID("zb3_clk", &zb3_clk),
	CLKDEV_CON_ID("zb3s_clk", &zb3s_clk),
	CLKDEV_CON_ID("zb1_clk", &zb1_clk),
	CLKDEV_CON_ID("fsia_clk", &fsia_clk),
	CLKDEV_CON_ID("fsib_clk", &fsib_clk),
	CLKDEV_CON_ID("sub_clk", &sub_clk),
	CLKDEV_CON_ID("spua_clk", &spua_clk),
	CLKDEV_CON_ID("spuv_clk", &spuv_clk),
	CLKDEV_CON_ID("z_clk", &z_clk),

	/* MSTP32 clocks */
	CLKDEV_CON_ID("usb0_dmac", &mstp_clks[MSTP214]),
	CLKDEV_CON_ID("mmc0", &mstp_clks[MSTP312]),
	CLKDEV_CON_ID("usb0", &mstp_clks[MSTP322]),
	CLKDEV_CON_ID("i2c0", &mstp_clks[MSTP116]),
	CLKDEV_CON_ID("i2c1", &mstp_clks[MSTP323]),
	CLKDEV_CON_ID("i2c2", &mstp_clks[MSTP001]),
	CLKDEV_CON_ID("i2c3", &mstp_clks[MSTP411]),
	CLKDEV_CON_ID("keysc0", &mstp_clks[MSTP403]),
	CLKDEV_CON_ID("mmcif0", &mstp_clks[MSTP312]),

	CLKDEV_CON_ID("fsi", &mstp_clks[MSTP328]),
	CLKDEV_CON_ID("sgx", &mstp_clks[MSTP112]),
	CLKDEV_DEV_ID("r8a66597_udc.0", &mstp_clks[MSTP322]),

	CLKDEV_DEV_ID("sh_mobile_sdhi.0", &mstp_clks[MSTP314]),
	CLKDEV_DEV_ID("sh_mobile_sdhi.1", &mstp_clks[MSTP313]),
	CLKDEV_DEV_ID("sh_mmcif.0", &mstp_clks[MSTP312]),
	CLKDEV_DEV_ID("sh_mobile_sdhi.2", &mstp_clks[MSTP311]),

	CLKDEV_DEV_ID("sh-sci.7", &mstp_clks[MSTP219]),	/* SCIFA7 */
	CLKDEV_DEV_ID("sh-sci.5", &mstp_clks[MSTP207]),	/* SCIFA5 */
	CLKDEV_DEV_ID("sh-sci.8", &mstp_clks[MSTP206]),	/* SCIFB */
	CLKDEV_DEV_ID("sh-sci.0", &mstp_clks[MSTP204]),	/* SCIFA0 */
	CLKDEV_DEV_ID("sh-sci.1", &mstp_clks[MSTP203]),	/* SCIFA1 */
	CLKDEV_DEV_ID("sh-sci.2", &mstp_clks[MSTP202]),	/* SCIFA2 */
	CLKDEV_DEV_ID("sh-sci.3", &mstp_clks[MSTP201]),	/* SCIFA3 */
	CLKDEV_DEV_ID("sh-sci.4", &mstp_clks[MSTP200]),	/* SCIFA4 */
	CLKDEV_DEV_ID("sh-sci.6", &mstp_clks[MSTP331]),	/* SCIFA6 */
	CLKDEV_DEV_ID("sh_cmt.10", &mstp_clks[MSTP329]), /* CMT10 */
	CLKDEV_DEV_ID("sh_keysc.0", &mstp_clks[MSTP403]), /* KEYSC0 */
	CLKDEV_DEV_ID("i2c-sh_mobile.0", &mstp_clks[MSTP116]), /* I2C0 */
	CLKDEV_DEV_ID("i2c-sh_mobile.1", &mstp_clks[MSTP323]), /* I2C1 */
	CLKDEV_DEV_ID("i2c-sh_mobile.2", &mstp_clks[MSTP001]), /* I2C2 */
	CLKDEV_DEV_ID("i2c-sh_mobile.3", &mstp_clks[MSTP411]), /* I2C3 */
};

void __init sh73a0_clock_init(void)
{
	int k, ret = 0;

	if (__raw_readl((void __iomem *)FRQCRB) & (1 << 28))
		z_clk.parent = &div4_clks[DIV4_Z];

	for (k = 0; !ret && (k < ARRAY_SIZE(main_clks)); k++)
		ret = clk_register(main_clks[k]);

	if (!ret)
		ret = sh_clk_div4_register(div4_clks, DIV4_NR, &div4_table);

	if (!ret)
		ret = sh_clk_div6_register(div6_clks, DIV6_NR);

	for (k = 0; !ret && (k < ARRAY_SIZE(sub_clks)); k++)
		ret = clk_register(sub_clks[k]);

	if (!ret)
		ret = sh_clk_mstp32_register(mstp_clks, MSTP_NR);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	if (!ret)
		clk_init();
	else
		panic("failed to setup sh73a0 clocks\n");
}
