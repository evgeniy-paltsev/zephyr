/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 Interrupt Unit device driver
 *
 * The ARCv2 interrupt unit has 16 allocated exceptions associated with
 * vectors 0 to 15 and 240 interrupts associated with vectors 16 to 255.
 * The interrupt unit is optional in the ARCv2-based processors. When
 * building a processor, you can configure the processor to include an
 * interrupt unit. The ARCv2 interrupt unit is highly programmable.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <init.h>


#ifdef CONFIG_ARC_SECURE_FIRMWARE
/* lowest priority */
#define DEFAULT_IRQ_PRIO ((CONFIG_NUM_IRQ_PRIO_LEVELS - 1) | _ARC_V2_IRQ_PRIORITY_SECURE)
#else
/* lowest priority */
#define DEFAULT_IRQ_PRIO (CONFIG_NUM_IRQ_PRIO_LEVELS - 1)
#endif

extern char __arc_isr_static_start[];
extern char __arc_isr_static_end[];

static void arc_v2_irq_init_prio(void)
{
	struct arc_isr_sratic_data *start = (struct arc_isr_sratic_data *)__arc_isr_static_start;
	struct arc_isr_sratic_data *end = (struct arc_isr_sratic_data *)__arc_isr_static_end;

	__ASSERT(start < end, "no static irq configuration");
	__ASSERT(start->irq >= CONFIG_GEN_IRQ_START_VECTOR, "incorrect static irq number");

	/*
	 * If interrupt is connected in build-time extract priority from
	 * static configuration.
	 */
	for (uint32_t irq = CONFIG_GEN_IRQ_START_VECTOR; irq < CONFIG_NUM_IRQS; irq++) {
		if (irq == start->irq && start < end) {
			/* TODO: get rid of z_irq_priority_set usage - as it locks interrupts */
			z_irq_priority_set(irq, start->priority, 0);
			start++;
		} else {
			z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
			z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY, DEFAULT_IRQ_PRIO);
		}
	}
}

#ifdef CONFIG_ARC_CONNECT
static void arc_shared_irq_init(void)
{
	/*
	* Initialize all IDU interrupts:
	* - select round-robbin
	* - disable lines
	*/
	BUILD_ASSERT(CONFIG_NUM_IRQS > ARC_CONNECT_IDU_IRQ_START);
	__ASSERT(z_arc_v2_core_id() == 0, "idu interrupts must be inited from master core");

	z_arc_connect_idu_disable();

	for (uint32_t i = 0; i < (CONFIG_NUM_IRQS - ARC_CONNECT_IDU_IRQ_START); i++) {
		/*
		 * TODO: don't use z_arc_connect_idu* finctions to avoid
		 * locking/unlocking every time
		 */
		z_arc_connect_idu_set_mode(i, ARC_CONNECT_INTRPT_TRIGGER_LEVEL, ARC_CONNECT_DISTRI_MODE_ROUND_ROBIN);
		z_arc_connect_idu_set_dest(i, GENMASK(CONFIG_MP_NUM_CPUS, 0));

		/* Disable (mask) line */
		z_arc_connect_idu_set_mask(i, 0x1);
	}

	z_arc_connect_idu_enable();

}
#else
#define arc_shared_irq_init()
#endif /* CONFIG_ARC_CONNECT */

/*
 * @brief Initialize the interrupt unit device driver
 *
 * Initializes the interrupt unit device driver and the device
 * itself.
 *
 * Interrupts are still locked at this point, so there is no need to protect
 * the window between a write to IRQ_SELECT and subsequent writes to the
 * selected IRQ's registers.
 *
 * @return 0 for success
 */
void arc_per_core_irq_init(void)
{
	arc_v2_irq_init_prio();

	/* Interrupts from 0 to 15 are exceptions and they are ignored
	 * by IRQ auxiliary registers. For that reason we skip those
	 * values in this loop.
	 */
	BUILD_ASSERT(CONFIG_GEN_IRQ_START_VECTOR == 16);

#ifdef CONFIG_ARC_CONNECT
	/*
	 * In case of IDU usage we
	 *  - disabe private IRQs (will be enabled with arch_irq_enable later)
	 *  - enable shared (IDU) IRQs - they are enabled / disabled by IDU
	 */
	for (uint32_t irq = CONFIG_GEN_IRQ_START_VECTOR; irq < ARC_CONNECT_IDU_IRQ_START; irq++) {

		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_ENABLE, _ARC_V2_INT_DISABLE);
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER, _ARC_V2_INT_LEVEL);
	}

	for (uint32_t irq = ARC_CONNECT_IDU_IRQ_START; irq < CONFIG_NUM_IRQS; irq++) {

		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_ENABLE, _ARC_V2_INT_ENABLE);
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER, _ARC_V2_INT_LEVEL);
	}
#else
	for (uint32_t irq = CONFIG_GEN_IRQ_START_VECTOR; irq < CONFIG_NUM_IRQS; irq++) {

		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_ENABLE, _ARC_V2_INT_DISABLE);
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER, _ARC_V2_INT_LEVEL);
	}
#endif /* CONFIG_ARC_CONNECT */
}

static int arc_irq_init(const struct device *unused)
{
	arc_shared_irq_init();
	/*
	* We initialize per-core part for core 0 here,
	* for rest cores it will be initialized in slave_start
	*/
	arc_per_core_irq_init();

	return 0;
}

SYS_INIT(arc_irq_init, PRE_KERNEL_1, 0);
