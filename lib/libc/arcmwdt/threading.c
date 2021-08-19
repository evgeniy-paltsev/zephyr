/*
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/__assert.h>
#include <sys/mutex.h>

#ifdef CONFIG_MULTITHREADING

/* TODO: use definition from mw/inc/internal/thread.h header */
typedef void *_lock_t;

K_MUTEX_DEFINE(__lock____mwheap_lock);

extern _lock_t _mwheap_lock;

static int arcmwdt_locks_prepare(const struct device *unused)
{
	ARG_UNUSED(unused);

	k_object_access_all_grant(&__lock____mwheap_lock);
	_mwheap_lock = &__lock____mwheap_lock;

	return 0;
}

SYS_INIT(arcmwdt_locks_prepare, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

void _mwmutex_create(_lock_t *mutex_ptr)
{
	/* _mwheap_lock is already allocated and initialized */
	if (*mutex_ptr == _mwheap_lock) {
		return;
	}

	/* Allocate mutex object */
#ifndef CONFIG_USERSPACE
	*mutex_ptr = k_malloc(sizeof(struct k_mutex));
#else
	*mutex_ptr = k_object_alloc(K_OBJ_MUTEX);
#endif /* !CONFIG_USERSPACE */
	__ASSERT(*mutex_ptr != NULL, "MWDT lock allocation failed");

	k_mutex_init((struct k_mutex *)*mutex_ptr);
}

void _mwmutex_delete(_lock_t *mutex_ptr)
{
	__ASSERT_NO_MSG(mutex_ptr != NULL);
#ifndef CONFIG_USERSPACE
	k_free(mutex_ptr);
#else
	k_object_release(mutex_ptr);
#endif /* !CONFIG_USERSPACE */
}

/* Wait on the mutex semaphore if we don't already own it.
 * See internal/thread.h to see how it's done for pthreads.
 */
void _mwmutex_lock(_lock_t mutex)
{
	__ASSERT_NO_MSG(mutex != NULL);
	k_mutex_lock((struct k_mutex *)mutex, K_FOREVER);
}

/* Signal the mutex semaphore or decrement the wait count
 * if we called _mwmutex_lock more than once.
 * See internal/thread.h to see how it's done for pthreads.
 */
void _mwmutex_unlock(_lock_t mutex)
{
	__ASSERT_NO_MSG(mutex != NULL);
	k_mutex_unlock((struct k_mutex *)mutex);
}
#endif /* CONFIG_MULTITHREADING */
