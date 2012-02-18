project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_PCIACCESS pciaccess REQUIRED)
pkg_check_modules(PC_X11 x11)
pkg_check_modules(PC_XEXT xext)

include_directories(${PC_PCIACCESS_INCLUDE_DIRS} ../include)

add_library(nva nva.c)
add_executable(nvalist nvalist.c)
add_executable(nvapeek nvapeek.c)
add_executable(nvapeekstat nvapeekstat.c)
add_executable(nvapeeki nvapeeki.c)
add_executable(nvapoke nvapoke.c)
add_executable(nvapeek8 nvapeek8.c)
add_executable(nvapoke8 nvapoke8.c)
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

if (PC_X11_FOUND AND PC_XEXT_FOUND)
	add_executable(nvamemtiming nvamemtiming.c set_timings.c vbios_mgmt.c libXNVCtrl/NVCtrl.c)
	target_link_libraries(nvamemtiming nva ${PC_PCIACCESS_LIBRARIES} ${PC_X11_LIBRARIES} ${PC_XEXT_LIBRARIES})
	install(TARGETS nvamemtiming
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
else(PC_X11_FOUND AND PC_XEXT_FOUND)
	message("Warning: nvamemtiming won't be build because of un-met dependencies (X11 and/or XEXT)")
endif(PC_X11_FOUND AND PC_XEXT_FOUND)

target_link_libraries(nvalist nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeek nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeekstat nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapoke nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeeki nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeek8 nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapoke8 nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvagetbios nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvafakebios nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvascan nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvatiming nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapms nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvafuzz nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvawatch nva ${PC_PCIACCESS_LIBRARIES})
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

install(TARGETS nva
		nvalist
		nvapeek
		nvapoke
		nvapeeki
		nvapeek8
		nvapoke8
		nvagetbios
		nvafakebios
		nvascan
		nvatiming
		nvapms
		nvafuzz
		nvawatch
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
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
