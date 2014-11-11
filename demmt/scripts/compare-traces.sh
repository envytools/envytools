#!/bin/bash

for c in $NVTR/*.mmt.xz; do
	cb=`basename $c`

	echo   "diff -u <(xzcat \$NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l \$NVTR/$cb) | less -ScR"
		cmp     <(xzcat  $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l  $NVTR/$cb)
done
