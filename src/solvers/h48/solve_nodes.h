typedef struct {
	cube_t cube;
	cube_t inverse;
	int8_t nmoves;
	int8_t depth;
	uint8_t moves[MAXLEN];
	_Atomic int64_t *nsols;
	int64_t maxsolutions;
	uint8_t h;
	uint8_t k;
	uint8_t base;
	const uint32_t *cocsepdata;
	const uint8_t *h48data;
	const uint8_t *h48data_fallback;
	uint64_t solutions_size;
	char **nextsol;
	uint8_t nissbranch;
	int8_t npremoves;
	uint8_t premoves[MAXLEN];
    uint64_t n_nodes;
} dfsarg_nodesh48_t;


STATIC void solve_h48_appendsolution_nodes(dfsarg_nodesh48_t *);
STATIC int64_t solve_h48_dfs_nodes(dfsarg_nodesh48_t *);
STATIC bool solve_h48_stop_nodes(dfsarg_nodesh48_t *);
STATIC int64_t solve_h48_nodes(cube_t, int8_t, int8_t,
    int8_t, uint64_t, const void *, uint64_t, char *, uint64_t *);



STATIC void
solve_h48_appendsolution_nodes(dfsarg_nodesh48_t *arg)
{
	int64_t strl;
	uint8_t invertedpremoves[MAXLEN];
	char *solution = *arg->nextsol; 

	strl = writemoves(
	    arg->moves, arg->nmoves, arg->solutions_size, *arg->nextsol);

	if (strl < 0)
		goto solve_h48_appendsolution_error;
	*arg->nextsol += strl-1; 
	arg->solutions_size -= strl-1;

	if (arg->npremoves) {
		**arg->nextsol = ' ';
		(*arg->nextsol)++;
		arg->solutions_size--;

		invertmoves(arg->premoves, arg->npremoves, invertedpremoves);
		strl = writemoves(invertedpremoves,
		    arg->npremoves, arg->solutions_size, *arg->nextsol);

		if (strl < 0)
			goto solve_h48_appendsolution_error;
		*arg->nextsol += strl-1;
		arg->solutions_size -= strl-1;
	}
	LOG("Solution found, %lld nodes visited: %s\n", arg->n_nodes, solution);
	**arg->nextsol = '\n';
	(*arg->nextsol)++;
	arg->solutions_size--;
	(*arg->nsols)++;

solve_h48_appendsolution_error:
	/* We could add some logging, but writemoves() already does */
	return;
}

STATIC_INLINE bool
solve_h48_stop_nodes(dfsarg_nodesh48_t *arg)
{
	uint32_t data, data_inv;
	int8_t cbound, cbound_inv, h48bound, h48bound_inv;
    int64_t coord, coord_inv;

    arg->n_nodes++;
	arg->nissbranch = MM_NORMAL;
	cbound = get_h48_cdata(arg->cube, arg->cocsepdata, &data);
	if (cbound + arg->nmoves + arg->npremoves > arg->depth)
		return true;

	cbound_inv = get_h48_cdata(arg->inverse, arg->cocsepdata, &data_inv);
	if (cbound_inv + arg->nmoves + arg->npremoves > arg->depth)
		return true;

	coord = coord_h48_edges(arg->cube, COCLASS(data), TTREP(data), arg->h);
    h48bound = get_h48_pval(arg->h48data, coord, arg->k);

	/* If the h48 bound is > 0, we add the base value.    */
	/* Otherwise, we use the fallback h0k4 value instead. */

	if (arg->k == 2) {
		if (h48bound == 0) {
            h48bound = get_h48_pval(
                arg->h48data_fallback, coord >> arg->h, 4);
		} else {
			h48bound += arg->base;
		}
	}
	if (h48bound + arg->nmoves + arg->npremoves > arg->depth)
		return true;
	if (h48bound + arg->nmoves + arg->npremoves == arg->depth)
		arg->nissbranch = MM_INVERSEBRANCH;

	coord_inv = coord_h48_edges(
        arg->inverse, COCLASS(data_inv), TTREP(data_inv), arg->h);
    h48bound_inv = get_h48_pval(arg->h48data, coord_inv, arg->k);
	if (arg->k == 2) {
		if (h48bound_inv == 0) {
            h48bound_inv = get_h48_pval(
                arg->h48data_fallback, coord_inv >> arg->h, 4);
		} else {
			h48bound_inv += arg->base;
		}
	}
	if (h48bound_inv + arg->nmoves + arg->npremoves > arg->depth)
		return true;
	if (h48bound_inv + arg->nmoves + arg->npremoves == arg->depth)
		arg->nissbranch = MM_NORMALBRANCH;

	return false;
}

STATIC int64_t
solve_h48_dfs_nodes(dfsarg_nodesh48_t *arg)
{
	dfsarg_nodesh48_t nextarg;
	int64_t ret;
	uint8_t m;

	if (*arg->nsols == arg->maxsolutions)
		return 0;

	if (solve_h48_stop_nodes(arg))
		return 0;

	if (issolved(arg->cube)) {
		if (arg->nmoves + arg->npremoves != arg->depth)
			return 0;
		solve_h48_appendsolution_nodes(arg);
		return 1;
	}

	nextarg = *arg;
	ret = 0;
	uint32_t allowed;
	if(arg->nissbranch & MM_INVERSE) {
		allowed = allowednextmove_h48(arg->premoves, arg->npremoves, arg->nissbranch);
		for (m = 0; m < 18; m++) {
			if(allowed & (1 << m)) {
				nextarg.npremoves = arg->npremoves + 1;
				nextarg.premoves[arg->npremoves] = m;
				nextarg.inverse = move(arg->inverse, m);
				nextarg.cube = premove(arg->cube, m);
                nextarg.n_nodes=arg->n_nodes;
				ret += solve_h48_dfs_nodes(&nextarg);
                arg->n_nodes=nextarg.n_nodes;
			}
		}
	} else {
		allowed = allowednextmove_h48(arg->moves, arg->nmoves, arg->nissbranch);
		for (m = 0; m < 18; m++) {
			if (allowed & (1 << m)) {
				nextarg.nmoves = arg->nmoves + 1;
				nextarg.moves[arg->nmoves] = m;
				nextarg.cube = move(arg->cube, m);
				nextarg.inverse = premove(arg->inverse, m);
                nextarg.n_nodes=arg->n_nodes;
				ret += solve_h48_dfs_nodes(&nextarg);
                arg->n_nodes=nextarg.n_nodes;
			}
		}
	}

	return ret;
}

STATIC int64_t
solve_h48_nodes(
	cube_t cube,
	int8_t minmoves,
	int8_t maxmoves,
	int8_t maxsolutions,
	uint64_t data_size,
	const void *data,
	uint64_t solutions_size,
	char *solutions,
    uint64_t *n_nodes_tot
)
{
	_Atomic int64_t nsols;
	dfsarg_nodesh48_t arg;
	tableinfo_t info, fbinfo;

	if(readtableinfo_n(data_size, data, 2, &info) != NISSY_OK)
		goto solve_h48_error_data;

	arg = (dfsarg_nodesh48_t) {
		.cube = cube,
		.inverse = inverse(cube),
		.nsols = &nsols,
		.maxsolutions = maxsolutions,
		.h = info.h48h,
		.k = info.bits,
		.base = info.base,
		.cocsepdata = (uint32_t *)((char *)data + INFOSIZE),
		.h48data = (uint8_t *)data + COCSEP_FULLSIZE + INFOSIZE,
		.solutions_size = solutions_size,
		.nextsol = &solutions,
        .n_nodes=0
	};

	if (info.bits == 2) {
		if (readtableinfo_n(data_size, data, 3, &fbinfo) != NISSY_OK)
			goto solve_h48_error_data;
		/* We only support h0k4 as fallback table */
		if (fbinfo.h48h != 0 || fbinfo.bits != 4)
			goto solve_h48_error_data;
		arg.h48data_fallback = arg.h48data + info.next;
	} else {
		arg.h48data_fallback = NULL;
	}

	nsols = 0;
	for (arg.depth = minmoves;
	     arg.depth <= maxmoves && nsols < maxsolutions;
	     arg.depth++)
	{
		LOG("Found %" PRId64 " solutions, searching at depth %"
		    PRId8 "\n", nsols, arg.depth);
		arg.nmoves = 0;
		arg.npremoves = 0;
		solve_h48_dfs_nodes(&arg);
        *n_nodes_tot = arg.n_nodes;
	}
	**arg.nextsol = '\0';
	(*arg.nextsol)++;


	return nsols;

solve_h48_error_data:
	LOG("solve_h48: error reading table\n");
	return NISSY_ERROR_DATA;
}
