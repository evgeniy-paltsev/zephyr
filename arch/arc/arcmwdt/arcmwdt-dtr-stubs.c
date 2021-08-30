/*
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

__weak void *__dso_handle = 0;

int __cxa_atexit(void (*destructor)(void *), void *objptr, void *dso)
{
	ARG_UNUSED(destructor);
	ARG_UNUSED(objptr);
	ARG_UNUSED(dso);
	return 0;
}

int atexit(void (*function)(void))
{
	return 0;
}
