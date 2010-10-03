all: headergen expand lookup demmio nvbios ctxdis nv50dis nvc0dis fucdis pmdis vp2dis

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
	rm -f ctxdis nv50dis nvc0dis fucdis pmdis

nv50dis: nv50dis.c nv50d-back.c dis.h coredis.c
	cc -o nv50dis nv50dis.c nv50d-back.c coredis.c

nvc0dis: nvc0dis.c nvc0d-back.c dis.h coredis.c
	cc -o nvc0dis nvc0dis.c nvc0d-back.c coredis.c

ctxdis: ctxdis.c ctxd-back.c dis.h coredis.c
	cc -o ctxdis ctxdis.c ctxd-back.c coredis.c

fucdis: fucdis.c fucd-back.c dis.h coredis.c
	cc -o fucdis fucdis.c fucd-back.c coredis.c

pmdis: pmdis.c pmd-back.c dis.h coredis.c
	cc -o pmdis pmdis.c pmd-back.c coredis.c

vp2dis: vp2dis.c vp2d-back.c dis.h coredis.c
	cc -o vp2dis vp2dis.c vp2d-back.c coredis.c
