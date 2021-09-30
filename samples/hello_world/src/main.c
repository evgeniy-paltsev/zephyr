/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

// extern void *__arc_isr_static_start;
// extern void *__arc_isr_static_end;

extern char __arc_isr_static_start[];
extern char __arc_isr_static_end[];

static void arc_v2_irq_init_prio(void)
{
	struct arc_isr_sratic_data *start = (struct arc_isr_sratic_data *)__arc_isr_static_start;
	struct arc_isr_sratic_data *end = (struct arc_isr_sratic_data *)__arc_isr_static_end;


	printk("section from %u to %u\n", (uint32_t)start, (uint32_t)end);

	while (start < end) {
		printk("ISR %u: %u\n", start->irq, start->priority);
		start++;
	}
}

static void sched_ipi_handler0(const void *unused) {}
static void sched_ipi_handler(const void *unused) {}
static void sched_ipi_handler2(const void *unused) {}

void main(void)
{
	IRQ_CONNECT(22, 2, sched_ipi_handler0, NULL, 0);
	IRQ_CONNECT(19, 2, sched_ipi_handler, NULL, 0);
	IRQ_CONNECT(18, 2, sched_ipi_handler2, NULL, 0);

	printk("Hello World! %s\n", CONFIG_BOARD);
	arc_v2_irq_init_prio();
}
