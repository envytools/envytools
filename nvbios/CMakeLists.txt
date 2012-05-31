project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

include_directories(../include)

add_library(bios bios.c print.c dcb.c gpio.c)

add_executable(nvbios nvbios.c)

target_link_libraries(nvbios envy bios)

install(TARGETS nvbios bios
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
