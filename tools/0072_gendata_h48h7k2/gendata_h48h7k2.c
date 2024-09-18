#include "../tool.h"

uint64_t expected[21] = {
	/* Base value is 8 */
	[0] = 0, /* Unknown */
	[1] = 0, /* Unknown */
	[2] = 0, /* Unknown */
	[3] = 0, /* Unknown */
};

void run(void) {
	gendata_run("h48", "7;2;20", "tables/h48h7k2", expected);
}

int main(void) {
	nissy_setlogger(log_stderr);

	timerun(run, "benchmark gendata_h48 h = 7, k = 2");

	return 0;
}
