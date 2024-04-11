/*
 * Copyright (c) 2018,2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS > 1);

ZTEST_SUITE(spinlock, NULL, NULL, NULL, NULL, NULL);
