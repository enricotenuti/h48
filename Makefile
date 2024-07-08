include config.mk

all: cube.o debugcube.o

cube.s: clean
	${CC} -D${CUBETYPE} ${CFLAGS} -c -S -o cube.s src/cube.c

cube.o: clean
	${CC} -D${CUBETYPE} ${CFLAGS} -c -o cube.o src/cube.c

debugcube.o: clean
	${CC} -D${CUBETYPE} ${DBGFLAGS} -c -o debugcube.o src/cube.c

clean:
	rm -rf *.o run

test: debugcube.o
	CUBETYPE=${CUBETYPE} TEST=${TEST} ./test/test.sh

tool: cube.o
	mkdir -p tools/results
	CUBETYPE=${CUBETYPE} ./tools/run_tool.sh

debugtool: debugcube.o
	mkdir -p tools/results
	CUBETYPE=${CUBETYPE} DEBUG=1 ./tools/run_tool.sh

shell: cube.o
	mkdir -p tables
	${CC} ${CFLAGS} -o run cube.o shell.c

debugshell: debugcube.o
	mkdir -p tables
	${CC} ${DBGFLAGS} -o run debugcube.o shell.c

.PHONY: all clean test tool debugtool shell debugshell
