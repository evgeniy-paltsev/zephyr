/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <tc_util.h>
#include <ztest.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/kernel_structs.h>

#if CONFIG_MP_NUM_CPUS < 2
#error SMP test requires at least two CPUs!
#endif

#define T2_STACK_SIZE (2048 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define DELAY_US 50000
#define TIMEOUT 1000
#define EQUAL_PRIORITY 1
#define TIME_SLICE_MS 500
#define THREAD_DELAY 1
#define SLEEP_MS_LONG 15000

struct k_thread t2;
K_THREAD_STACK_DEFINE(t2_stack, T2_STACK_SIZE);


K_SEM_DEFINE(cpuid_sema, 0, 1);
K_SEM_DEFINE(sema, 0, 1);

#define THREADS_NUM CONFIG_MP_NUM_CPUS

struct thread_info {
	k_tid_t tid;
	int executed;
	int priority;
	int cpu_id;
};
static ZTEST_BMEM volatile struct thread_info tinfo[THREADS_NUM];
static struct k_thread tthread[THREADS_NUM];
static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREADS_NUM, STACK_SIZE);

static struct k_poll_signal tsignal[THREADS_NUM];
static struct k_poll_event tevent[THREADS_NUM];

#ifdef CONFIG_TRACE_SCHED_IPI
/* global variable for testing send IPI */
static volatile int sched_ipi_has_called;

void z_trace_sched_ipi(void)
{
	sched_ipi_has_called++;
}
#endif



/**
 * @brief Torture test for context switching code
 *
 * @ingroup kernel_smp_tests
 *
 * @details Leverage the polling API to stress test the context switching code.
 *          This test will hammer all the CPUs with thread swapping requests.
 */
static void process_events(void *arg0, void *arg1, void *arg2)
{
	uintptr_t id = (uintptr_t) arg0;

	while (1) {
		k_poll(&tevent[id], 1, K_FOREVER);

		if (tevent[id].signal->result != 0x55) {
			ztest_test_fail();
		}

		tevent[id].signal->signaled = 0;
		tevent[id].state = K_POLL_STATE_NOT_READY;

		k_poll_signal_reset(&tsignal[id]);
	}
}

static void signal_raise(void *arg0, void *arg1, void *arg2)
{
	while (1) {
		for (uintptr_t i = 0; i < THREADS_NUM; i++) {
			k_poll_signal_raise(&tsignal[i], 0x55);
		}
	}
}

void test_smp_switch_torture(void)
{
	for (uintptr_t i = 0; i < THREADS_NUM; i++) {
		k_poll_signal_init(&tsignal[i]);
		k_poll_event_init(&tevent[i], K_POLL_TYPE_SIGNAL,
				  K_POLL_MODE_NOTIFY_ONLY, &tsignal[i]);

		k_thread_create(&tthread[i], tstack[i], STACK_SIZE,
				(k_thread_entry_t) process_events,
				(void *) i, NULL, NULL, K_PRIO_PREEMPT(i + 1),
				K_INHERIT_PERMS, K_NO_WAIT);
	}

	k_thread_create(&t2, t2_stack, T2_STACK_SIZE, signal_raise,
			NULL, NULL, NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);

	k_sleep(K_MSEC(SLEEP_MS_LONG));

	k_thread_abort(&t2);
	for (uintptr_t i = 0; i < THREADS_NUM; i++) {
		k_thread_abort(&tthread[i]);
	}
}

void test_main(void)
{
	/* Sleep a bit to guarantee that both CPUs enter an idle
	 * thread from which they can exit correctly to run the main
	 * test.
	 */
	k_sleep(K_MSEC(10));

	ztest_test_suite(smp,
			 ztest_unit_test(test_smp_switch_torture)
			 );
	ztest_run_test_suite(smp);
}
