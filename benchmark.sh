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
		for i in 1 8 64; do
			make clean
			THREADS=$i ./configure.sh
			TOOL=benchmark_performance TOOLARGS="$ARGS" make tool
		done
		;;
	architecture)
		echo "Architecture Benchmark..."
		for i in 1 8; do
			make clean
			THREADS=$i ARCH=PORTABLE ./configure.sh
			TOOL=benchmark_performance TOOLARGS="$ARGS" make tool
		done
		;;
	memory)
		echo "Memory Benchmark..."
		for i in {1..6}; do
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
		for i in 64 32 16 8 4 2 1; do
			make clean
			THREADS=$i ./configure.sh
			TOOL=benchmark_performance TOOLARGS="$ARGS" make tool
		done
		;;
	depth)
		echo "Depth Benchmark..."
			make clean
			THREADS=$THREADS ./configure.sh
			TOOL=benchmark_performance TOOLARGS="$ARGS" make tool
		;;
	*)
	echo "Invalid argument. Possible values:"
	echo "  - performance"
	echo "  - architecture"
	echo "  - memory"
	echo "  - nodes"
	echo "  - multithread"
	echo "  - depth"
	exit 1
	;;
esac