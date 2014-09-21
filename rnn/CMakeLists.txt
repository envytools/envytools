project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

# <3 libxml2
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-sign")

find_package(LibXml2 REQUIRED)

find_path(LIBICONV_INCLUDE_DIR iconv.h)
include_directories(${LIBXML2_INCLUDE_DIR} ${LIBICONV_INCLUDE_DIR})

configure_file(rnn_path.h.in rnn_path.h ESCAPE_QUOTES)
include_directories(${PROJECT_BINARY_DIR})

add_library(rnn rnn.c rnndec.c)
add_library(seq seq.c)

add_executable(demmio demmio.c)
add_executable(headergen headergen.c)
add_executable(dedma dedma.c dedma_cache.c dedma_back.c)
add_executable(lookup lookup.c)
add_executable(rnncheck rnncheck.c)
add_executable(mmt_bin2dedma mmt_bin2dedma.c mmt_bin2dedma_nvidia.c mmt_bin_decode.c mmt_bin_decode_nvidia.c)
add_executable(demmt demmt.c mmt_bin_decode.c mmt_bin_decode_nvidia.c demmt_nvrm.c
	demmt_pushbuf.c demmt_objects.c demmt_drm.c demmt_macro.c demmt_region.c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBDRM_NV libdrm_nouveau)
if (PC_LIBDRM_NV_FOUND)
	include_directories(${PC_LIBDRM_NV_INCLUDE_DIRS})
	add_definitions(-DLIBDRM_AVAILABLE)
else (PC_LIBDRM_NV_FOUND)
	message("Warning: demmt won't parse nouveau ioctls because libdrm_nouveau was not found")
endif (PC_LIBDRM_NV_FOUND)

target_link_libraries(rnn ${LIBXML2_LIBRARIES} envyutil)
target_link_libraries(demmio envy nvhw rnn seq)
target_link_libraries(headergen rnn)
target_link_libraries(dedma rnn)
target_link_libraries(lookup rnn)
target_link_libraries(rnncheck rnn)
target_link_libraries(demmt rnn envy)

install(TARGETS demmio headergen rnn dedma lookup demmt mmt_bin2dedma
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})

add_test(check_rnndb rnncheck root.xml)
