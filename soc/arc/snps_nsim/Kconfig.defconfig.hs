# Copyright (c) 2019 Synopsys, Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

if SOC_NSIM_HS

config NUM_IRQ_PRIO_LEVELS
	# This processor supports 16 priority levels:
	# 0 for Fast Interrupts (FIRQs) and 1-15 for Regular Interrupts (IRQs).
	default 2

config NUM_IRQS
	# must be > the highest interrupt number used
	default 30

config RGF_NUM_BANKS
	default 1

config SYS_CLOCK_HW_CYCLES_PER_SEC
	default 5000000

config HARVARD
	default y

config ARC_FIRQ
	default n

config CACHE_FLUSHING
	default y

endif # SOC_NSIM_HS
