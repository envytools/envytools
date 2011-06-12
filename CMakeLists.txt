project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(LibXml2 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

pkg_check_modules(PC_PCIACCESS pciaccess REQUIRED)

include_directories(${LIBXML2_INCLUDE_DIR})

flex_target(envylex envylex.l ${CMAKE_CURRENT_BINARY_DIR}/envylex.c)
bison_target(envyparse envyparse.y ${CMAKE_CURRENT_BINARY_DIR}/envyparse.c)
add_flex_bison_dependency(envylex envyparse)
include_directories(.)

add_library(rnn rnn.c rnndec.c)
add_library(nva nva.c)
add_library(envy coredis.c nv50dis.c nvc0dis.c ctxdis.c fucdis.c pmsdis.c vp2dis.c vp3mdis.c macrodis.c envyprint.c)

add_executable(envydis envydis.c)
add_executable(demmio demmio.c)
add_executable(headergen headergen.c)
add_executable(nvbios nvbios.c)
add_executable(dedma dedma.c dedma_cache.c dedma_back.c)
add_executable(envyas envyas.c ${FLEX_envylex_OUTPUTS} ${BISON_envyparse_OUTPUTS})
add_executable(nvalist nvalist.c)
add_executable(nvapeek nvapeek.c)
add_executable(nvapoke nvapoke.c)
add_executable(nvapeek8 nvapeek8.c)
add_executable(nvapoke8 nvapoke8.c)
add_executable(nvagetbios nvagetbios.c)
add_executable(nvafakebios nvafakebios.c)
add_executable(nvascan nvascan.c)
add_executable(nvatiming nvatiming.c)

target_link_libraries(envydis envy)
target_link_libraries(envyas envy)
target_link_libraries(demmio envy rnn ${LIBXML2_LIBRARIES})
target_link_libraries(headergen rnn ${LIBXML2_LIBRARIES})
target_link_libraries(dedma rnn ${LIBXML2_LIBRARIES})
target_link_libraries(nvalist nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeek nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapoke nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeek8 nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapoke8 nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvagetbios nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvafakebios nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvascan nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvatiming nva ${PC_PCIACCESS_LIBRARIES})

install(TARGETS envydis demmio headergen nvbios envy rnn envyas dedma nva nvalist nvapeek nvapoke nvapeek8 nvapoke8 nvagetbios nvafakebios
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
