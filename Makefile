all: ctxdis nv50dis nvc0dis fcdis pmdis

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

clean:
	rm -f ctxdis nv50dis nvc0dis fcdis pmdis
