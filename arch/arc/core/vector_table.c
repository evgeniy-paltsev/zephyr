/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Populated exception vector table
 *
 * Vector table with exceptions filled in. The reset vector is the system entry
 * point, ie. the first instruction executed.
 *
 * The table is populated with all the system exception handlers. No exception
 * should not be triggered until the kernel is ready to handle them.
 *
 * We are using a C file instead of an assembly file (like the ARM vector table)
 * to work around an issue with the assembler where:
 *
 *   .word <function>
 *
 * statements would end up with the two half-words of the functions' addresses
 * swapped.
 */

#include <zephyr/types.h>
#include <toolchain.h>
#include "vector_table.h"

#ifdef CONFIG_64BIT

#if 0
struct vector_table {
	uint64_t reset;
	uint64_t memory_error;
	uint64_t instruction_error;
	uint64_t ev_machine_check;
	uint64_t ev_tlb_miss_i;
	uint64_t ev_tlb_miss_d;
	uint64_t ev_prot_v;
	uint64_t ev_privilege_v;
	uint64_t ev_swi;
	uint64_t ev_trap;
	uint64_t ev_extension;
	uint64_t ev_div_zero;
	uint64_t unused_1;
	uint64_t ev_maligned;
	uint64_t unused_2;
	uint64_t unused_3;
};

struct vector_table _VectorTable Z_GENERIC_SECTION(.exc_vector_table) = {
	(uint64_t)__reset,
	(uint64_t)__memory_error,
	(uint64_t)__instruction_error,
	(uint64_t)__ev_machine_check,
	(uint64_t)__ev_tlb_miss_i,
	(uint64_t)__ev_tlb_miss_d,
	(uint64_t)__ev_prot_v,
	(uint64_t)__ev_privilege_v,
	(uint64_t)__ev_swi,
	(uint64_t)__ev_trap,
	(uint64_t)__ev_extension,
	(uint64_t)__ev_div_zero,
	0, /* Unused in ARCv3 */
	(uint64_t)__ev_maligned,
	0,
	0
};
#endif

#include <toolchain.h>
#include <linker/sections.h>
#include <sw_isr_table.h>
#include <arch/cpu.h>

struct vector_table {
	uint32_t reset;
	uint32_t memory_error;
	uint32_t instruction_error;
	uint32_t ev_machine_check;
	uint32_t ev_tlb_miss_i;
	uint32_t ev_tlb_miss_d;
	uint32_t ev_prot_v;
	uint32_t ev_privilege_v;
	uint32_t ev_swi;
	uint32_t ev_trap;
	uint32_t ev_extension;
	uint32_t ev_div_zero;
	uint32_t unused_1;
	uint32_t ev_maligned;
	uint32_t unused_2;
	uint32_t unused_3;
	/* interrupts */
	uint32_t isr[IRQ_TABLE_SIZE];
};

struct vector_table _VectorTable Z_GENERIC_SECTION(.exc_vector_table) = {
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	(uint32_t)0x12345678,
	0, /* Unused in ARCv3 */
	(uint32_t)0x12345678,
	0,
	0,
	/* interrupts */
	{(uint32_t)0x12345678}
};

/* The final version of IVDT will be 64 bit, so all this is a temporary hack */
#include <arch/cpu.h>
#include <stdbool.h>

union bcr_di_cache {
	struct {
		unsigned int ver:8, config:4, sz:4, line_len:4, pad:12;
	} fields;
	unsigned int word;
};

#define read_aux_reg z_arc_v2_aux_reg_read
#define write_aux_reg z_arc_v2_aux_reg_write

#define inlined_cachefunc	 inline __attribute__((always_inline))

/* Data cache related auxiliary registers */
#define ARC_AUX_DC_IVDC		0x47
#define ARC_AUX_DC_CTRL		0x48

#define ARC_AUX_IC_IVIC		0x10
#define ARC_AUX_IC_CTRL		0x11
#define ARC_AUX_IC_IVIL		0x19

#define ARC_AUX_DC_FLSH		0x4B

#define ARC_BCR_IC_BUILD	0x77
#define ARC_BCR_DC_BUILD	0x72

/* Bit values in IC_CTRL */
#define IC_CTRL_CACHE_DISABLE	BIT(0)

/* Bit values in DC_CTRL */
#define DC_CTRL_CACHE_DISABLE	BIT(0)
#define DC_CTRL_INV_MODE_FLUSH	BIT(6)
#define DC_CTRL_FLUSH_STATUS	BIT(8)

#define OP_INV			BIT(0)
#define OP_FLUSH		BIT(1)
#define OP_FLUSH_N_INV		(OP_FLUSH | OP_INV)

static inlined_cachefunc bool icache_exists(void)
{
	union bcr_di_cache ibcr;

	ibcr.word = read_aux_reg(ARC_BCR_IC_BUILD);
	return !!ibcr.fields.ver;
}

static inlined_cachefunc bool icache_enabled(void)
{
	if (!icache_exists())
		return false;

	return !(read_aux_reg(ARC_AUX_IC_CTRL) & IC_CTRL_CACHE_DISABLE);
}

static inlined_cachefunc bool dcache_exists(void)
{
	union bcr_di_cache dbcr;

	dbcr.word = read_aux_reg(ARC_BCR_DC_BUILD);
	return !!dbcr.fields.ver;
}

static inlined_cachefunc bool dcache_enabled(void)
{
	if (!dcache_exists())
		return false;

	return !(read_aux_reg(ARC_AUX_DC_CTRL) & DC_CTRL_CACHE_DISABLE);
}

static inlined_cachefunc void __before_dc_op(const int op)
{
	unsigned int ctrl;

	ctrl = read_aux_reg(ARC_AUX_DC_CTRL);

	/* IM bit implies flush-n-inv, instead of vanilla inv */
	if (op == OP_INV)
		ctrl &= ~DC_CTRL_INV_MODE_FLUSH;
	else
		ctrl |= DC_CTRL_INV_MODE_FLUSH;

	write_aux_reg(ARC_AUX_DC_CTRL, ctrl);
}

static inlined_cachefunc void __after_dc_op(const int op)
{
	if (op & OP_FLUSH)	/* flush / flush-n-inv both wait */
		while (read_aux_reg(ARC_AUX_DC_CTRL) & DC_CTRL_FLUSH_STATUS);
}

static inlined_cachefunc void __dc_entire_op(const int cacheop)
{
	int aux;

	if (!dcache_enabled())
		return;

	__before_dc_op(cacheop);

	if (cacheop & OP_INV)	/* Inv or flush-n-inv use same cmd reg */
		aux = ARC_AUX_DC_IVDC;
	else
		aux = ARC_AUX_DC_FLSH;

	write_aux_reg(aux, 0x1);

	__after_dc_op(cacheop);
}

/* IC supports only invalidation */
static inlined_cachefunc void __ic_entire_invalidate(void)
{
	if (!icache_enabled())
		return;

	/* Any write to IC_IVIC register triggers invalidation of entire I$ */
	write_aux_reg(ARC_AUX_IC_IVIC, 1);
	/*
	 * As per ARC HS databook (see chapter 5.3.3.2)
	 * it is required to add 3 NOPs after each write to IC_IVIC.
	 */
	__builtin_arc_nop();
	__builtin_arc_nop();
	__builtin_arc_nop();
	read_aux_reg(ARC_AUX_IC_CTRL);  /* blocks */
}

void sync_n_cleanup_l1_cache_all(void)
{
	__dc_entire_op(OP_FLUSH_N_INV);

	__ic_entire_invalidate();
}

void set_arcv3_vector_hack(void)
{
	_VectorTable.reset = (uint32_t)(uint64_t)__reset;
	_VectorTable.memory_error = (uint32_t)(uint64_t)__memory_error;
	_VectorTable.instruction_error = (uint32_t)(uint64_t)__instruction_error;
	_VectorTable.ev_machine_check = (uint32_t)(uint64_t)__ev_machine_check;
	_VectorTable.ev_tlb_miss_i = (uint32_t)(uint64_t)__ev_tlb_miss_i;
	_VectorTable.ev_tlb_miss_d = (uint32_t)(uint64_t)__ev_tlb_miss_d;
	_VectorTable.ev_prot_v = (uint32_t)(uint64_t)__ev_prot_v;
	_VectorTable.ev_privilege_v = (uint32_t)(uint64_t)__ev_privilege_v;
	_VectorTable.ev_swi = (uint32_t)(uint64_t)__ev_swi;
	_VectorTable.ev_trap = (uint32_t)(uint64_t)__ev_trap;
	_VectorTable.ev_extension = (uint32_t)(uint64_t)__ev_extension;
	_VectorTable.ev_div_zero = (uint32_t)(uint64_t)__ev_div_zero;
	_VectorTable.unused_1 = 0x23456789;
	_VectorTable.ev_maligned = (uint32_t)(uint64_t)__ev_maligned;
	_VectorTable.unused_2 = 0x23456789;
	_VectorTable.unused_3 = 0x23456789;

	_Static_assert(IRQ_TABLE_SIZE > 0, "No Interrupts? Huh.");

	/* interrupts */
	for (int i = 0; i < IRQ_TABLE_SIZE; i++)
		_VectorTable.isr[i] = (uint32_t)(uint64_t)&_isr_wrapper;

	sync_n_cleanup_l1_cache_all();
}

#else

struct vector_table {
	uint32_t reset;
	uint32_t memory_error;
	uint32_t instruction_error;
	uint32_t ev_machine_check;
	uint32_t ev_tlb_miss_i;
	uint32_t ev_tlb_miss_d;
	uint32_t ev_prot_v;
	uint32_t ev_privilege_v;
	uint32_t ev_swi;
	uint32_t ev_trap;
	uint32_t ev_extension;
	uint32_t ev_div_zero;
	uint32_t ev_dc_error;
	uint32_t ev_maligned;
	uint32_t unused_1;
	uint32_t unused_2;
};

struct vector_table _VectorTable Z_GENERIC_SECTION(.exc_vector_table) = {
	(uint32_t)__reset,
	(uint32_t)__memory_error,
	(uint32_t)__instruction_error,
	(uint32_t)__ev_machine_check,
	(uint32_t)__ev_tlb_miss_i,
	(uint32_t)__ev_tlb_miss_d,
	(uint32_t)__ev_prot_v,
	(uint32_t)__ev_privilege_v,
	(uint32_t)__ev_swi,
	(uint32_t)__ev_trap,
	(uint32_t)__ev_extension,
	(uint32_t)__ev_div_zero,
	(uint32_t)__ev_dc_error,
	(uint32_t)__ev_maligned,
	0,
	0
};

#endif
