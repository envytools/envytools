project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(LibXml2 REQUIRED)

find_path(LIBICONV_INCLUDE_DIR iconv.h)
include_directories(${LIBXML2_INCLUDE_DIR} ${LIBICONV_INCLUDE_DIR} ../include)

add_library(rnn rnn.c rnndec.c)

add_executable(demmio demmio.c)
add_executable(headergen headergen.c)
add_executable(dedma dedma.c dedma_cache.c dedma_back.c)
add_executable(lookup lookup.c)

target_link_libraries(demmio envy rnn ${LIBXML2_LIBRARIES})
target_link_libraries(headergen rnn ${LIBXML2_LIBRARIES})
target_link_libraries(dedma rnn ${LIBXML2_LIBRARIES})
target_link_libraries(lookup rnn ${LIBXML2_LIBRARIES})

install(TARGETS demmio headergen rnn dedma lookup
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
