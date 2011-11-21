project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

include_directories(../include)

add_library(vstream bitstream.c)

add_executable(vstest vstest.c)

target_link_libraries(vstest vstream)

install(TARGETS vstest vstream
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
