#include <zephyr.h>
#include <sys/util.h>
#include <sys/util_macro.h>
#include <init.h>

#include "mmu-arcv3.h"

// translated area (main memory)
// 0x0000_0000_8000_0000 - 0x0000_0000_0020_0000 (2MiB) - > 512 pages
// translated area (peripherals)
// 0x0000_0000_F000_0000 - 0x0000_0000_F000_1000 (4KiB) - > 1 page
//
// VA = 9 + 9 + 9 + 9 + 12
//
#define PAGE_SIZE		KB(4)


#define DDR_START		0x0000000080000000UL
#define DDR_SZ			MB(2)

#define IO_START		0x00000000F0000000UL
#define IO_SZ			PAGE_SIZE


#define DDR_END			(DDR_START + DDR_SZ - 1)
#define IO_END			(IO_START + IO_SZ - 1)


#define LEVEL1_SHIFT		39
#define LEVEL2_SHIFT		30
#define LEVEL3_SHIFT		21
#define LEVEL4_SHIFT		12

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

#define _PAGE_AP_U_N_K		(1UL <<  6)  /* 1: User + Kernel, 0: Kernel only) */
#define _PAGE_AP_READONLY	(1UL <<  7)  /* 1: Read only, 0: Read + Write) */

#define __SHR_OUTER		2UL
#define __SHR_INNER		3UL
#define _PAGE_SHARED_OUTER	(__SHR_OUTER <<  8)  /* Outer Shareable */
#define _PAGE_SHARED_INNER	(__SHR_INNER <<  8)  /* Inner Shareable */

#define _PAGE_ACCESSED		(1UL << 10)  /* software managed, exception if clear */
#define _PAGE_NOTGLOBAL		(1UL << 11)  /* ASID */

#define _PAGE_DIRTY		(1UL << 51)  /* software managed */
#define _PAGE_NOTEXEC_K		(1UL << 53)  /* Execute User */
#define _PAGE_NOTEXEC_U		(1UL << 54)  /* Execute Kernel */

#define _PAGE_SPECIAL		(1UL << 55)



#define TABLE12_ATTRS_NONE	0
#define TABLE12_ATTRS_RW	TABLE12_ATTRS_NONE
/* Kernel-execute-never next + User-Execute-never next */
//#define TABLE12_ATTRS_RW	(BIT64(60) | BIT(59))

#define LBLOCK_ATTRS_MAIN	( /* global, BIT(11) == 0 */							\
				  _PAGE_ACCESSED								\
				| _PAGE_SHARED_INNER								\
				  /* Access permission, AP: 2’b00 = Read-write access in kernel mode only */	\
				| _PAGE_MEMATTR(MEMATTR_IDX_NORMAL) /* Memory attribute index */		\
				)

#define HBLOCK_ATTRS_MAIN	( _PAGE_NOTEXEC_U ) /* TODO: DBM */

#define LBLOCK_ATTRS_IO		( /* global, BIT(11) == 0 */							\
				  _PAGE_ACCESSED								\
				| _PAGE_SHARED_INNER								\
				  /* Access permission, AP: 2’b00 = Read-write access in kernel mode only */	\
				| _PAGE_MEMATTR(MEMATTR_IDX_VOLATILE) /* Memory attribute index */		\
				)

#define HBLOCK_ATTRS_IO		( _PAGE_NOTEXEC_U | _PAGE_NOTEXEC_K ) /* TODO: DBM */


#define TABEL_SZ_9BIT 		512

_Static_assert(true, "");

/* main memory */
#define LEVEL1_ENTRIES		1
#define LEVEL1_IDX		0x0
_Static_assert((DDR_START & GENMASK(47, LEVEL1_SHIFT)) >> LEVEL1_SHIFT == LEVEL1_IDX, "LEVEL1_ENTRIES");
_Static_assert((DDR_END & GENMASK(47, LEVEL1_SHIFT)) >> LEVEL1_SHIFT == LEVEL1_IDX, "LEVEL1_ENTRIES");

#define LEVEL2_ENTRIES		1
#define LEVEL2_MM_IDX		0x2
_Static_assert((DDR_START & GENMASK(38, LEVEL2_SHIFT)) >> LEVEL2_SHIFT == LEVEL2_MM_IDX, "LEVEL2_ENTRIES");
_Static_assert((DDR_END & GENMASK(38, LEVEL2_SHIFT)) >> LEVEL2_SHIFT == LEVEL2_MM_IDX, "LEVEL2_ENTRIES");

#define LEVEL3_ENTRIES		1
#define LEVEL3_MM_IDX		0x0
_Static_assert((DDR_START & GENMASK(29, LEVEL3_SHIFT)) >> LEVEL3_SHIFT == LEVEL3_MM_IDX, "LEVEL3_ENTRIES");
_Static_assert((DDR_END & GENMASK(29, LEVEL3_SHIFT)) >> LEVEL3_SHIFT == LEVEL3_MM_IDX, "LEVEL3_ENTRIES");

/* peripherals */
_Static_assert((IO_START & GENMASK(47, LEVEL1_SHIFT)) >> LEVEL1_SHIFT == LEVEL1_IDX, "LEVEL1_ENTRIES");
_Static_assert((IO_END & GENMASK(47, LEVEL1_SHIFT)) >> LEVEL1_SHIFT == LEVEL1_IDX, "LEVEL1_ENTRIES");

#define LEVEL2_IO_IDX		0x3
_Static_assert((IO_START & GENMASK(38, LEVEL2_SHIFT)) >> LEVEL2_SHIFT == LEVEL2_IO_IDX, "LEVEL2_ENTRIES");
_Static_assert((IO_END & GENMASK(38, LEVEL2_SHIFT)) >> LEVEL2_SHIFT == LEVEL2_IO_IDX, "LEVEL2_ENTRIES");

#define LEVEL3_IO_IDX		0x180
_Static_assert((IO_START & GENMASK(29, LEVEL3_SHIFT)) >> LEVEL3_SHIFT == LEVEL3_IO_IDX, "LEVEL3_ENTRIES");
_Static_assert((IO_END & GENMASK(29, LEVEL3_SHIFT)) >> LEVEL3_SHIFT == LEVEL3_IO_IDX, "LEVEL3_ENTRIES");

#define LEVEL4_IO_IDX		0x0
_Static_assert((IO_START & GENMASK(20, LEVEL4_SHIFT)) >> LEVEL4_SHIFT == LEVEL4_IO_IDX, "LEVEL4_ENTRIES");
_Static_assert((IO_END & GENMASK(20, LEVEL4_SHIFT)) >> LEVEL4_SHIFT == LEVEL4_IO_IDX, "LEVEL4_ENTRIES");


/* always have one entry at 0 index for 32bit adresses space */
uint64_t __aligned(PAGE_SIZE) level_tabel_1 [TABEL_SZ_9BIT];
/* only 30 and 31 bit can be different which gives us 4 entries max for 32bit
 * adresses space */
uint64_t __aligned(PAGE_SIZE) level_tabel_2 [TABEL_SZ_9BIT];

uint64_t __aligned(PAGE_SIZE) level_tabel_3_mm [TABEL_SZ_9BIT];
uint64_t __aligned(PAGE_SIZE) level_tabel_3_io [TABEL_SZ_9BIT];

uint64_t __aligned(PAGE_SIZE) level_tabel_4_mm [TABEL_SZ_9BIT];
uint64_t __aligned(PAGE_SIZE) level_tabel_4_io [TABEL_SZ_9BIT];


static void k_page_table_init_main(void);
static void k_page_table_init_io(void);
static void k_mmu_init_enable(void);
static void k_mmu_sanitycheck_prepare(void);
void flush_l1_dcache_all(void);

static void k_page_table_init(void)
{
	k_page_table_init_main();
	k_page_table_init_io();

	flush_l1_dcache_all();
}

// translated area (main memory)
// 0x0000_0000_8000_0000 - 0x0000_0000_0020_0000 (2MiB) - > 512 pages
static void k_page_table_init_main(void)
{
	uint64_t entry;

	/* level 1 init */
	for (int i = 0; i < TABEL_SZ_9BIT; i++) {
		level_tabel_1[i] = DEAD_ENTRY;
	}

	entry = (uint64_t)&level_tabel_2 | TABLE12_ATTRS_NONE | VALID_TABLE_ENTRY;
	level_tabel_1[LEVEL1_IDX] = entry;


	/* level 2 init */
	for (int i = 0; i < TABEL_SZ_9BIT; i++) {
		level_tabel_2[i] = DEAD_ENTRY;
	}

	entry = (uint64_t)&level_tabel_3_mm | TABLE12_ATTRS_NONE | VALID_TABLE_ENTRY;
	level_tabel_2[LEVEL2_MM_IDX] = entry;


	/* level 3 init */
	for (int i = 0; i < TABEL_SZ_9BIT; i++) {
		level_tabel_3_mm[i] = DEAD_ENTRY;
	}

	//TODO: table attributes
	entry = (uint64_t)&level_tabel_4_mm | VALID_TABLE_ENTRY;
	level_tabel_3_mm[LEVEL3_MM_IDX] = entry;


	/* level 4 init - pages */
	for (int i = 0; i < TABEL_SZ_9BIT; i++) {
		uint64_t paddr = DDR_START + PAGE_SIZE * i;
		/* Bits[47:12] of the 48-bit 4KB aligned physical address */
		paddr &= GENMASK(47, 12);
		entry = HBLOCK_ATTRS_MAIN | paddr | LBLOCK_ATTRS_MAIN | VALID_TABLE_ENTRY;
		level_tabel_4_mm[i] = entry;
	}
}

// translated area (peripherals)
// 0x0000_0000_F000_0000 - 0x0000_0000_F000_1000 (4KiB) - > 1 page
static void k_page_table_init_io(void)
{
	uint64_t entry;

	entry = (uint64_t)&level_tabel_3_io | TABLE12_ATTRS_RW | VALID_TABLE_ENTRY;
	level_tabel_2[LEVEL2_IO_IDX] = entry;


	/* level 3 init */
	for (int i = 0; i < TABEL_SZ_9BIT; i++) {
		level_tabel_3_io[i] = DEAD_ENTRY;
	}

	entry = (uint64_t)&level_tabel_4_io | TABLE12_ATTRS_RW | VALID_TABLE_ENTRY;
	level_tabel_3_io[LEVEL3_IO_IDX] = entry;


	/* level 4 init */
	for (int i = 0; i < TABEL_SZ_9BIT; i++) {
		level_tabel_4_io[i] = DEAD_ENTRY;
	}

	uint64_t paddr = IO_START;
	paddr &= GENMASK(47, 12);
	entry = HBLOCK_ATTRS_IO | paddr | LBLOCK_ATTRS_IO | VALID_TABLE_ENTRY;
	level_tabel_4_io[LEVEL4_IO_IDX] = entry;
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

	ttbc.t0sz = 16;
	ttbc.t1sz = 16;	/* Not relevant since kernel linked under 4GB hits T0SZ */
	ttbc.t0sh = __SHR_INNER;
	ttbc.t1sh = __SHR_INNER;
	ttbc.t0c = 0; /* uncached */
	ttbc.t1c = 0; /* uncached */
//	ttbc.t0c = 1; /* cached */
//	ttbc.t1c = 1; /* cached */
	ttbc.a1 = 0;  /* ASID used is from MMU_RTP0 */

	z_arc_v2_aux_reg_write(ARC_REG_MMU_TTBC, ttbc.word);

	memattr.attr[MEMATTR_IDX_NORMAL] = MEMATTR_NORMAL;
	memattr.attr[MEMATTR_IDX_UNCACHED] = MEMATTR_UNCACHED;
	memattr.attr[MEMATTR_IDX_VOLATILE] = MEMATTR_VOLATILE;

	z_arc_aux_reg_write_64(ARC_REG_MMU_MEM_ATTR, memattr.word);

	z_arc_aux_reg_write_64(ARC_REG_MMU_RTP0, (uint64_t)&level_tabel_1);
	z_arc_aux_reg_write_64(ARC_REG_MMU_RTP1, 0);	/* to catch bugs */

	z_arc_v2_aux_reg_write(ARC_REG_MMU_CTRL, 0x7);

}

static volatile uint64_t cannary = 0xDEADBEEFDEADBEEF;

static void k_mmu_sanitycheck_prepare(void)
{
	cannary = 0x12345678UL;
}

static void __attribute__((optimize("-O0"))) k_mmu_sanitycheck(void)
{
	uint64_t value;

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

static int k_mmu_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	k_mmu_sanitycheck_prepare();
	k_page_table_init();
	k_mmu_init_enable();
	k_mmu_sanitycheck();

	return 0;
}

SYS_INIT(k_mmu_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
