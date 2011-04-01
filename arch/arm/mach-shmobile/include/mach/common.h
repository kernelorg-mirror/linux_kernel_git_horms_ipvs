#ifndef __ARCH_MACH_COMMON_H
#define __ARCH_MACH_COMMON_H

extern struct sys_timer shmobile_timer;
extern void shmobile_setup_console(void);
extern void shmobile_secondary_vector(void);
extern int clk_init(void);
extern void (*shmobile_arch_reset)(char mode, const char *cmd);

extern void sh7367_init_irq(void);
extern void sh7367_add_early_devices(void);
extern void sh7367_add_standard_devices(void);
extern void sh7367_clock_init(void);
extern void sh7367_pinmux_init(void);

extern void sh7377_init_irq(void);
extern void sh7377_add_early_devices(void);
extern void sh7377_add_standard_devices(void);
extern void sh7377_pinmux_init(void);

extern void sh7372_init_irq(void);
extern void sh7372_add_early_devices(void);
extern void sh7372_add_standard_devices(void);
extern void sh7372_pinmux_init(void);

extern void sh73a0_init_irq(void);
extern void sh73a0_add_early_devices(void);
extern void sh73a0_add_standard_devices(void);
extern void sh73a0_clock_init(void);
extern void sh73a0_pinmux_init(void);

extern unsigned int sh73a0_get_core_count(void);
extern void sh73a0_secondary_init(unsigned int cpu);
extern int sh73a0_boot_secondary(unsigned int cpu);
extern void sh73a0_smp_prepare_cpus(void);

#endif /* __ARCH_MACH_COMMON_H */
