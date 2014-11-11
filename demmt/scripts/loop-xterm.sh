#!/bin/sh

# You can use this script if you want to trace X and start something from xterm.

export DISPLAY=:0
while [ true ];
do
	xterm
	sleep 1
done
