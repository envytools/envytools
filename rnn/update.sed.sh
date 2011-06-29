#!/bin/bash
if test renouveau.xml -nt renouveau; then
	./renouveau2rnn
fi

for i in renouveau/*; do
	if test "${i:${#i} - 4:4}" == ".xml"; then
		continue
	fi

	if test "$i" -nt "$i.xml"; then
		./text2xml "$i" > "$i.xml"
	fi
done

function addrs
{
	sed -nre "s/d NV01_SUBCHAN ([^ ]*) ([^ ]*).*/\1 \2/p"
}

function gen
{
	join <(./expand "$3" -v obj-class "$1"|addrs|sort) <(./expand "$4" -v obj-class "$2"|addrs|sort)|sed -re 's@\([^()]*\)@@g; s@([^ ]*) ([^ ]*) ([^ ]*)@s/\2/\3/g;@g; s@s/NV..TCL_@s/'"$1"'_@'|sort|uniq
}

gen NV10TCL NV10TCL renouveau/nv10-20tcl.xml nv10_3d.xml
gen NV20TCL NV20TCL renouveau/nv10-20tcl.xml nv20_3d.xml
gen NV34TCL NV30_3D renouveau/nv30-40tcl.xml nv30-40_3d.xml
gen NV40TCL NV40_3D renouveau/nv30-40tcl.xml nv30-40_3d.xml
gen NV50TCL NV50_3D renouveau/nv50tcl.xml nv50_3d.xml

