#include <zephyr.h>
#include <sys/util.h>
#include <sys/util_macro.h>
#include <init.h>
#include <stdbool.h>

#include "mmu-arcv3.h"

#define PAGE_SIZE		KB(4)

#define DDR_START		DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define DDR_SZ			DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

_Static_assert(DDR_SZ <= MB(2), "DDR_SZ");

#ifdef CONFIG_64BIT
// translated area (main memory)
// 0x0000_0000_8000_0000 - 0x0000_0000_8020_0000 (2MiB) - > 512 pages
// translated area (peripherals)
// 0x0000_0000_F000_0000 - 0x0000_0000_F000_1000 (4KiB) - > 1 page
//
// VA = 9 + 9 + 9 + 9 + 12 = 48
//
#define IO_START		0x00000000F0000000UL
#define IO_SZ			PAGE_SIZE
#else
// translated area (main memory)
// 0x8000_0000 - 0x8020_0000 (2MiB) - > 512 pages
// translated area (peripherals)
// 0xF000_0000 - 0xF000_1000 (4KiB) - > 1 page
//
// VA = 2 + 9 + 9 + 12 = 32
//

#define IO_START		0xF0000000UL
#define IO_SZ			PAGE_SIZE
#endif /* CONFIG_64BIT */


#define DDR_END			(DDR_START + DDR_SZ - 1)
#define IO_END			(IO_START + IO_SZ - 1)

#ifdef CONFIG_64BIT
#define VADDR_BITS              48

#define PGD_SHIFT		39	/* LEVEL1 */
#define PUD_SHIFT		30	/* LEVEL2 */
#define PMD_SHIFT		21	/* LEVEL3 */
#define PTE_SHIFT		12	/* LEVEL4 */

#define PUD_ENTR_SZ		BIT(PGD_SHIFT - PUD_SHIFT)
_Static_assert(PUD_ENTR_SZ == 512, "PUD_ENTR_SZ");
#define PMD_ENTR_SZ		BIT(PUD_SHIFT  - PMD_SHIFT)
_Static_assert(PMD_ENTR_SZ == 512, "PMD_ENTR_SZ");

#else
#define VADDR_BITS              32

#define PGD_SHIFT		30	/* LEVEL1 */
#define PMD_SHIFT		21	/* LEVEL2 */
#define PTE_SHIFT		12	/* LEVEL3 */

#define PMD_ENTR_SZ		BIT(PGD_SHIFT  - PMD_SHIFT)
_Static_assert(PMD_ENTR_SZ == 512, "PMD_ENTR_SZ");

#endif /* CONFIG_64BIT */

#define PGD_ENTR_SZ		BIT(VADDR_BITS - PGD_SHIFT)
#define PTE_ENTR_SZ		BIT(PMD_SHIFT  - PTE_SHIFT)
_Static_assert(PTE_ENTR_SZ == 512, "PTE_ENTR_SZ");

/* Bit[0] == 0 indicates a invalid descriptor */
#define DEAD_ENTRY		0
#define VALID_ENTRY		BIT64(0)

/* Bit[1]: 1 indicates a table descriptor */
#define TABLE_ENTRY		BIT64(1)

#define VALID_TABLE_ENTRY	(TABLE_ENTRY | VALID_ENTRY)


#define MEMATTR_NORMAL		0x69
#define MEMATTR_UNCACHED	0x01
#define MEMATTR_VOLATILE	0x00 /* Uncached + No Early Write Ack + Strict Ordering */

#define MEMATTR_IDX_NORMAL	0
#define MEMATTR_IDX_UNCACHED	1
#define MEMATTR_IDX_VOLATILE	2

#define _PAGE_MEMATTR(idx)	((idx) << 2)

#define _PAGE_AP_U_N_K		(1ULL <<  6)  /* 1: User + Kernel, 0: Kernel only) */
#define _PAGE_AP_READONLY	(1ULL <<  7)  /* 1: Read only, 0: Read + Write) */

#define __SHR_OUTER		2ULL
#define __SHR_INNER		3ULL
#define _PAGE_SHARED_OUTER	(__SHR_OUTER <<  8)  /* Outer Shareable */
#define _PAGE_SHARED_INNER	(__SHR_INNER <<  8)  /* Inner Shareable */

#define _PAGE_ACCESSED		(1ULL << 10)  /* software managed, exception if clear */
#define _PAGE_NOTGLOBAL		(1ULL << 11)  /* ASID */

#define _PAGE_DIRTY		(1ULL << 51)  /* software managed */
#define _PAGE_NOTEXEC_K		(1ULL << 53)  /* Execute User */
#define _PAGE_NOTEXEC_U		(1ULL << 54)  /* Execute Kernel */

#define _PAGE_SPECIAL		(1ULL << 55)



#define TABLE12_ATTRS_NONE	0
#define TABLE12_ATTRS_RW	TABLE12_ATTRS_NONE
/* Kernel-execute-never next + User-Execute-never next */
//#define TABLE12_ATTRS_RW	(BIT64(60) | BIT(59))

#define LBLOCK_ATTRS_MAIN	( _PAGE_NOTGLOBAL /* non global, BIT(11) == 1 */				\
				|  _PAGE_ACCESSED								\
				| _PAGE_SHARED_INNER								\
				  /* Access permission, AP: 2’b00 = Read-write access in kernel mode only */	\
				| _PAGE_MEMATTR(MEMATTR_IDX_NORMAL) /* Memory attribute index */		\
				)

#define HBLOCK_ATTRS_MAIN_RWX	( _PAGE_NOTEXEC_U )
#define HBLOCK_ATTRS_MAIN_RW	( _PAGE_NOTEXEC_U | _PAGE_NOTEXEC_K )

#define LBLOCK_ATTRS_IO		( _PAGE_NOTGLOBAL /* non global, BIT(11) == 1 */				\
				| _PAGE_ACCESSED								\
				| _PAGE_SHARED_INNER								\
				  /* Access permission, AP: 2’b00 = Read-write access in kernel mode only */	\
				| _PAGE_MEMATTR(MEMATTR_IDX_VOLATILE) /* Memory attribute index */		\
				)

#define HBLOCK_ATTRS_IO		( _PAGE_NOTEXEC_U | _PAGE_NOTEXEC_K )


#define ASID_VALUE		93ULL	/* Just some random number */
#define ASID_SHIFT		48


_Static_assert(true, "");

/* main memory */
#ifdef CONFIG_64BIT
#define PGD_ENTRIES		1
/* 0x0 in case of 0x0000_0000_8000_0000 */
#define PGD_IDX			((DDR_START & GENMASK((VADDR_BITS - 1), PGD_SHIFT)) >> PGD_SHIFT)
_Static_assert((DDR_END & GENMASK((VADDR_BITS - 1), PGD_SHIFT)) >> PGD_SHIFT == PGD_IDX, "PGD_ENTRIES");

#define PUD_ENTRIES		1
/* 0x2 in case of 0x0000_0000_8000_0000 */
#define PUD_MM_IDX		((DDR_START & GENMASK((PGD_SHIFT - 1), PUD_SHIFT)) >> PUD_SHIFT)
_Static_assert((DDR_END & GENMASK((PGD_SHIFT - 1), PUD_SHIFT)) >> PUD_SHIFT == PUD_MM_IDX, "PUD_ENTRIES");

#define PMD_ENTRIES		1
/* 0x0 in case of 0x0000_0000_8000_0000 */
#define PMD_MM_IDX		((DDR_START & GENMASK((PUD_SHIFT - 1), PMD_SHIFT)) >> PMD_SHIFT)
_Static_assert((DDR_END & GENMASK((PUD_SHIFT - 1), PMD_SHIFT)) >> PMD_SHIFT == PMD_MM_IDX, "PMD_ENTRIES");
#else
#define PGD_ENTRIES		1
#define PGD_IDX			((DDR_START & GENMASK((VADDR_BITS - 1), PGD_SHIFT)) >> PGD_SHIFT)
_Static_assert((DDR_END & GENMASK((VADDR_BITS - 1), PGD_SHIFT)) >> PGD_SHIFT == PGD_IDX, "PGD_ENTRIES");

#define PMD_ENTRIES		1
#define PMD_MM_IDX		((DDR_START & GENMASK((PGD_SHIFT - 1), PMD_SHIFT)) >> PMD_SHIFT)
_Static_assert((DDR_END & GENMASK((PGD_SHIFT - 1), PMD_SHIFT)) >> PMD_SHIFT == PMD_MM_IDX, "PMD_ENTRIES");
#endif /* CONFIG_64BIT */


/* peripherals */
#ifdef CONFIG_64BIT
_Static_assert((IO_START & GENMASK((VADDR_BITS - 1), PGD_SHIFT)) >> PGD_SHIFT == PGD_IDX, "PGD_ENTRIES");
_Static_assert((IO_END & GENMASK((VADDR_BITS - 1), PGD_SHIFT)) >> PGD_SHIFT == PGD_IDX, "PGD_ENTRIES");

/* 0x3 in case of 0xF000_0000 */
#define PUD_IO_IDX		((IO_START & GENMASK((PGD_SHIFT - 1), PUD_SHIFT)) >> PUD_SHIFT)
_Static_assert((IO_END & GENMASK((PGD_SHIFT - 1), PUD_SHIFT)) >> PUD_SHIFT == PUD_IO_IDX, "PUD_ENTRIES");

/* 0x180 in case of 0xF000_0000 */
#define PMD_IO_IDX		((IO_START & GENMASK((PUD_SHIFT - 1), PMD_SHIFT)) >> PMD_SHIFT)
_Static_assert((IO_END & GENMASK((PUD_SHIFT - 1), PMD_SHIFT)) >> PMD_SHIFT == PMD_IO_IDX, "PMD_ENTRIES");
#else
/* 0x3 in case of 0xF000_0000 */
#define PGD_IO_IDX		((IO_START & GENMASK((VADDR_BITS - 1), PGD_SHIFT)) >> PGD_SHIFT)
_Static_assert((IO_END & GENMASK((VADDR_BITS - 1), PGD_SHIFT)) >> PGD_SHIFT == PGD_IO_IDX, "PUD_ENTRIES");

/* 0x180 in case of 0xF000_0000 */
#define PMD_IO_IDX		((IO_START & GENMASK((PGD_SHIFT - 1), PMD_SHIFT)) >> PMD_SHIFT)
_Static_assert((IO_END & GENMASK((PGD_SHIFT - 1), PMD_SHIFT)) >> PMD_SHIFT == PMD_IO_IDX, "PMD_ENTRIES");
#endif /* CONFIG_64BIT */

/* 0x0 in case of 0xF000_0000 */
#define PTE_IO_IDX		((IO_START & GENMASK((PMD_SHIFT -1), PTE_SHIFT)) >> PTE_SHIFT)
_Static_assert((IO_END & GENMASK((PMD_SHIFT -1), PTE_SHIFT)) >> PTE_SHIFT == PTE_IO_IDX, "LEVEL4_ENTRIES");


/* always have one entry at 0 index for 32bit adresses space */
uint64_t __aligned(PAGE_SIZE) pgd_table [PGD_ENTR_SZ];

#ifdef CONFIG_64BIT
/* only 30 and 31 bit can be different which gives us 4 entries max for 32bit address space */
uint64_t __aligned(PAGE_SIZE) pud_table [PUD_ENTR_SZ];
#endif /* CONFIG_64BIT */

uint64_t __aligned(PAGE_SIZE) pmd_table_mm [PMD_ENTR_SZ];
uint64_t __aligned(PAGE_SIZE) pmd_table_io [PMD_ENTR_SZ];

uint64_t __aligned(PAGE_SIZE) pte_table_mm [PTE_ENTR_SZ];
uint64_t __aligned(PAGE_SIZE) pte_table_io [PTE_ENTR_SZ];


static void k_page_table_init_main(void);
static void k_page_table_init_io(void);
static void k_mmu_init_enable(void);
static void k_mmu_sanitycheck_prepare(void);

static void k_page_table_init(void)
{
	k_page_table_init_main();
	k_page_table_init_io();
}

static uint64_t ptr_to_uint64_t(void *ptr)
{
#ifdef CONFIG_64BIT
	return (uint64_t)ptr;
#else
	return (uint64_t)((uint32_t)ptr);
#endif /* CONFIG_64BIT */

}

static void z_arc_aux_reg_write_64_nonatomic(unsigned int reg, uint64_t value)
{
#ifdef CONFIG_64BIT
	z_arc_aux_reg_write_64(reg, value);
#else
	z_arc_v2_aux_reg_write(reg, (uint32_t)value);
	z_arc_v2_aux_reg_write((reg + 1), (uint32_t)(value >> 32));
#endif /* CONFIG_64BIT */
}

static bool z_is_text_address(uint64_t page_start)
{
	extern char __text_region_start[];
	extern char __text_region_end[];

	uintptr_t text_region_start = (uintptr_t)__text_region_start;
	uintptr_t text_region_end = (uintptr_t)__text_region_end;

	uint64_t page_end = page_start + PAGE_SIZE - 1;

	return text_region_start <= page_end && page_start <= text_region_end;
}

// translated area (main memory)
static void k_page_table_init_main(void)
{
	uint64_t entry;

	/* level 1 (pgd) init */
	for (int i = 0; i < PGD_ENTR_SZ; i++) {
		pgd_table[i] = DEAD_ENTRY;
	}

#ifdef CONFIG_64BIT
	entry = ptr_to_uint64_t(&pud_table) | TABLE12_ATTRS_NONE | VALID_TABLE_ENTRY;
	pgd_table[PGD_IDX] = entry;


	/* level 2 (pud) init */
	for (int i = 0; i < PUD_ENTR_SZ; i++) {
		pud_table[i] = DEAD_ENTRY;
	}

	entry = ptr_to_uint64_t(&pmd_table_mm) | TABLE12_ATTRS_NONE | VALID_TABLE_ENTRY;
	pud_table[PUD_MM_IDX] = entry;
#else

	entry = ptr_to_uint64_t(&pmd_table_mm) | TABLE12_ATTRS_NONE | VALID_TABLE_ENTRY;
	pgd_table[PGD_IDX] = entry;
#endif /* CONFIG_64BIT */

	for (int i = 0; i < PMD_ENTR_SZ; i++) {
		pmd_table_mm[i] = DEAD_ENTRY;
	}

	//TODO: table attributes
	entry = ptr_to_uint64_t(&pte_table_mm) | VALID_TABLE_ENTRY;
	pmd_table_mm[PMD_MM_IDX] = entry;


	/* last level (pte) init - pages */
	for (int i = 0; i < PTE_ENTR_SZ; i++) {
		uint64_t paddr = DDR_START + PAGE_SIZE * i;

		if (paddr <= DDR_END) {
			if (z_is_text_address(paddr)) {
				entry = HBLOCK_ATTRS_MAIN_RWX;
			} else {
				entry = HBLOCK_ATTRS_MAIN_RW;
			}
			/* Bits[(VADDR_BITS - 1):12] of the 48-bit 4KB aligned physical address */
			paddr &= GENMASK((VADDR_BITS - 1), PTE_SHIFT);
			entry = paddr | LBLOCK_ATTRS_MAIN | VALID_TABLE_ENTRY;
		} else {
			entry = DEAD_ENTRY;
		}

		pte_table_mm[i] = entry;
	}
}

// translated area (peripherals)
// 0x __ F000_0000 - 0x __ F000_1000 (4KiB) - > 1 page
static void k_page_table_init_io(void)
{
	uint64_t entry;

	entry = ptr_to_uint64_t(&pmd_table_io) | TABLE12_ATTRS_RW | VALID_TABLE_ENTRY;

#ifdef CONFIG_64BIT
	pud_table[PUD_IO_IDX] = entry;
#else
	pgd_table[PGD_IO_IDX] = entry;
#endif /* CONFIG_64BIT */

	for (int i = 0; i < PMD_ENTR_SZ; i++) {
		pmd_table_io[i] = DEAD_ENTRY;
	}

	entry = ptr_to_uint64_t(&pte_table_io) | TABLE12_ATTRS_RW | VALID_TABLE_ENTRY;
	pmd_table_io[PMD_IO_IDX] = entry;


	/* last level (pte) init */
	for (int i = 0; i < PTE_ENTR_SZ; i++) {
		pte_table_io[i] = DEAD_ENTRY;
	}

	uint64_t paddr = IO_START;
	paddr &= GENMASK((VADDR_BITS - 1), PTE_SHIFT);
	entry = HBLOCK_ATTRS_IO | paddr | LBLOCK_ATTRS_IO | VALID_TABLE_ENTRY;
	pte_table_io[PTE_IO_IDX] = entry;
}

static void k_mmu_init_enable(void)
{
	union {
		uint32_t word;
		struct {
			uint32_t t0sz:5, t0sh:2, t0c:1, res0:7, a1:1,
			    t1sz:5, t1sh:2, t1c:1, res1:8;
		};
	} ttbc;

	union {
		uint64_t word;
		struct {
			uint8_t attr[8];
		};
	} memattr;

#ifdef CONFIG_64BIT
	ttbc.t0sz = 16;
	ttbc.t1sz = 16;	/* Not relevant since kernel linked under 4GB hits T0SZ */
#else
	/* All virtual address to RTP0 region */
	ttbc.t0sz = 0;
	ttbc.t1sz = 0;
#endif /* CONFIG_64BIT */
	ttbc.t0sh = __SHR_INNER;
	ttbc.t1sh = __SHR_INNER;
	ttbc.t0c = 1; /* cached */
	ttbc.t1c = 1; /* cached */
	ttbc.a1 = 0;  /* ASID used is from MMU_RTP0 */

	z_arc_v2_aux_reg_write(ARC_REG_MMU_TTBC, ttbc.word);

	memattr.attr[MEMATTR_IDX_NORMAL] = MEMATTR_NORMAL;
	memattr.attr[MEMATTR_IDX_UNCACHED] = MEMATTR_UNCACHED;
	memattr.attr[MEMATTR_IDX_VOLATILE] = MEMATTR_VOLATILE;

	z_arc_aux_reg_write_64_nonatomic(ARC_REG_MMU_MEM_ATTR, memattr.word);

	uint64_t rtp0_val = ptr_to_uint64_t(&pgd_table) | (ASID_VALUE << ASID_SHIFT);

	z_arc_aux_reg_write_64_nonatomic(ARC_REG_MMU_RTP0, rtp0_val);
	z_arc_aux_reg_write_64_nonatomic(ARC_REG_MMU_RTP1, 0);	/* to catch bugs */

	uint32_t ctrl = ARC_REG_MMU_CTRL_EN | ARC_REG_MMU_CTRL_KU | ARC_REG_MMU_CTRL_WX;
	z_arc_v2_aux_reg_write(ARC_REG_MMU_CTRL, ctrl);

}

#define DO_MMU_SANITYCHECK_TEST			0

/* basic test of MMU - check that we can access where expected an can't access where not expected */
#if DO_MMU_SANITYCHECK_TEST == 1
static volatile uint64_t cannary = 0xDEADBEEFDEADBEEF;

static void k_mmu_sanitycheck_prepare(void)
{
	cannary = 0x12345678UL;
}

static void __attribute__((optimize("-O0"))) k_mmu_sanitycheck(void)
{
	uint64_t volatile value;

	value = *(uint64_t *)(DDR_START);
	value = *(uint64_t *)(DDR_START + DDR_SZ - 8);

	if (cannary != 0x12345678UL) {
		while (true) {}
	}

	cannary = 0xDEADBEEFDEADBEEF;

#if 0
	// We should get fault here
	value = *(uint64_t *)(DDR_START + DDR_SZ + 0x8);
	*(uint64_t *)(DDR_START + DDR_SZ + 0x10) = value;
#endif
}
#else
#define	k_mmu_sanitycheck_prepare()
#define k_mmu_sanitycheck()
#endif /* DO_MMU_SANITYCHECK_TEST == 1 */

static int k_mmu_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	k_mmu_sanitycheck_prepare();
	k_page_table_init();
	k_mmu_init_enable();
	k_mmu_sanitycheck();

	return 0;
}

SYS_INIT(k_mmu_init, PRE_KERNEL_1, 1);
