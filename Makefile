headergen: headergen.c rnn.c rnn.h
	gcc -o headergen headergen.c rnn.c -lxml2 -I/usr/include/libxml2 -g
