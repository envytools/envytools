all: ctxdis nv50dis nv60dis

nv50dis: nv50dis.c
	cc -o nv50dis nv50dis.c

nv60dis: nv60dis.c
	cc -o nv60dis nv60dis.c

ctxdis: ctxdis.c
	cc -o ctxdis ctxdis.c

clean:
	rm -f ctxdis nv50dis nv60dis
