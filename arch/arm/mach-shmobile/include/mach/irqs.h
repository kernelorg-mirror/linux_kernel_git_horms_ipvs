#ifndef __ASM_MACH_IRQS_H
#define __ASM_MACH_IRQS_H

#define NR_IRQS         1024
#define NR_IRQS_LEGACY  8

/* GIC */
#define gic_spi(nr)		((nr) + 32)

/* INTCA */
#define evt2irq(evt)		(((evt) >> 5) - 16)
#define irq2evt(irq)		(((irq) + 16) << 5)

/* INTCS */
#define INTCS_VECT_BASE		0x2200
#define INTCS_VECT(n, vect)	INTC_VECT((n), INTCS_VECT_BASE + (vect))
#define intcs_evt2irq(evt)	evt2irq(INTCS_VECT_BASE + (evt))

/* Soft IRQ - allocate unused IRQ resources at the end of the INTCS IRQs */
#define SOFT_IRQ_BASE		(512 - 16)
#define soft_irq(nr)		((nr) + SOFT_IRQ_BASE)

/* PINT (for sh73a0) */
#define PINT_IRQ_BASE		512
#define pint2irq(bit)		(PINT_IRQ_BASE + (bit))

#endif /* __ASM_MACH_IRQS_H */
