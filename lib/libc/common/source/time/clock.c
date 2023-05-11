/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>

#ifndef CLOCKS_PER_SEC
#warning libc does not define CLOCKS_PER_SEC which is required by ISO
#define CLOCKS_PER_SEC CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#endif

clock_t clock(void)
{
	clock_t clk;

	clk = ((clock_t)k_uptime_get() * CLOCKS_PER_SEC) / MSEC_PER_SEC;
	if (clk == (clock_t)-1) {
		/* (clock_t)-1 represents an error so roll over to 0 */
		clk = 0;
	}

	return clk;
}
