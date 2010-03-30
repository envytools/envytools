all: headergen expand lookup nv50_texture.h

headergen: headergen.c rnn.c rnn.h
	gcc -o headergen headergen.c rnn.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

expand: expand.c rnn.c rnn.h
	gcc -o expand expand.c rnn.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

lookup: lookup.c rnn.c rnn.h rnndec.c rnndec.h
	gcc -o lookup lookup.c rnn.c rnndec.c -lxml2 -I/usr/include/libxml2 -g -Wall -Wno-pointer-sign

nv50_texture.h: nv50_texture.xml headergen
	./headergen nv50_texture.xml > nv50_texture.h

clean:
	rm -f headergen nv50_texture.h
