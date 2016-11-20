project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

add_library(envybios
	bios.c
	bit.c
	conn.c
	d.c
	dunk.c
	D.c
	dacload.c
	dcb.c
	extdev.c
	gpio.c
	i2c.c
	i2cscript.c
	info.c
	iunk.c
	L.c
	mem.c
	mux.c
	power.c
	print.c
	T.c
)

add_executable(nvbios nvbios.c)

target_link_libraries(nvbios envy envybios)

install(TARGETS nvbios envybios
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
