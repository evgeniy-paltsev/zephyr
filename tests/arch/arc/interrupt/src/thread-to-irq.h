#ifndef ZEPHYR_TESTS_ARCH_ARC_THREAD_TO_IRQ_
#define ZEPHYR_TESTS_ARCH_ARC_THREAD_TO_IRQ_

#define ISR0_TOKEN	0xDEADBEEF
#define ISR1_TOKEN	0xCAFEBABE

/* Use the last two available IRQ lines for testing */
#define IRQ0_LINE	(CONFIG_NUM_IRQS - 1)
#define IRQ1_LINE	(CONFIG_NUM_IRQS - 2)

#define IRQ_REG_TOKEN(irq, reg)		(0x56780000 + (((irq) + 1) << 8) + (reg))
#define THREAD_REG_TOKEN(reg)		(0x12340000 + ((reg) << 8) + (reg))

#define IRQ0_R_1TO12_OK			2
#define IRQ0_R_13_OK			3
#define IRQ0_R_14TO25_OK		4

#define THR_R_1TO12_OK			5
#define THR_R_13_OK			6
#define THR_R_14TO25_OK			7

#endif /* ZEPHYR_TESTS_ARCH_ARC_THREAD_TO_IRQ_ */
