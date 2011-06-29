project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

include_directories(../include)

flex_target(envylex envylex.l ${CMAKE_CURRENT_BINARY_DIR}/envylex.c)
bison_target(envyparse envyparse.y ${CMAKE_CURRENT_BINARY_DIR}/envyparse.c)
add_flex_bison_dependency(envylex envyparse)
include_directories(.)

add_library(envy coredis.c nv50dis.c nvc0dis.c ctxdis.c fucdis.c pmsdis.c vp2dis.c vp3mdis.c macrodis.c envyprint.c)

add_executable(envydis envydis.c)
add_executable(envyas envyas.c ${FLEX_envylex_OUTPUTS} ${BISON_envyparse_OUTPUTS})

target_link_libraries(envydis envy)
target_link_libraries(envyas envy)

install(TARGETS envydis envy envyas
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
