all: headergen expand lookup demmio nvbios nv50_texture.h

headergen: headergen.c rnn.c rnn.h
	gcc -o headergen headergen.c rnn.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

expand: expand.c rnn.c rnn.h rnndec.c rnndec.h
	gcc -o expand expand.c rnn.c rnndec.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

lookup: lookup.c rnn.c rnn.h rnndec.c rnndec.h
	gcc -o lookup lookup.c rnn.c rnndec.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

demmio: demmio.c rnn.c rnn.h rnndec.c rnndec.h
	gcc -o demmio demmio.c rnn.c rnndec.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

nv50_texture.xml.h: nv50_texture.xml headergen
	./headergen nv50_texture.xml

nvbios: nvbios.c
	gcc nvbios.c -o nvbios -Wall

clean:
	rm -f headergen nv50_texture.h
