all: headergen nv50_texture.h

headergen: headergen.c rnn.c rnn.h
	gcc -o headergen headergen.c rnn.c -lxml2 -I/usr/include/libxml2 -g

nv50_texture.h: nv50_texture.xml headergen
	./headergen nv50_texture.xml > nv50_texture.h

clean:
	rm -f headergen nv50_texture.h
