#include <pthread.h>

#include "../tool.h"

const char *solver = "h48";
const char *options = "0;4;20";
const char *filename = "tables/h48h0k4";
char *buf;

char *scrambles[] = {
	"R D' R2 D R U2 R' D' R U2 R D R'", /* 12 optimal */
	"RLUD RLUD RLUD", /* 12 optimal */
	NULL
};

void run(void) {
	int i;
	int64_t n;
	char sol[100], cube[22];

	printf("Solved the following scrambles:\n\n");
	for (i = 0; scrambles[i] != NULL; i++) {
		printf("%d. %s\n", i+1, scrambles[i]);
		fprintf(stderr, "Solving scramble %s\n", scrambles[i]);
		if (nissy_frommoves(scrambles[i], cube) == -1) {
			fprintf(stderr, "Invalid scramble\n");
			printf("Invalid\n");
			continue;
		}
		n = nissy_solve(
		    cube, "h48", options, "", 0, 20, 1, -1, buf, sol);
		if (n == 0) {
			printf("No solution\n");
			fprintf(stderr, "No solution found\n");
		} else {
			printf("Solutions:\n%s\n", sol);
		}
	}
}

int main(void) {

	srand(time(NULL));
	nissy_setlogger(log_stderr);

	if (getdata(solver, options, &buf, filename) != 0)
		return 1;

	timerun(run, "small solver benchmark");

	free(buf);
	return 0;
}