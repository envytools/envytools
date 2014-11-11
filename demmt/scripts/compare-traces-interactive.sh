#!/bin/bash

if [ "$1" = "" ];
then
	if [ "$NVTR" = "" ]; then
		echo "NVTR env var empty"
		exit 1
	fi

	files=$NVTR/*.mmt.xz
else
	files=$NVTR/$1
fi

overwrite=0
diffandskip=0

for c in $files; do
	cb=`basename $c`

	echo "-----------------------------------------------------------"
	echo "$cb"
	if [ "$1" != "" ]; then
		echo "diff -u <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) | less -ScR"
	fi

	cmp <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) >/dev/null
	if [ $? -ne 0 ];
	then
		end=0

		if [ $overwrite -eq 1 ]; then
			rm $NVTR/demmt/$cb.demmt.xz
			./demmt -q -l $NVTR/$cb | xz -9 > $NVTR/demmt/$cb.demmt.xz
			end=1
		fi
		if [ $diffandskip -eq 1 ]; then
			diff --label "$cb.old" --label "$cb.new" -u <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) | less -ScR
			end=1
		fi

		while [ $end -eq 0 ];
		do
			echo "Expected output differ. What do you want to do?"
			echo " - Show diff (d)"
			echo " - Show diff with big context (D)"
			echo " - Show diff --ignore-all-space (w)"
			echo " - Show diff and skip all (I)"
			echo " - Skip (enter)"
			echo " - Overwrite (o)"
			echo " - Overwrite all (O)"
			echo " - Show \"quick\" (f)"
			echo " - Show \"full\" (g)"
			echo " - Quit (q)"

			read X
			if [ "$X" = "d" ]; then
				echo "diff -u <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) | less -ScR"
				diff --label "$cb.old" --label "$cb.new" -u <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) | less -ScR
			elif [ "$X" = "w" ]; then
				echo "diff -uw <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) | less -ScR"
				diff --label "$cb.old" --label "$cb.new" -uw <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) | less -ScR
			elif [ "$X" = "D" ]; then
				echo "diff -U 10000 -u <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) | less -ScR"
				diff --label "$cb.old" --label "$cb.new" -U 10000 -u <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) | less -ScR
			elif [ "$X" = "I" ]; then
				end=1
				diffandskip=1
				diff --label "$cb.old" --label "$cb.new" -u <(xzcat $NVTR/demmt/$cb.demmt.xz) <(./demmt -q -l $NVTR/$cb) | less -ScR
			elif [ "$X" = "f" ]; then
				./demmt -q -l $NVTR/$cb
			elif [ "$X" = "g" ]; then
				./demmt -l $NVTR/$cb
			elif [ "$X" = "o" ]; then
				./demmt -q -l $NVTR/$cb | xz -9 > $NVTR/demmt/$cb.demmt.xz
				end=1
			elif [ "$X" = "O" ]; then
				./demmt -q -l $NVTR/$cb | xz -9 > $NVTR/demmt/$cb.demmt.xz
				end=1
				overwrite=1
			elif [ "$X" = "" ]; then
				end=1
			elif [ "$X" = "q" ]; then
				exit 1
			else
				echo "huh?"
			fi
		done
	fi


	cmp <(xzcat $NVTR/demmt/$cb-raw.demmt.xz) <(./demmt -l $NVTR/$cb) >/dev/null
	if [ $? -ne 0 ];
	then
		end=0

		if [ $overwrite -eq 1 ]; then
			rm $NVTR/demmt/$cb-raw.demmt.xz
			./demmt -l $NVTR/$cb | xz -9 > $NVTR/demmt/$cb-raw.demmt.xz
			end=1
		fi
		if [ $diffandskip -eq 1 ]; then
			diff --label "$cb.old" --label "$cb.new" -u <(xzcat $NVTR/demmt/$cb-raw.demmt.xz) <(./demmt -l $NVTR/$cb) | less -ScR
			end=1
		fi

		while [ $end -eq 0 ];
		do
			echo "Expected RAW output differ. What do you want to do?"
			echo " - Show diff (d)"
			echo " - Show diff with big context (D)"
			echo " - Show diff --ignore-all-space (w)"
			echo " - Show diff and skip all (I)"
			echo " - Skip (enter)"
			echo " - Overwrite (o)"
			echo " - Overwrite all (O)"
			echo " - Show \"quick\" (f)"
			echo " - Show \"full\" (g)"
			echo " - Quit (q)"

			read X
			if [ "$X" = "d" ]; then
				echo "diff -u <(xzcat $NVTR/demmt/$cb-raw.demmt.xz) <(./demmt -l $NVTR/$cb) | less -ScR"
				diff --label "$cb.old" --label "$cb.new" -u <(xzcat $NVTR/demmt/$cb-raw.demmt.xz) <(./demmt -l $NVTR/$cb) | less -ScR
			elif [ "$X" = "w" ]; then
				echo "diff -uw <(xzcat $NVTR/demmt/$cb-raw.demmt.xz) <(./demmt -l $NVTR/$cb) | less -ScR"
				diff --label "$cb.old" --label "$cb.new" -uw <(xzcat $NVTR/demmt/$cb-raw.demmt.xz) <(./demmt -l $NVTR/$cb) | less -ScR
			elif [ "$X" = "D" ]; then
				echo "diff -U 10000 -u <(xzcat $NVTR/demmt/$cb-raw.demmt.xz) <(./demmt -l $NVTR/$cb) | less -ScR"
				diff --label "$cb.old" --label "$cb.new" -U 10000 -u <(xzcat $NVTR/demmt/$cb-raw.demmt.xz) <(./demmt -l $NVTR/$cb) | less -ScR
			elif [ "$X" = "I" ]; then
				end=1
				diffandskip=1
				diff --label "$cb.old" --label "$cb.new" -u <(xzcat $NVTR/demmt/$cb-raw.demmt.xz) <(./demmt -l $NVTR/$cb) | less -ScR
			elif [ "$X" = "f" ]; then
				./demmt -q -l $NVTR/$cb
			elif [ "$X" = "g" ]; then
				./demmt -l $NVTR/$cb
			elif [ "$X" = "o" ]; then
				./demmt -l $NVTR/$cb | xz -9 > $NVTR/demmt/$cb-raw.demmt.xz
				end=1
			elif [ "$X" = "O" ]; then
				./demmt -l $NVTR/$cb | xz -9 > $NVTR/demmt/$cb-raw.demmt.xz
				end=1
				overwrite=1
			elif [ "$X" = "" ]; then
				end=1
			elif [ "$X" = "q" ]; then
				exit 1
			else
				echo "huh?"
			fi
		done
	fi
done
