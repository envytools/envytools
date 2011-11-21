project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

include_directories(../include)

add_library(vstream bitstream.c h264.c)

add_executable(vstest vstest.c)
add_executable(predtest predtest.c)

target_link_libraries(vstest vstream)
target_link_libraries(predtest vstream)

install(TARGETS vstest vstream
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
