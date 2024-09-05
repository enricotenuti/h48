/*
If you include this file, you should also include the following:

inttypes, stdarg, stdbool, string

All the functions below return 0 in case of success and a positive
number in case of error, unless otherwise specified.

Arguments of type char [static 22] denote a cube in B32 format.
Other available formats are H48 and SRC. See README.md for more info on
these formats.

Accepted moves are U, D, R, L, F and B, optionally followed by a 2,
a ' or a 3.

A transformation must be given in the format
    (rotation|mirrored) (2 letters)
for example 'rotation UF' or 'mirrored BL'.
*/

/* Apply the secod argument as a permutation on the first argument */
int64_t nissy_compose(
	const char cube[static 22],
	const char permutation[static 22],
	char result[static 22]
);

/* Compute the inverse of the given cube */
int64_t nissy_inverse(
	const char cube[static 22],
	char result[static 22]
);

/* Apply the given sequence of moves on the given cube */
int64_t nissy_applymoves(
	const char cube[static 22],
	const char *moves,
	char result[static 22]
);

/* Apply the single given transformation to the given cube */
int64_t nissy_applytrans(
	const char cube[static 22],
	const char *transformation,
	char result[static 22]
);

/* Return the cube obtained by applying the given moves to the solved cube */
int64_t nissy_frommoves(
	const char *moves,
	char result[static 22]
);

/* Convert the given cube between the two given formats */
int64_t nissy_convert(
	const char *format_in,
	const char *format_out,
	const char *cube_string,
	char *result
);

/* Get the cube with the given ep, eo, cp and co values. */
int64_t nissy_getcube(
	int64_t ep,
	int64_t eo,
	int64_t cp,
	int64_t co,
	const char *options,
	char result[static 22]
);

/*
Returns the size of the data generated by nissy_gendata, when called with
the same parameters, or -1 in case of error. The returned value can be
slightly larger than the actual table size.
*/
int64_t nissy_datasize(
	const char *solver,
	const char *options /* TODO: remove options, use only solver name */
);

/* Returns the number of bytes written, or -1 in case of error */
int64_t nissy_gendata(
	const char *solver,
	const char *options, /* TODO: remove options, use only solver name */
	void *generated_data
);

/* Returns the number of solutions found, or -1 in case of error */
int64_t nissy_solve(
	const char cube[static 22],
	const char *solver, 
	const char *options, /* TODO: remove options, use only solver name */
	const char *nisstype, /* TODO: remove, use flags */
	int8_t minmoves,
	int8_t maxmoves,
	int64_t maxsolutions,
	int8_t optimal,
	const void *data,
	char *solutions
);

/* Set a global logger function used by this library. */
void nissy_setlogger(void (*logger_function)(const char *, ...));