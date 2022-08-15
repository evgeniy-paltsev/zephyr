# Copyright (c) 2021 Synopsys, Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

if SOC_QEMU_ARC_HS

config CPU_HS3X
	default y

config ARC_CONNECT
	default y

config MP_NUM_CPUS
	default 2

endif
