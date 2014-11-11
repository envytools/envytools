#!/bin/bash

for c in $NVTR/*.mmt.xz; do
	cb=`basename $c`

	if [ -n "$2" ]; then
		./demmt -l $c 2>&1 | grep -v "^EOF$" | grep "$1" | grep -q "$2" && (echo "./demmt -l \$NVTR/$cb | less -ScR -p \"$1\"")
	else
		./demmt -l $c 2>&1 | grep -v "^EOF$" | grep -q "$1" && (echo "./demmt -l \$NVTR/$cb | less -ScR -p \"$1\"")
	fi
done
