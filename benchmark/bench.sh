#!/bin/sh

CC="cc -std=c99 -pthread -O3 -D$CUBETYPE"
if [ "$CUBETYPE" = "CUBE_AVX2" ]; then
        CC="$CC -mavx2"
fi

BENCHBIN="benchmark/run"
BENCHDIR="benchmark/results"
CUBEOBJ="cube.o"

$CC -o $BENCHBIN benchmark/bench.c $CUBEOBJ || exit 1;

d="$(date +'%Y-%m-%d-%H-%M-%S')" 
mkdir -p "$BENCHDIR"
$BENCHBIN | tee "$BENCHDIR/results-$d.txt" "$BENCHDIR/results.txt"

echo ""
echo "Results saved to $BENCHDIR/results.txt"

rm -rf $BENCHBIN $CUBEOBJ
