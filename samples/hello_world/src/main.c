/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <soc.h>


#define TEST_IRQ_0_PRIVATE	22
#define TEST_IRQ_0_SHARED	26

extern char __arc_isr_static_start[];
extern char __arc_isr_static_end[];

/* TODO: add locking */
static void arc_v2_irq_prio_check_single(uint32_t irq, uint32_t expected_prio)
{
	uint32_t prio;
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	prio = z_arc_v2_aux_reg_read(_ARC_V2_IRQ_PRIORITY);

	printk("ISR %u: priority read %u : %s\n", irq, prio, prio == expected_prio ? "OK" : "FAIL");
}


static void arc_v2_irq_check_masking_single(uint32_t irq)
{
	arch_irq_enable(irq);
	if (!arch_irq_is_enabled(irq)) {
		goto failed;
	}

	arch_irq_disable(irq);
	if (arch_irq_is_enabled(irq)) {
		goto failed;
	}

	arch_irq_enable(irq);
	if (!arch_irq_is_enabled(irq)) {
		goto failed;
	}

	arch_irq_disable(irq);
	if (arch_irq_is_enabled(irq)) {
		goto failed;
	}

	printk("ISR %u: masking OK\n", irq);

	return;

failed:
	printk("ISR %u: masking FAIL\n", irq);
}

static void arc_v2_irq_check_masking(void)
{
	arc_v2_irq_check_masking_single(TEST_IRQ_0_PRIVATE);
	arc_v2_irq_check_masking_single(TEST_IRQ_0_SHARED);
}

static void arc_v2_irq_check_default_state_single(uint32_t irq, bool expected)
{
	printk("ISR %u: default state: %s\n", irq, (!!arch_irq_is_enabled(irq) == expected) ? "OK" : "FAIL");
}

static void arc_v2_irq_check_default_state(void)
{
	arc_v2_irq_check_default_state_single(TEST_IRQ_0_PRIVATE, false);
	arc_v2_irq_check_default_state_single(TEST_IRQ_0_SHARED, false);

	arc_v2_irq_check_default_state_single(IRQ_TIMER0, true);

#ifdef CONFIG_ARC_CONNECT
	arc_v2_irq_check_default_state_single(19, true);
#endif /* CONFIG_ARC_CONNECT */
}

static void arc_v2_irq_check(void)
{
	struct arc_isr_sratic_data *start = (struct arc_isr_sratic_data *)__arc_isr_static_start;
	struct arc_isr_sratic_data *end = (struct arc_isr_sratic_data *)__arc_isr_static_end;


	printk("section from %u to %u\n", (uint32_t)start, (uint32_t)end);

	while (start < end) {
		printk("ISR %u: priority static encoded %u\n", start->irq, start->priority);
		arc_v2_irq_prio_check_single(start->irq, start->priority);
		start++;
	}

	arc_v2_irq_check_default_state();
	arc_v2_irq_check_masking();
}

static void sched_ipi_handler0(const void *unused) {}
static void sched_ipi_handler1(const void *unused) {}
static void sched_ipi_handler2(const void *unused) {}
static void sched_ipi_handler3(const void *unused) {}
static void sched_ipi_handler4(const void *unused) {}
static void sched_ipi_handler5(const void *unused) {}

static void arc_v2_irq_connect(void)
{
	IRQ_CONNECT(TEST_IRQ_0_PRIVATE, 1, sched_ipi_handler0, NULL, 0);
	arc_v2_irq_prio_check_single(TEST_IRQ_0_PRIVATE, 1);
	IRQ_CONNECT(17, 0, sched_ipi_handler1, NULL, 0);
	arc_v2_irq_prio_check_single(17, 0);
	IRQ_CONNECT(18, 1, sched_ipi_handler2, NULL, 0);
	arc_v2_irq_prio_check_single(18, 1);

	/* shared irq in case of SMP */
	IRQ_CONNECT(27, 1, sched_ipi_handler4, NULL, 0);
	arc_v2_irq_prio_check_single(27, 1);
	IRQ_CONNECT(26, 0, sched_ipi_handler3, NULL, 0);
	arc_v2_irq_prio_check_single(26, 0);
	IRQ_CONNECT(28, 0, sched_ipi_handler5, NULL, 0);
	arc_v2_irq_prio_check_single(28, 0);
}

void main(void)
{
	arc_v2_irq_connect();

	printk("Hello World! %s\n", CONFIG_BOARD);
	arc_v2_irq_check();
}
