/*
 * SuperH FLCTL nand controller
 *
 * Copyright © 2008 Renesas Solutions Corp.
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

#ifndef __SH_FLCTL_H__
#define __SH_FLCTL_H__

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

/* FLCTL registers */
#define FLCMNCR(f)		(f->reg + 0x0)
#define FLCMDCR(f)		(f->reg + 0x4)
#define FLCMCDR(f)		(f->reg + 0x8)
#define FLADR(f)		(f->reg + 0xC)
#define FLADR2(f)		(f->reg + 0x3C)
#define FLDATAR(f)		(f->reg + 0x10)
#define FLDTCNTR(f)		(f->reg + 0x14)
#define FLINTDMACR(f)		(f->reg + 0x18)
#define FLBSYTMR(f)		(f->reg + 0x1C)
#define FLBSYCNT(f)		(f->reg + 0x20)
#define FLDTFIFO(f)		(f->reg + 0x50)
#define FLECFIFO(f)		(f->reg + 0x60)
#define FLTRCR(f)		(f->reg + 0x2C)
#define FLHOLDCR(f)		(f->reg + 0x38)
#define	FL4ECCRESULT0(f)	(f->reg + 0x80)
#define	FL4ECCRESULT1(f)	(f->reg + 0x84)
#define	FL4ECCRESULT2(f)	(f->reg + 0x88)
#define	FL4ECCRESULT3(f)	(f->reg + 0x8C)
#define	FL4ECCCR(f)		(f->reg + 0x90)
#define	FL4ECCCNT(f)		(f->reg + 0x94)
#define	FLERRADR(f)		(f->reg + 0x98)
#define FLGECCCR(f)		(f->reg + 0x400)
#define FLGECCSR(f)		(f->reg + 0x404)
#define FLGECCINTCR(f)		(f->reg + 0x408)
#define FLGECCRSTR(f)		(f->reg + 0x40C)
#define FLGECCERCNTR(f)		(f->reg + 0x410)
#define FLSYSREG0(f)		(f->reg + 0x1000)

/* DMAC registers */
#define DMASAR(f)		(f->dmareg + 0x0) /* DMA source addr */
#define DMADAR(f)		(f->dmareg + 0x4) /* DMA destination addr */
#define DMATCR(f)		(f->dmareg + 0x8) /* DMA transfer count */
#define DMACHCR(f)		(f->dmareg + 0xC) /* DMA channel control */
#define DMARS(f)		(f->dmareg + 0x40) /* DMA extended resource */

/* FLCMNCR control bits */
#define PULSE3		(0x1 << 27)	/* Flash Clock Select */
#define FLMLC		(0x1 << 26)	/* 0:512+16 1:2048+64 */
#define ECCPOS2		(0x1 << 25)
#define _4ECCCNTEN	(0x1 << 24)
#define _4ECCEN		(0x1 << 23)
#define _4ECCCORRECT	(0x1 << 22)
#define BUSYON		(0x1 << 21)	/* 0:Hold bus 1:Release bus */
#define SHBUSSEL	(0x1 << 20)
#define SEL_16BIT	(0x1 << 19)
#define SNAND_E		(0x1 << 18)	/* SNAND (0=512 1=2048)*/
#define PULSE2		(0x1 << 17)	/* Flash Clock Select */
#define ENDIAN		(0x1 << 16)	/* 1 = little endian */
#define PULSE1		(0x1 << 15)	/* Flash Clock Select */
#define ECCPOS_00	(0x00 << 12)
#define ECCPOS_01	(0x01 << 12)
#define ECCPOS_02	(0x02 << 12)
#define ACM_CACCES_MODE	(0x00 << 10)	/* Command access mode */
#define ACM_SACCES_MODE	(0x01 << 10)	/* Sector access mode */
#define ACM_HACCES_MODE	(0x02 << 10)	/* High-speed sector access mode */
#define PULSE0		(0x1 << 9)	/* Flash Clock Select */
#define	CE1_ENABLE	(0x1 << 4)	/* Chip Enable 1 */
#define	CE0_ENABLE	(0x1 << 3)	/* Chip Enable 0 */
#define	TYPESEL_SET	(0x1 << 0)

/* FLCMDCR control bits */
#define ADRCNT2_E	(0x1 << 31)	/* 5byte address enable */
#define SCTCNT_19	(0x1 << 30)	/* Sector Transfer Count */
#define SCTCNT_18	(0x1 << 29)	/* Sector Transfer Count */
#define SCTCNT_17	(0x1 << 28)	/* Sector Transfer Count */
#define SCTCNT_16	(0x1 << 27)	/* Sector Transfer Count */
#define ADRMD_E		(0x1 << 26)	/* Sector address access */
#define CDSRC_E		(0x1 << 25)	/* Data buffer selection */
#define DOSR_E		(0x1 << 24)	/* Status read check */
#define SELRW		(0x1 << 21)	/*  0:read 1:write */
#define DOADR_E		(0x1 << 20)	/* Address stage execute */
#define ADRCNT_1	(0x00 << 18)	/* Address data bytes: 1byte */
#define ADRCNT_2	(0x01 << 18)	/* Address data bytes: 2byte */
#define ADRCNT_3	(0x02 << 18)	/* Address data bytes: 3byte */
#define ADRCNT_4	(0x03 << 18)	/* Address data bytes: 4byte */
#define DOCMD2_E	(0x1 << 17)	/* 2nd cmd stage execute */
#define DOCMD1_E	(0x1 << 16)	/* 1st cmd stage execute */

/* FLTRCR control bits */
#define TRSTRT		(0x1 << 0)	/* translation start */
#define TREND		(0x1 << 1)	/* translation end */
#define TRSTATUS	(0x1 << 2)	/* transfer state */

/* FL4ECCCR control bits */
#define	_4ECCFA		(0x1 << 2)	/* 4 symbols correct fault */
#define	_4ECCEND	(0x1 << 1)	/* 4 symbols end */
#define	_4ECCEXST	(0x1 << 0)	/* 4 symbols exist */

#define INIT_FL4ECCRESULT_VAL	0x03FF03FF
#define LOOP_TIMEOUT_MAX	0x00030000

#define LBA_NAND_SDA_SIZE (32 << 20)
struct sh_flctl {
	struct mtd_info		mtd;
	struct nand_chip	chip;
	struct platform_device	*pdev;
	void __iomem		*reg;
	void __iomem		*dmareg;
	void __iomem		*dmabuf;

	uint8_t	done_buff[2048 + 64];	/* max size 2048 + 64 */
	int	read_bytes;
	int	index;
	int	seqin_column;		/* column in SEQIN cmd */
	int	seqin_page_addr;	/* page_addr in SEQIN cmd */
	uint32_t seqin_read_cmd;		/* read cmd in SEQIN cmd */
	int	erase1_page_addr;	/* page_addr in ERASE1 cmd */
	uint32_t erase_ADRCNT;		/* bits of FLCMDCR in ERASE1 cmd */
	uint32_t rw_ADRCNT;	/* bits of FLCMDCR in READ WRITE cmd */

	int	hwecc_cant_correct[4];

	unsigned page_size:1;	/* NAND page size (0 = 512, 1 = 2048) */
	unsigned hwecc:1;	/* Hardware ECC (0 = disabled, 1 = enabled) */
};

struct sh_flctl_platform_data {
	struct mtd_partition	*parts;
	int			nr_parts;
	unsigned long		flcmncr_val;

	unsigned has_hwecc:1;
};

static inline struct sh_flctl *mtd_to_flctl(struct mtd_info *mtdinfo)
{
	return container_of(mtdinfo, struct sh_flctl, mtd);
}

#endif	/* __SH_FLCTL_H__ */
