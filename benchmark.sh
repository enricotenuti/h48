#!/bin/bash
if [ -z "$1" ]; then
	echo "No argument provided. Possible values:"
	echo "  - performance"
	echo "  - architecture"
	echo "  - memory"
	echo "  - nodes"
	echo "  - multithread"
	exit 1
fi

input=$1
ARGS="h48h6k2"
THREADS=8

case $input in
	performance)
		echo "Performance Benchmark..."
		make clean
		THREADS=$THREADS ./configure.sh
		TOOL=benchmark_performance TOOLARGS="$ARGS" make tool
		;;
	architecture)
		echo "Architecture Benchmark..."
		make clean
		THREADS=$THREADS ./configure.sh
		TOOL=benchmark_performance TOOLARGS="$ARGS" make tool
		make clean
		THREADS=$THREADS ARCH=PORTABLE ./configure.sh
		TOOL=benchmark_performance TOOLARGS="$ARGS" make tool
		;;
	memory)
		echo "Memory Benchmark..."
		for i in {0..7}; do
			make clean
			THREADS=$THREADS ./configure.sh
			M_ARGS="h48h${i}k2"
			TOOL=benchmark_performance TOOLARGS="$M_ARGS" make tool
		done
		;;
	nodes)
		echo "Nodes Benchmark..."
		make clean
		THREADS=1 ./configure.sh
		TOOL=benchmark_nodes TOOLARGS="$ARGS" make tool
		;;
	multithread)
		echo "Multithread Benchmark..."
		make clean
		for i in 1 2 4 8 16; do
			make clean
			THREADS=$i ./configure.sh
			TOOL=benchmark_performance TOOLARGS="$ARGS" make tool
		done
		;;
	*)
	echo "Invalid argument. Possible values:"
	echo "  - performance"
	echo "  - architecture"
	echo "  - memory"
	echo "  - nodes"
	echo "  - multithread"
	exit 1
	;;
esac