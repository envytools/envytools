all: ctxdis nv50dis nv60dis

nv50dis: nv50dis.c coredis.h coredis.c
	cc -o nv50dis nv50dis.c coredis.c

nv60dis: nv60dis.c coredis.h coredis.c
	cc -o nv60dis nv60dis.c coredis.c

ctxdis: ctxdis.c coredis.h coredis.c
	cc -o ctxdis ctxdis.c coredis.c

clean:
	rm -f ctxdis nv50dis nv60dis
