#define COCSEP_CLASSES        3393U
#define COCSEP_TABLESIZE      (_3p7 << 7U)
#define COCSEP_VISITEDSIZE    ((COCSEP_TABLESIZE + 7U) / 8U)
#define COCSEP_INFOSIZE       12U
#define COCSEP_FULLSIZE       (4 * (COCSEP_TABLESIZE + COCSEP_INFOSIZE))

#define ESEP_MAX(h)           ((COCSEP_CLASSES * _12c4 * _8c4) << (h))
#define ESEP_TABLESIZE(h, k)  (ESEP_MAX((h)) >> (k))
#define ESEP_INFOSIZE         25 /* TODO unknown yet */

#define H48_ESIZE(h)          ((_12c4 * _8c4) << (h))

#define _esep_ind(i)          (i / 8U)
#define _esep_shift(i)        (4U * (i % 8U))
#define _esep_mask(i)         (((1U << 4U) - 1U) << _esep_shift(i))
#define _visited_ind(i)       (i / 8U)
#define _visited_mask(i)      (1U << (i % 8U))

typedef struct {
	cube_fast_t cube;
	uint8_t depth;
	uint8_t maxdepth;
	uint16_t *n;
	uint32_t *buf32;
	uint8_t *visited;
	uint64_t *selfsim;
	cube_fast_t *rep;
} dfsarg_cocsep_t;

typedef struct {
	uint8_t depth;
	uint8_t h;
	uint32_t *cocsepdata;
	uint32_t *buf32;
	uint64_t *selfsim;
	cube_fast_t *crep;
} bfsarg_esep_t;

_static_inline int64_t coord_h48(cube_fast_t, const uint32_t *, uint8_t);
_static_inline cube_fast_t invcoord_h48(int64_t, const cube_fast_t *, uint8_t);

_static size_t gendata_cocsep(void *, uint64_t *, cube_fast_t *);
_static uint32_t gendata_cocsep_dfs(dfsarg_cocsep_t *);
_static size_t gendata_h48(void *, uint8_t);
_static uint64_t gendata_esep_bfs(bfsarg_esep_t *);

_static_inline bool get_visited(const uint8_t *, int64_t);
_static_inline void set_visited(uint8_t *, int64_t);
_static_inline uint8_t get_esep_pval(const uint32_t *, int64_t);
_static_inline void set_esep_pval(uint32_t *, int64_t, uint8_t);

_static_inline int64_t
coord_h48(cube_fast_t c, const uint32_t *cocsepdata, uint8_t h)
{
	cube_fast_t d;
	int64_t cocsep, coclass, esep, eo, ret;
	uint32_t data;
	uint8_t ttrep;

	DBG_ASSERT(h <= 11, -1, "coord_h48: h must be between 0 and 11\n");

	cocsep = coord_fast_cocsep(c);
	data = cocsepdata[cocsep];
	coclass = (data & (0xFFFFU << 16U)) >> 16U;
	ttrep = (data & (0xFFU << 8U)) >> 8U;

	d = transform(c, ttrep); /* TODO: transform only edges */
	esep = coord_fast_esep(d);
	eo = coord_fast_eo(d);

	ret = (coclass * H48_ESIZE(h)) + (esep << h) + (eo >> (11-h));

	return ret;
}

/*

This function does not necessarily return a cube whose coordinate is
the given value, because it works up to symmetry. This means that the
returned cube is a transformed cube of one that gives the correct value.
*/
_static_inline cube_fast_t
invcoord_h48(int64_t i, const cube_fast_t *crep, uint8_t h) {
	cube_fast_t ret; int64_t coclass, ee, esep, eo;

	DBG_ASSERT(h <= 11, cubetofast(zero),
	    "invcoord_h48: h must be between 0 and 11\n");

	coclass = i / H48_ESIZE(h);
	ee = i % H48_ESIZE(h);
	esep = ee >> h;
	eo = (ee & ((1<<h)-1)) << (11-h);

	ret = invcoord_fast_esep(esep);
	copy_corners_fast(&ret, crep[coclass]);
	set_eo_fast(&ret, eo);

	return ret;
}

/*
Each element of the cocsep table is a uint32_t used as follows:
  - Lowest 8-bit block: pruning value
  - Second-lower 8-bit block: "ttrep" (transformation to representative)
  - Top 16-bit block: symcoord value
After the data as described above, more auxiliary information is appended:
  - A uint32_t representing the number of symmetry classes
  - A uint32_t representing the highest value of the pruning table
  - One uint32_t for each "line" of the pruning table, representing the number
    of positions having that pruning value.
*/
_static size_t
gendata_cocsep(void *buf, uint64_t *selfsim, cube_fast_t *rep)
{
	uint32_t *buf32, *info, cc;
	uint16_t n;
	uint8_t i, j, visited[COCSEP_VISITEDSIZE];
	dfsarg_cocsep_t arg;

	buf32 = (uint32_t *)buf;
	info = buf32 + COCSEP_TABLESIZE;
	memset(buf32, 0xFFU, sizeof(uint32_t) * COCSEP_TABLESIZE);
	memset(info, 0, sizeof(uint32_t) * COCSEP_INFOSIZE);
	memset(selfsim, 0, sizeof(uint64_t) * COCSEP_CLASSES);

	arg = (dfsarg_cocsep_t) {
		.cube = cubetofast(solvedcube()),
		.n = &n,
		.buf32 = buf32,
		.visited = visited,
		.selfsim = selfsim,
		.rep = rep
	};
	for (i = 0, n = 0, cc = 0; i < 10; i++) {
		DBG_LOG("cocsep: generating depth %" PRIu8 "\n", i);
		memset(visited, 0, COCSEP_VISITEDSIZE);
		arg.depth = 0;
		arg.maxdepth = i;
		cc = gendata_cocsep_dfs(&arg);
		info[i+2] = cc;
		DBG_LOG("found %" PRIu32 "\n", cc);
	}

	info[0] = (uint32_t)n;
	info[1] = 9U; /* Known max pruning value */
	DBG_ASSERT(n == COCSEP_CLASSES, 0,
	    "cocsep: computed %" PRIu16 " symmetry classes, "
	    "expected %" PRIu16 "\n", n, COCSEP_CLASSES);

	DBG_LOG("cocsep data computed\n");
	DBG_LOG("Symmetry classes: %" PRIu32 "\n", info[0]);
	DBG_LOG("Maximum pruning value: %" PRIu32 "\n", info[1]);
	DBG_LOG("Pruning value distribution:\n");
	for (j = 0; j < 10; j++)
		DBG_LOG("%" PRIu8 ":\t%" PRIu32 "\n", j, info[j+2]);

	return COCSEP_FULLSIZE;
}

_static uint32_t
gendata_cocsep_dfs(dfsarg_cocsep_t *arg)
{
	uint8_t m, t, tinv, olddepth;
	uint32_t cc;
	int64_t i, ii;
	uint64_t sim;
	cube_fast_t d;
	dfsarg_cocsep_t nextarg;

	i = coord_fast_cocsep(arg->cube);
	olddepth = (uint8_t)(arg->buf32[i] & 0xFFU);
	if (olddepth < arg->depth || get_visited(arg->visited, i))
		return 0;
	set_visited(arg->visited, i);

	if (arg->depth == arg->maxdepth) {
		if ((arg->buf32[i] & 0xFFU) != 0xFFU)
			return 0;

		for (t = 0, cc = 0; t < 48; t++) {
			d = transform(arg->cube, t);
			ii = coord_fast_cocsep(d);
			arg->selfsim[*arg->n] |= (i == ii) << t;
			set_visited(arg->visited, ii);
			tinv = inverse_trans(t);
			cc += (arg->buf32[ii] & 0xFFU) == 0xFFU;
			arg->buf32[ii] =
			    (*arg->n << 16U) | (tinv << 8U) | arg->depth;
		}
		arg->rep[*arg->n] = arg->cube;
		(*arg->n)++;

		return cc;
	}

	memcpy(&nextarg, arg, sizeof(dfsarg_cocsep_t));
	nextarg.depth++;
	for (m = 0, cc = 0; m < 18; m++) {
		nextarg.cube = move(arg->cube, m);
		cc += gendata_cocsep_dfs(&nextarg);
	}

	return cc;
}

/*
TODO description
generating fixed table with h=0, k=4
*/
_static size_t
gendata_h48(void *buf, uint8_t h)
{
	const int k = 1; /* TODO: other cases? */
	uint32_t *buf32, *info, *cocsepdata;
	bfsarg_esep_t arg;
	int64_t sc, cc, tot;
	uint64_t selfsim[COCSEP_CLASSES];
	cube_fast_t crep[COCSEP_CLASSES];
	size_t cocsepsize;

	cocsepsize = gendata_cocsep(buf, selfsim, crep);
	DBG_ASSERT(cocsepsize == COCSEP_FULLSIZE, 0,
	    "gendata_h48: error computing cocsep data\n");

	cocsepdata = (uint32_t *)buf;
	buf32 = cocsepdata + cocsepsize;
	info = buf32 + ESEP_TABLESIZE(h, k);
	memset(buf32, 0xFFU, sizeof(uint32_t) * ESEP_TABLESIZE(h, k));
	memset(info, 0, sizeof(uint32_t) * ESEP_INFOSIZE);

	sc = coord_h48(cubetofast(solved), cocsepdata, h);
	set_esep_pval(buf32, sc, 0);
	arg = (bfsarg_esep_t) {
		.h = h,
		.cocsepdata = cocsepdata,
		.buf32 = buf32,
		.crep = crep,
		.selfsim = selfsim
	};
	for (tot = 1, arg.depth = 1, cc = 0; tot < ESEP_MAX(h); arg.depth++) {
		DBG_LOG("esep: generating depth %" PRIu8 "\n", arg.depth);
		cc = gendata_esep_bfs(&arg);
		tot += cc;
		info[arg.depth+1] = cc;
		DBG_LOG("found %" PRIu64 "\n", cc);
	}

	return COCSEP_FULLSIZE + cc;
}

_static uint64_t
gendata_esep_bfs(bfsarg_esep_t *arg)
{
	uint8_t c, m, t, x;
	uint32_t cc;
	uint64_t i, j, k, cocsep_coord, sim;
	cube_fast_t cube, moved, transd;

	for (i = 0, cc = 0; i < ESEP_MAX(arg->h); i++) {
		c = get_esep_pval(arg->buf32, i);
		if (c != arg->depth - 1)
			continue;
		cube = invcoord_h48(i, arg->crep, arg->h);
		for (m = 0; m < 18; m++) {
			moved = move(cube, m);
			j = coord_h48(moved, arg->cocsepdata, arg->h);
			x = get_esep_pval(arg->buf32, j);
			if (x <= arg->depth)
				continue;
			set_esep_pval(arg->buf32, j, arg->depth);
			cc += x != arg->depth;
			cocsep_coord = j / H48_ESIZE(arg->h);
			sim = arg->selfsim[cocsep_coord];
			for (t = 1; t < 48; t++) { /* Skip trivial trans */
				if (!(sim & (1 << t)))
					continue;
				transd = transform(moved, t);
				k = coord_h48(transd, arg->cocsepdata, arg->h);
				x = get_esep_pval(arg->buf32, k);
				if (x <= arg->depth)
					continue;
				set_esep_pval(arg->buf32, k, arg->depth);
				cc += x != arg->depth;
			}
		}
	}

	return cc;
}

_static_inline bool get_visited(const uint8_t *a, int64_t i)
{
	return a[_visited_ind(i)] & _visited_mask(i);
}

_static_inline void set_visited(uint8_t *a, int64_t i)
{
	a[_visited_ind(i)] |= _visited_mask(i);
}

_static_inline uint8_t
get_esep_pval(const uint32_t *buf32, int64_t i)
{
	return (buf32[_esep_ind(i)] & _esep_mask(i)) >> _esep_shift(i);
}

_static_inline void
set_esep_pval(uint32_t *buf32, int64_t i, uint8_t val)
{
	buf32[_esep_ind(i)] =
	    (buf32[_esep_ind(i)] & (~_esep_mask(i))) | (val << _esep_shift(i));
}
