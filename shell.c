#include <inttypes.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/cube.h"

#define PRINTCUBE_BUFFER_SIZE 1024 /* Should be enough */
#define SOLUTIONS_BUFFER_SIZE 500000 /* Should be enough */
#define MAX_PATH_LENGTH 10000 /* Should be enough */

typedef struct {
	int command_index;
	char cube[22];
	char cube_perm[22];
	char *str_cube;
	char *str_format;
	char *str_format_in;
	char *str_format_out;
	char *str_moves;
	char *str_trans;
	char *str_solver;
	char *str_options; /* TODO: remove, use only solver */
	char *str_nisstype; /* TODO: remove, use flags */
	int8_t minmoves;
	int8_t maxmoves;
	int8_t optimal;
	int64_t maxsolutions;
	int64_t datasize; /* Option for gendata + solve, TODO change? */
} args_t;

static void print_cube_result(int64_t, char [static 22]);
static void print_str_result(int64_t, char *);

static int64_t compose_exec(args_t *);
static int64_t inverse_exec(args_t *);
static int64_t applymoves_exec(args_t *);
static int64_t applytrans_exec(args_t *);
static int64_t frommoves_exec(args_t *);
static int64_t readcube_exec(args_t *);
static int64_t writecube_exec(args_t *);
static int64_t convertcube_exec(args_t *);
static int64_t datasize_exec(args_t *);
static int64_t gendata_exec(args_t *);
static int64_t solve_exec(args_t *);

static int parse_args(int, char **, args_t *);
static bool parse_int8(char *, int8_t *);
static bool parse_int64(char *, int64_t *);

static bool set_cube(int, char **, args_t *);
static bool set_cube_perm(int, char **, args_t *);
static bool set_str_cube(int, char **, args_t *);
static bool set_str_format(int, char **, args_t *);
static bool set_str_format_in(int, char **, args_t *);
static bool set_str_format_out(int, char **, args_t *);
static bool set_str_moves(int, char **, args_t *);
static bool set_str_trans(int, char **, args_t *);
static bool set_str_solver(int, char **, args_t *);
static bool set_str_options(int, char **, args_t *);
static bool set_str_nisstype(int, char **, args_t *);
static bool set_minmoves(int, char **, args_t *);
static bool set_maxmoves(int, char **, args_t *);
static bool set_optimal(int, char **, args_t *);
static bool set_maxsolutions(int, char **, args_t *);
 
#define COMMAND(N, E) { .name = N, .exec = E }
struct {
	char *name;
	int64_t (*exec)(args_t *);
} commands[] = {
	COMMAND("compose", compose_exec),
	COMMAND("inverse", inverse_exec),
	COMMAND("applymoves", applymoves_exec),
	COMMAND("applytrans", applytrans_exec),
	COMMAND("frommoves", frommoves_exec),
	COMMAND("readcube", readcube_exec),
	COMMAND("writecube", writecube_exec),
	COMMAND("convertcube", convertcube_exec),
	COMMAND("datasize", datasize_exec),
	COMMAND("gendata", gendata_exec),
	COMMAND("solve", solve_exec),
	COMMAND(NULL, NULL)
};

#define OPTION(N, A, S) { .name = N, .nargs = A, .set = S }
struct {
	char *name;
	int nargs;
	bool (*set)(int, char **, args_t *);
} options[] = {
	OPTION("-cube", 1, set_cube),
	OPTION("-perm", 1, set_cube_perm),
	OPTION("-cubestr", 1, set_str_cube),
	OPTION("-format", 1, set_str_format),
	OPTION("-fin", 1, set_str_format_in),
	OPTION("-fout", 1, set_str_format_out),
	OPTION("-moves", 1, set_str_moves),
	OPTION("-trans", 1, set_str_trans),
	OPTION("-solver", 1, set_str_solver),
	OPTION("-options", 1, set_str_options), /* TODO: remove, use only solver */
	OPTION("-nisstype", 1, set_str_nisstype), /* TODO: remove, use flags */
	OPTION("-m", 1, set_minmoves),
	OPTION("-M", 1, set_maxmoves),
	OPTION("-O", 1, set_optimal),
	OPTION("-n", 1, set_maxsolutions),
	OPTION(NULL, 0, NULL)
};

char *tablepaths[] = {
	"tables/",
	"",
	NULL
};

static void
print_cube_result(int64_t ret, char result[static 22])
{
	switch (ret) {
	case 0:
		break;
	case 1:
		fprintf(stderr, "Warning: resulting cube not solvable\n");
		break;
	case 2: /* Fallthrough */
	default:
		fprintf(stderr, "Unknown error (result is inconsistent)\n");
	}

	printf("%s\n", result);
}

static void
print_str_result(int64_t ret, char *result)
{
	switch (ret) {
	case 0:
		break;
	default:
		fprintf(stderr, "Unknown error\n");
	}

	printf("%s\n", result);
}

static int64_t
compose_exec(args_t *args)
{
	char result[22];
	int64_t ret;

	ret = nissy_compose(args->cube, args->cube_perm, result);
	print_cube_result(ret, result);

	return ret;
}

static int64_t
inverse_exec(args_t *args)
{
	char result[22];
	int64_t ret;

	ret = nissy_inverse(args->cube, result);
	print_cube_result(ret, result);

	return ret;
}

static int64_t
applymoves_exec(args_t *args)
{
	char result[22];
	int64_t ret;

	ret = nissy_applymoves(args->cube, args->str_moves, result);
	print_cube_result(ret, result);

	return ret;
}

static int64_t
applytrans_exec(args_t *args)
{
	char result[22];
	int64_t ret;

	ret = nissy_applytrans(args->cube, args->str_trans, result);
	print_cube_result(ret, result);

	return ret;
}

static int64_t
frommoves_exec(args_t *args)
{
	char result[22];
	int64_t ret;

	ret = nissy_frommoves(args->str_trans, result);
	print_cube_result(ret, result);

	return ret;
}

static int64_t
readcube_exec(args_t *args)
{
	char result[22];
	int64_t ret;

	ret = nissy_readcube(args->str_format, args->str_cube, result);
	print_cube_result(ret, result);

	return ret;
}

static int64_t
writecube_exec(args_t *args)
{
	char result[PRINTCUBE_BUFFER_SIZE];
	int64_t ret;

	ret = nissy_writecube(args->str_format, args->str_cube, result);
	print_str_result(ret, result);

	return ret;
}

static int64_t
convertcube_exec(args_t *args)
{
	char result[PRINTCUBE_BUFFER_SIZE];
	int64_t ret;

	ret = nissy_convertcube(
	    args->str_format_in, args->str_format_out, args->str_cube, result);
	print_str_result(ret, result);

	return ret;
}

static int64_t
datasize_exec(args_t *args)
{
	int64_t ret;

	ret = nissy_datasize(args->str_solver, args->str_options);
	if (ret < 0)
		fprintf(stderr, "Unknown error (make sure solver is valid)\n");
	printf("%" PRId64 "\n", ret);

	return ret;
}

static int64_t
gendata_exec(args_t *args)
{
	int i;
	FILE *file;
	char *buf, path[MAX_PATH_LENGTH];
	int64_t ret, size;
	size_t written;

	/* TODO: should give warning if overwriting existing file */
	for (i = 0; tablepaths[i] != NULL; i++) {
		strcpy(path, tablepaths[i]);
		strcat(path, args->str_solver);
		file = fopen(path, "wb");
		if (file != NULL)
			break;
	}

	if (tablepaths[i] == NULL) {
		fprintf(stderr, "Cannot write data to file\n");
		return -2;
	}

	size = nissy_datasize(args->str_solver, args->str_options);
	if (size < 0) {
		fprintf(stderr,
		    "Unknown error in retrieving data size"
		    "(make sure solver is valid)\n");
		return -3;
	}

	buf = malloc(size);

	ret = nissy_gendata(args->str_solver, args->str_options, buf);
	if (ret < 0) {
		fprintf(stderr, "Unknown error in generating data\n");
		free(buf);
		return -4;
	}
	if (ret != size) {
		fprintf(stderr, "Unknown error: unexpected data size\n");
		free(buf);
		return -5;
	}

	written = fwrite(buf, size, 1, file);
	free(buf);

	if (written != (int64_t)size) {
		fprintf(stderr,
		    "Error: data was generated correctly, but could not be "
		    "written to file (generated %" PRId64 " bytes, written "
		    "%zu)\n", written, size);
		return -6;
	}

	args->datasize = size;
	fprintf(stderr, "Data written to %s\n", path);

	return 0;
}

static int64_t
solve_exec(args_t *args)
{
	int i;
	FILE *file;
	char *buf, solutions[SOLUTIONS_BUFFER_SIZE], path[MAX_PATH_LENGTH];
	int64_t ret, gendata_ret;
	size_t read;

	for (i = 0; tablepaths[i] != NULL; i++) {
		strcpy(path, tablepaths[i]);
		strcat(path, args->str_solver);
		file = fopen(path, "rb");
		if (file != NULL)
			break;
	}

	if (tablepaths[i] == NULL) {
		fprintf(stderr,
		    "Cannot read data file, "
		    "generating it (this can take a while)\n");
		gendata_ret = gendata_exec(args);
		if (gendata_ret)
			return gendata_ret;
	}

	/* Ugh, this is not elegant TODO */
	for (i = 0; tablepaths[i] != NULL; i++) {
		strcpy(path, tablepaths[i]);
		strcat(path, args->str_solver);
		file = fopen(path, "rb");
		if (file != NULL)
			break;
	}

	if (tablepaths[i] == NULL) {
		fprintf(stderr, "Error: data file not found\n");
		return -1;
	}

	buf = malloc(args->datasize);
	read = fread(buf, args->datasize, 1, file);
	if (read != args->datasize) {
		fprintf(stderr, "Error reading data from file\n");
		return -2;
	}

	ret = nissy_solve(
	    args->cube, args->str_solver, args->str_options, args->str_nisstype,
	    args->minmoves, args->maxmoves, args->maxsolutions, args->optimal,
	    buf, solutions);

	free(buf);

	if (ret == 0)
		fprintf(stderr, "No solutions found\n");
	else
		printf("%s", solutions);

	return 0;
}

static int
parse_args(int argc, char **argv, args_t *args)
{
/* TODO: this function should set sensible defaults for all options */
	int i, j, n;

	if (argc == 0) {
		printf("No command given\n");
		return 1;
	}

	for (i = 0; commands[i].name != NULL; i++) {
		if (!strcmp(argv[0], commands[i].name)) {
			args->command_index = i;
			break;
		}
	}

	if (commands[i].name == NULL) {
		fprintf(stderr, "Unknown command %s\n", argv[0]);
		return 1;
	}

	for (i = 1; i < argc; i++) {
		for (j = 0; options[j].name != NULL; j++) {
			n = argc - i - 1;
			if (strcmp(argv[i], options[j].name))
				continue;
			if (n < options[j].nargs) {
				fprintf(stderr,
				    "Too few arguments for option %s\n",
				    options[j].name);
				return 1;
			}
			if (!options[j].set(n, argv+i, args)) {
				fprintf(stderr,
				    "Error parsing arguments for option %s\n",
				    options[j].name);
				return 1;
			}
			i += options[j].nargs;
			break;
		}
		if (options[j].name == NULL) {
			fprintf(stderr, "Unknown option %s\n", argv[i]);
			return 1;
		}
	}

	return 0;
}

bool
parse_int8(char *argv, int8_t *result)
{
	bool noerror;
	int64_t n;

	noerror = parse_int64(argv, &n);
	*result = (int8_t)n;

	return noerror && n >= INT8_MIN && n <= INT8_MAX;
}

bool
parse_int64(char *argv, int64_t *result)
{
	*result = strtoll(argv, NULL, 10);

	return errno != 0;
}

static bool
set_cube(int argc, char **argv, args_t *args)
{
	memcpy(args->cube, argv[0], 22);
	args->cube[21] = 0;

	return true;
}

static bool
set_cube_perm(int argc, char **argv, args_t *args)
{
	memcpy(args->cube_perm, argv[0], 22);
	args->cube_perm[21] = 0;

	return true;
}

static bool
set_str_cube(int argc, char **argv, args_t *args)
{
	args->str_cube = argv[0];

	return true;
}

static bool
set_str_format(int argc, char **argv, args_t *args)
{
	args->str_format = argv[0];

	return true;
}

static bool
set_str_format_in(int argc, char **argv, args_t *args)
{
	args->str_format_in = argv[0];

	return true;
}

static bool
set_str_format_out(int argc, char **argv, args_t *args)
{
	args->str_format_out = argv[0];

	return true;
}

static bool
set_str_moves(int argc, char **argv, args_t *args)
{
	args->str_moves = argv[0];

	return true;
}

static bool
set_str_trans(int argc, char **argv, args_t *args)
{
	args->str_trans = argv[0];

	return true;
}

static bool
set_str_solver(int argc, char **argv, args_t *args)
{
	args->str_solver = argv[0];

	return true;
}

static bool
set_str_options(int argc, char **argv, args_t *args)
{
	args->str_options = argv[0];

	return true;
}

static bool
set_str_nisstype(int argc, char **argv, args_t *args)
{
	args->str_nisstype = argv[0];

	return true;
}

static bool
set_minmoves(int argc, char **argv, args_t *args)
{
	return parse_int8(argv[0], &args->minmoves);
}

static bool
set_maxmoves(int argc, char **argv, args_t *args)
{
	return parse_int8(argv[0], &args->maxmoves);
}

static bool
set_optimal(int argc, char **argv, args_t *args)
{
	return parse_int8(argv[0], &args->optimal);
}

static bool
set_maxsolutions(int argc, char **argv, args_t *args)
{
	return parse_int64(argv[0], &args->maxsolutions);
}

int main(int argc, char **argv)
{
	int parse_error;
	args_t args;

	parse_error = parse_args(argc-1, argv+1, &args);
	if (parse_error)
		return parse_error;

	return (int)commands[args.command_index].exec(&args);
}