project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

add_library(envybios
	bios.c print.c
	bit.c info.c dacload.c iunk.c
	i2cscript.c
	dcb.c dunk.c i2c.c gpio.c extdev.c conn.c mem.c mux.c power.c
	D.c L.c T.c
)

add_executable(nvbios nvbios.c)

target_link_libraries(nvbios envy envybios)

install(TARGETS nvbios envybios
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
