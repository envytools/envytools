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
add_executable(nvalist nvalist.c)
add_executable(nvapeek nvapeek.c)
add_executable(nvapeekstat nvapeekstat.c)
add_executable(nvapeeki nvapeeki.c)
add_executable(nvapoke nvapoke.c)
add_executable(nvahammer nvahammer.c)
add_executable(nvafill nvafill.c)
add_executable(nvagetbios nvagetbios.c)
add_executable(nvafakebios nvafakebios.c)
add_executable(nvascan nvascan.c)
add_executable(nvatiming nvatiming.c)
add_executable(nvapms nvapms.c)
add_executable(nvafuzz nvafuzz.c)
add_executable(nvawatch nvawatch.c)
add_executable(nvacounter nvacounter.c)
add_executable(nvaevo nvaevo.c)
add_executable(nvastrscan nvastrscan.c)
add_executable(nvaxttime nvaxttime.c)
add_executable(nvaxtstart nvaxtstart.c)
add_executable(nvaxtpeek nvaxtpeek.c)
add_executable(nvaxtpoke nvaxtpoke.c)
add_executable(nvaxtrsr nvaxtrsr.c)
add_executable(nvaxtssr nvaxtssr.c)
add_executable(nvaxtinsn nvaxtinsn.c)
add_executable(nvavucstart nvavucstart.c)
add_executable(nvaupload nvaupload.c)
add_executable(nvadownload nvadownload.c)
add_executable(evotiming evotiming.c)
add_executable(nv01dump nv01dump.c)
add_executable(nv01post nv01post.c)

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

target_link_libraries(nvalist nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeek nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeekstat nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapoke nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvahammer nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvafill nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeeki nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvagetbios nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvafakebios nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvascan nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvatiming nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapms nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvafuzz nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvawatch nva ${PC_PCIACCESS_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(nvacounter nva ${PC_PCIACCESS_LIBRARIES} rt)
target_link_libraries(nvaevo nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvastrscan nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvaxttime nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvaxtstart nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvaxtpeek nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvaxtpoke nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvaxtrsr nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvaxtssr nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvavucstart nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvaxtinsn nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvaupload nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvadownload nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(evotiming nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nv01dump nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nv01post nva ${PC_PCIACCESS_LIBRARIES})

install(TARGETS nva
		nvalist
		nvapeek
		nvapeekstat
		nvapoke
		nvahammer
		nvafill
		nvapeeki
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
		nvaupload
		nvadownload
		evotiming
		nv01dump
		nv01post
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})

else(PC_PCIACCESS_FOUND)
	message("Warning: nva won't be built because of un-met dependencies (pciaccess)")
endif(PC_PCIACCESS_FOUND)
