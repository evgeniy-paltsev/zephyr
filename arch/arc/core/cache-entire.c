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

void flush_l1_dcache_all(void)
{
	__dc_entire_op(OP_FLUSH);
}
