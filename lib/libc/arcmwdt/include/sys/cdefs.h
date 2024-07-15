/*
 * Copyright (c) 2023 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_ARC
/* MWDT ARC classic has no it's own cdefs. Add this empty one for satisfy dependencies */
#else
#include_next <sys/cdefs.h>
#endif
