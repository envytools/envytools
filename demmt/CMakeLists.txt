project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

add_executable(mmt_bin2dedma mmt_bin2dedma.c mmt_bin2dedma_nvidia.c mmt_bin_decode.c mmt_bin_decode_nvidia.c)
add_executable(demmt main.c mmt_bin_decode.c mmt_bin_decode_nvidia.c nvrm.c
	pushbuf.c object_state.c drm.c macro.c region.c buffer.c config.c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBDRM_NV libdrm_nouveau)
if (PC_LIBDRM_NV_FOUND)
	include_directories(${PC_LIBDRM_NV_INCLUDE_DIRS})
	add_definitions(-DLIBDRM_AVAILABLE)
else (PC_LIBDRM_NV_FOUND)
	message("Warning: demmt won't parse nouveau ioctls because libdrm_nouveau was not found")
endif (PC_LIBDRM_NV_FOUND)

target_link_libraries(demmt rnn envy)

install(TARGETS demmt mmt_bin2dedma
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
