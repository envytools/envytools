project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_PCIACCESS pciaccess)

if (PC_PCIACCESS_FOUND)

SET(HWTEST_PROGS
	nv10_tile
	nv20_tile
)

add_library(hwtest vram.c common.c)
target_link_libraries(hwtest nva)

foreach(PROG ${HWTEST_PROGS})
	add_executable(${PROG} ${PROG}.c)
	target_link_libraries(${PROG} hwtest)
endforeach(PROG)

install(TARGETS ${HWTEST_PROGS}
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})

else(PC_PCIACCESS_FOUND)
	message("Warning: hwtest won't be built because of un-met dependencies (pciaccess)")
endif(PC_PCIACCESS_FOUND)
