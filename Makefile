<<<<<<< HEAD
all: headergen expand lookup demmio nvbios nv50_texture.h ctxdis nv50dis nvc0dis fcdis pmdis

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
	rm -f ctxdis nv50dis nvc0dis fcdis pmdis

nv50dis: nv50dis.c coredis.h coredis.c
	cc -o nv50dis nv50dis.c coredis.c

nvc0dis: nvc0dis.c coredis.h coredis.c
	cc -o nvc0dis nvc0dis.c coredis.c

ctxdis: ctxdis.c coredis.h coredis.c
	cc -o ctxdis ctxdis.c coredis.c

fcdis: fcdis.c coredis.h coredis.c
	cc -o fcdis fcdis.c coredis.c

pmdis: pmdis.c coredis.h coredis.c
	cc -o pmdis pmdis.c coredis.c
