/*
 * Copyright (c) 2021 Evgeniy Paltsev
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <interrupt_util.h>

#include "thread-to-irq.h"

/*
 * In terms of priority, the IRQ1 is triggered from the ISR of the IRQ0;
 * therefore, the priority of IRQ1 must be greater than that of the IRQ0.
 */
#define IRQ0_PRIO	1
#define IRQ1_PRIO	0


void irq0_handler(const void *param);
void irq1_handler(const void *param);
void _payload_thread(void);

volatile uint32_t arc_isr0_token = 0;
volatile uint32_t arc_isr1_token = 0;

/* Thread SP before and after interruption by interrupt 0 */
volatile uintptr_t arc_thread_sp_before = 0;
volatile uintptr_t arc_thread_sp_after = 0;

/* Interrupt 0 SP before and after interruption by interrupt 1 */
volatile uintptr_t arc_irq0_sp_before = 0;
volatile uintptr_t arc_irq0_sp_after = 0;

volatile uint32_t arc_irq0_regs_1to12_ok = 0;
volatile uint32_t arc_irq0_regs_13_ok = 0;
volatile uint32_t arc_irq0_regs_14to25_ok = 0;

volatile uint32_t arc_thread_regs_1to12_ok = 0;
volatile uint32_t arc_thread_regs_13_ok = 0;
volatile uint32_t arc_thread_regs_14to25_ok = 0;

BUILD_ASSERT(_NUM_COOP_PRIO != 0, "No COOP threads!");
BUILD_ASSERT(_NUM_PREEMPT_PRIO != 0, "No PREEMPT threads!");


#ifdef CONFIG_FROM_COOP_THREAD
#define PAYLOAD_THREAD_PRIO	K_PRIO_COOP(0)
#else
#define PAYLOAD_THREAD_PRIO	K_PRIO_PREEMPT(0)
#endif

#define STACK_SIZE	(1024 + CONFIG_TEST_EXTRA_STACKSIZE)

static struct k_thread tdata;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);

/**
 * @brief Thread to interrupt to interrupt register context verify
  */
static void payload_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	_payload_thread();
}

void test_thread_to_irq_to_irq(void)
{
	/* Connect and enable test IRQs */
	arch_irq_connect_dynamic(IRQ0_LINE, IRQ0_PRIO, irq0_handler, NULL, 0);
	arch_irq_connect_dynamic(IRQ1_LINE, IRQ1_PRIO, irq1_handler, NULL, 0);

	irq_enable(IRQ0_LINE);
	irq_enable(IRQ1_LINE);

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				payload_thread, NULL, NULL, NULL,
				PAYLOAD_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	k_sleep(K_MSEC(400));

	printk("payload thread priority: %d\n", PAYLOAD_THREAD_PRIO);

	/* Validate ISR result token */
	zassert_equal(arc_isr0_token, ISR0_TOKEN, "isr0 did not execute");
	zassert_equal(arc_isr1_token, ISR1_TOKEN, "isr1 did not execute");

	/* Validate that thread SP didn't change after IRQ0 trigger */
	printk("thread: SP: 0x%" PRIxPTR "\n", arc_thread_sp_after);
	zassert_not_equal(arc_thread_sp_after, 0, "thread SP not set");
	zassert_equal(arc_thread_sp_before, arc_thread_sp_after, "thread SP mismatch");

	/* Validate that IRQ0 SP didn't change after IRQ1 trigger */
	printk("irq0: SP: 0x%" PRIxPTR "\n", arc_irq0_sp_after);
	zassert_not_equal(arc_irq0_sp_after, 0, "irq0 SP not set");
	zassert_equal(arc_irq0_sp_before, arc_irq0_sp_after, "irq0 SP mismatch");

	zassert_equal(arc_irq0_regs_1to12_ok, IRQ0_R_1TO12_OK, "irq0 regs r1-r12 corrupted");
	zassert_equal(arc_irq0_regs_13_ok, IRQ0_R_13_OK, "irq0 reg r13 corrupted");
	zassert_equal(arc_irq0_regs_14to25_ok, IRQ0_R_14TO25_OK, "irq0 regs r14-r25 corrupted");

	zassert_equal(arc_thread_regs_1to12_ok, THR_R_1TO12_OK, "thread regs r1-r12 corrupted");
	zassert_equal(arc_thread_regs_13_ok, THR_R_13_OK, "thread reg r13 corrupted");
	zassert_equal(arc_thread_regs_14to25_ok, THR_R_14TO25_OK, "thread regs r14-r25 corrupted");

	/* Clear the spawn thread to avoid side effect */
	k_thread_abort(tid);

}
