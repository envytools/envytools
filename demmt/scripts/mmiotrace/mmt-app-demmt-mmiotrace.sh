#!/bin/bash

# This script lets you trace an application, decode it with demmt and feed
# decoded output into mmiotrace, in synchronous manner.

#export PATH=/path/to/valgrind and demmt:$PATH

if [ "$1" = "" ]; then
	echo "no args"
	exit
fi

export MMT2DEMMT=/tmp/mmt2demmt-$RANDOM
export SYNC=/tmp/demmt2mmt-$RANDOM
export TRACE=/sys/kernel/debug/tracing/trace_marker

rm -f $SYNC $MMT2DEMMT
mkfifo $SYNC $MMT2DEMMT

valgrind --tool=mmt --log-file=$MMT2DEMMT --mmt-sync-file=$SYNC \
	--mmt-trace-nvidia-ioctls --mmt-trace-file=/dev/nvidia0 \
	--mmt-trace-file=/dev/nvidia1 --mmt-trace-file=/dev/nvidiactl \
	$* &

demmt -s $SYNC <$MMT2DEMMT >$TRACE

rm $SYNC $MMT2DEMMT
