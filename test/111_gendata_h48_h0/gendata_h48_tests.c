#include "../test.h"

#define MAXDEPTH 5
#define COCSEPSIZE 1119792
#define ETABLESIZE ((3393 * 495 * 70) >> 1)

int64_t gendata_h48h0k4(void *, uint8_t);

void run(void) {
	char str[STRLENMAX];
	uint8_t i;
	uint32_t *buf, *h48info;
	size_t result;

	fgets(str, STRLENMAX, stdin);
	buf = (uint32_t *)malloc(sizeof(uint32_t) * 60000000);
	result = gendata_h48h0k4(buf, MAXDEPTH);
	h48info = buf + (ETABLESIZE + COCSEPSIZE) / 4;

	printf("%zu\n\n", result);

	printf("cocsepdata:\n");
	printf("Classes: %" PRIu32 "\n", buf[COCSEPSIZE/4-12]);
	printf("Max value: %" PRIu32 "\n", buf[COCSEPSIZE/4-11]);
	for (i = 0; i < 10; i++)
		printf("%" PRIu32 ": %" PRIu32 "\n", i, buf[COCSEPSIZE/4-10+i]);

	printf("\nh48:\n");
	for (i = 0; i < MAXDEPTH+1; i++)
		printf("%" PRIu32 ": %" PRIu32 "\n", i, h48info[i+1]);

	free(buf);
}