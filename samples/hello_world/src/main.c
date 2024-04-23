/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/atomic.h>


#define TICKET_SPINLOCKS_measure 1

static struct k_spinlock lock;


k_spinlock_key_t __attribute__ ((noinline)) k_spin_lock_cust(struct k_spinlock *l)
{
	k_spinlock_key_t k;

	/* Note that we need to use the underlying arch-specific lock
	 * implementation.  The "irq_lock()" API in SMP context is
	 * actually a wrapper for a global spinlock!
	 */
	k.key = arch_irq_lock();

#if TICKET_SPINLOCKS_measure
	/*
	 * Enqueue ourselves to the end of a spinlock waiters queue
	 * receiving a ticket
	 */
	atomic_val_t ticket = atomic_inc(&l->tail);
	/* Spin until our ticket is served */
	while (atomic_get(&l->owner) != ticket) {
		arch_spin_relax();
	}
#else
	while (!atomic_cas(&l->locked, 0, 1)) {
		arch_spin_relax();
	}
#endif /* CONFIG_TICKET_SPINLOCKS */

	return k;
}


void __attribute__ ((noinline)) k_spin_unlock_cust(struct k_spinlock *l, k_spinlock_key_t key)
{

#if TICKET_SPINLOCKS_measure
	/* Give the spinlock to the next CPU in a FIFO */
	atomic_inc(&l->owner);
#else
	/* Strictly we don't need atomic_clear() here (which is an
	 * exchange operation that returns the old value).  We are always
	 * setting a zero and (because we hold the lock) know the existing
	 * state won't change due to a race.  But some architectures need
	 * a memory barrier when used like this, and we don't have a
	 * Zephyr framework for that.
	 */
	atomic_clear(&l->locked);
#endif /* CONFIG_TICKET_SPINLOCKS */
	arch_irq_unlock(key.key);
}

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD);

	k_spinlock_key_t spinlock_key = k_spin_lock_cust(&lock);
	k_spin_unlock_cust(&lock, spinlock_key);

	return 0;
}
