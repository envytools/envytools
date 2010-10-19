all: headergen expand lookup demmio nvbios envydis

headergen: headergen.c rnn.c rnn.h
	gcc -o headergen headergen.c rnn.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

expand: expand.c rnn.c rnn.h rnndec.c rnndec.h
	gcc -o expand expand.c rnn.c rnndec.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

lookup: lookup.c rnn.c rnn.h rnndec.c rnndec.h
	gcc -o lookup lookup.c rnn.c rnndec.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

demmio: demmio.c rnn.c rnn.h rnndec.c rnndec.h dis.h pmd-back.c coredis.c
	gcc -o demmio demmio.c rnn.c rnndec.c pmd-back.c ctxd-back.c coredis.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

nvbios: nvbios.c
	gcc nvbios.c -o nvbios -Wall

clean:
	rm -f headergen nv50_texture.h
	rm -f envydis

envydis: envydis.c nv50d-back.c nvc0d-back.c ctxd-back.c vp2d-back.c fucd-back.c pmd-back.c dis.h coredis.c
	cc -o envydis envydis.c nv50d-back.c nvc0d-back.c ctxd-back.c vp2d-back.c fucd-back.c pmd-back.c coredis.c
