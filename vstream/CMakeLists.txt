project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")

include_directories(../include)

include_directories(${PC_PCIACCESS_INCLUDE_DIRS} ${PC_X11_INCLUDE_DIRS} ${PC_VDPAU_INCLUDE_DIRS} ../include)

add_library(vstream bitstream.c
	h264.c h264_slice.c h264_residual.c h264_print.c
	h264_cabac.c h264_cavlc.c h264_se.c
	h262.c h262_slice.c h262_print.c
	h261.c
)

add_executable(vstest vstest.c)
add_executable(predtest predtest.c)
add_executable(deh261 deh261.c)
add_executable(deh262 deh262.c)
add_executable(deh264 deh264.c)
add_executable(test264 test264.c)

target_link_libraries(vstest vstream)
target_link_libraries(predtest vstream)
target_link_libraries(deh261 vstream)
target_link_libraries(deh262 vstream)
target_link_libraries(deh264 vstream)
target_link_libraries(test264 vstream)

install(TARGETS vstream deh261 deh262 deh264 test264
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
