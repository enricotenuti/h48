#!/bin/sh

outdir=${1:-"../test/05_transform"}

mkdir -p "$outdir"
i=100

while read -r line; do
	[ -z "$line" ] && continue

	trans_piece="$(echo $line | awk '{print $1}' | tr -d 'rm')"
	move1="$(echo $line | awk '{print $2}')"
	move2="$(echo $line | awk '{print $3}')"

	rotation="rotation $trans_piece"

	file1="$(ls | grep move_*_${move1}.txt)"
	file2="$(ls | grep move_*_${move2}.txt)"
	echo "$rotation" >"$outdir/${i}_${trans_piece}r_${move1}.in"
	cat "$file1" >>"$outdir/${i}_${trans_piece}r_${move1}.in"
	cp "$file2" "$outdir/${i}_${trans_piece}r_${move1}.out"

	i=$((i+1))

	mirrored="mirrored $trans_piece"

	file1="$(ls | grep move_*_${move1}.txt)"
	file2="$(ls | grep move_*_${move2}.txt)"
	echo "$mirrored" >"$outdir/${i}_${trans_piece}m_${move1}.in"
	cat "$file1" >>"$outdir/${i}_${trans_piece}m_${move1}.in"
	cp "$file2" "$outdir/${i}_${trans_piece}m_${move1}.out"

	i=$((i+1))
done <transform_moves.txt
