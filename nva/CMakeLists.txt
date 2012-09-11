project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_PCIACCESS pciaccess)
pkg_check_modules(PC_X11 x11)
pkg_check_modules(PC_XEXT xext)
find_package (Threads)

if (PC_PCIACCESS_FOUND)

include_directories(${PC_PCIACCESS_INCLUDE_DIRS})
link_directories(${PC_PCIACCESS_LIBRARY_DIRS})

add_library(nva nva.c regspace.c)
target_link_libraries(nva ${PC_PCIACCESS_LIBRARIES})

SET(NVA_PROGS
	nvalist
	nvapeek
	nvapeekstat
	nvapeeki
	nvapoke
	nvahammer
	nvafill
	nvagetbios
	nvafakebios
	nvascan
	nvatiming
	nvapms
	nvafuzz
	nvawatch
	nvacounter
	nvaevo
	nvastrscan
	nvaxttime
	nvaxtstart
	nvaxtpeek
	nvaxtpoke
	nvaxtrsr
	nvaxtssr
	nvaxtinsn
	nvavucstart
	nvafucstart
	nvaupload
	nvadownload
	evotiming
	nv01dump
	nv01post
   )

foreach(PROG ${NVA_PROGS})
	add_executable(${PROG} ${PROG}.c)
	target_link_libraries(${PROG} nva)
endforeach(PROG)

if (PC_X11_FOUND AND PC_XEXT_FOUND)
	add_executable(nvamemtiming nvamemtiming.c set_timings.c vbios_mgmt.c libXNVCtrl/NVCtrl.c)
	target_link_libraries(nvamemtiming nva ${PC_PCIACCESS_LIBRARIES} ${PC_X11_LIBRARIES} ${PC_XEXT_LIBRARIES})
	install(TARGETS nvamemtiming
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
else(PC_X11_FOUND AND PC_XEXT_FOUND)
	message("Warning: nvamemtiming won't be built because of un-met dependencies (libx11 and/or libxext)")
endif(PC_X11_FOUND AND PC_XEXT_FOUND)

target_link_libraries(nvawatch ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(nvacounter rt)
install(TARGETS nva ${NVA_PROGS}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})

install(FILES README DESTINATION share/doc/envytools RENAME README-nva)

else(PC_PCIACCESS_FOUND)
	message("Warning: nva won't be built because of un-met dependencies (pciaccess)")
endif(PC_PCIACCESS_FOUND)
