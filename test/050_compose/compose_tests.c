#include "../test.h"

cube_t compose(cube_t, cube_t);

void run(void) {
	char str[STRLENMAX];
	cube_t c1, c2, c3;

	fgets(str, STRLENMAX, stdin);
	c1 = readcube("H48", str);
	fgets(str, STRLENMAX, stdin);
	c2 = readcube("H48", str);

	c3 = compose(c1, c2);

	if (iserror(c3)) {
		printf("Error composing cubes\n");
	} else if (!issolvable(c3)) {
		printf("Composed cube is not solvable\n");
	} else {
		writecube("H48", c3, NISSY_SIZE_H48, str);
		printf("%s\n", str);
	}
}
