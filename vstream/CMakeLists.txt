project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

include_directories(../include)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_PCIACCESS pciaccess)
pkg_check_modules(PC_X11 x11)
pkg_check_modules(PC_VDPAU vdpau)

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

install(TARGETS vstest vstream deh261 deh262 deh264 test264
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})

if (PC_PCIACCESS_FOUND AND PC_X11_FOUND AND PC_VDPAU_FOUND)
	link_directories(${PC_PCIACCESS_LIBRARY_DIRS} ${PC_X11_LIBRARY_DIRS} ${PC_VDPAU_LIBRARY_DIRS})
	add_executable(vdpow vdpow.c)
	target_link_libraries(vdpow vstream nva ${PC_PCIACCESS_LIBRARIES} ${PC_X11_LIBRARIES}  ${PC_VDPAU_LIBRARIES} rt pthread)
	install(TARGETS vdpow RUNTIME DESTINATION bin)
endif (PC_PCIACCESS_FOUND AND PC_X11_FOUND AND PC_VDPAU_FOUND)
