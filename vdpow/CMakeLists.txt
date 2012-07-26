project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

include_directories(../include)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_PCIACCESS pciaccess)
pkg_check_modules(PC_X11 x11)
pkg_check_modules(PC_VDPAU vdpau)

if (PC_PCIACCESS_FOUND AND PC_X11_FOUND AND PC_VDPAU_FOUND)
	link_directories(${PC_PCIACCESS_LIBRARY_DIRS} ${PC_X11_LIBRARY_DIRS} ${PC_VDPAU_LIBRARY_DIRS})
	add_executable(vdpow vdpow.c)
	target_link_libraries(vdpow vstream nva ${PC_PCIACCESS_LIBRARIES} ${PC_X11_LIBRARIES}  ${PC_VDPAU_LIBRARIES} rt pthread)
	install(TARGETS vdpow RUNTIME DESTINATION bin)
else (PC_PCIACCESS_FOUND AND PC_X11_FOUND AND PC_VDPAU_FOUND)
	message("Warning: vdpow won't be built because of un-met dependencies (libx11 and/or vdpau)")
endif (PC_PCIACCESS_FOUND AND PC_X11_FOUND AND PC_VDPAU_FOUND)
