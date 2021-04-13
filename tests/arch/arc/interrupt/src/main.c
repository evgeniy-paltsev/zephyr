/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_thread_to_irq_to_irq(void);

void test_main(void)
{
	ztest_test_suite(interrupt_feature,
			 ztest_unit_test(test_thread_to_irq_to_irq)
			);
	ztest_run_test_suite(interrupt_feature);
}
