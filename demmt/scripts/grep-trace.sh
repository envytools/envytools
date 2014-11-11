#!/bin/bash

for c in $NVTR/*.mmt.xz; do
	cb=`basename $c`
	echo "$cb"

	./demmt -l $c 2>/dev/null | grep "$1"
done
