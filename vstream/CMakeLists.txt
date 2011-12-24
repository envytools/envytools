project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

include_directories(../include)

add_library(vstream bitstream.c h264.c h264_slice.c h264_residual.c h264_se.c h264_cabac.c h264_cavlc.c h264_print.c)

add_executable(vstest vstest.c)
add_executable(predtest predtest.c)
add_executable(deh264 deh264.c)

target_link_libraries(vstest vstream)
target_link_libraries(predtest vstream)
target_link_libraries(deh264 vstream)

install(TARGETS vstest vstream deh264
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
