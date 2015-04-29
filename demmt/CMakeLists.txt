project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

add_executable(mmt_bin2dedma mmt_bin2dedma.c mmt_bin2dedma_nvidia.c mmt_bin_decode.c mmt_bin_decode_nvidia.c)
add_executable(demmt
	buffer.c
	buffer_decode.c
	config.c
	decode_utils.c
	drm.c
	fglrx.c
	macro.c
	main.c
	mmt_bin_decode.c
	mmt_bin_decode_nvidia.c
	nvrm.c
	nvrm_decode_ioctl.c
	nvrm_decode_mthd.c
	nvrm_decode_query.c
	object_state.c
	object_g80_2d.c
	object_g80_3d.c
	object_g80_m2mf.c
	object_gf100_2d.c
	object_gf100_3d.c
	object_gf100_m2mf.c
	object_gk104_3d.c
	object_gk104_compute.c
	object_gk104_copy.c
	object_gk104_p2mf.c
	pushbuf.c
	region.c
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBDRM_NV libdrm_nouveau)
if (PC_LIBDRM_NV_FOUND)
	include_directories(${PC_LIBDRM_NV_INCLUDE_DIRS})
	add_definitions(-DLIBDRM_AVAILABLE)
else (PC_LIBDRM_NV_FOUND)
	message("Warning: demmt won't parse nouveau ioctls because libdrm_nouveau was not found")
endif (PC_LIBDRM_NV_FOUND)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSECCOMP libseccomp)
if (LIBSECCOMP_FOUND)
	include_directories(${LIBSECCOMP_INCLUDE_DIRS})
	add_definitions(-DLIBSECCOMP_AVAILABLE)
else (LIBSECCOMP_FOUND)
	message("Warning: demmt won't sandbox itself because libseccomp was not found")
endif (LIBSECCOMP_FOUND)

target_link_libraries(demmt rnn envy ${LIBSECCOMP_LIBRARIES})

install(TARGETS demmt mmt_bin2dedma
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
