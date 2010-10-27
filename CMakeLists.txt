project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(LibXml2 REQUIRED)

include_directories(${LIBXML2_INCLUDE_DIR})

add_library(rnn rnn.c rnndec.c)
add_library(envy coredis.c nv50dis.c nvc0dis.c ctxdis.c fucdis.c pmsdis.c vp2dis.c vp3mdis.c vp3tdis.c macrodis.c)

add_executable(envydis envydis.c)
add_executable(demmio demmio.c)
add_executable(headergen headergen.c)
add_executable(nvbios nvbios.c)

target_link_libraries(envydis envy)
target_link_libraries(demmio envy rnn ${LIBXML2_LIBRARIES})
target_link_libraries(headergen rnn ${LIBXML2_LIBRARIES})

install(TARGETS envydis demmio headergen nvbios envy rnn
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
