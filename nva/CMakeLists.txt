project(ENVYTOOLS C)
cmake_minimum_required(VERSION 3.5)

option(DISABLE_NVA "Disable building nva tools" OFF)
if (NOT DISABLE_NVA)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(PC_PCIACCESS pciaccess)
	pkg_check_modules(PC_X11 x11)
	pkg_check_modules(PC_XEXT xext)
	pkg_search_module(PC_PYTHON python3 python-3.5)
	find_package (Threads)
	find_program (CYTHON_EXECUTABLE cython3)

	if (PC_PCIACCESS_FOUND)

		include_directories(${PC_PCIACCESS_INCLUDE_DIRS})
		link_directories(${PC_PCIACCESS_LIBRARY_DIRS})

		add_library(nva nva.c regspace.c)
		target_link_libraries(nva nvhw ${PC_PCIACCESS_LIBRARIES})

		SET(NVA_PROGS
			nvalist
			nvapeek
			nvapeekstat
			nvapeeki
			nvapoke
			nvahammer
			nvafill
			nvagetbios
			nvafakebios
			nvascan
			nvatiming
			nvapms
			nvafuzz
			nvawatch
			nvacounter
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
			nvafucstart
			nvagetpmu
			nvaupload
			nvadownload
			evotiming
			nv01dump
			nv01post
			nv03post
			nv01play
			nvaspyi2c
			nvammiotracereplay
			nvaforcetemp
		)

		foreach(PROG ${NVA_PROGS})
			add_executable(${PROG} ${PROG}.c)
			target_link_libraries(${PROG} nva)
		endforeach(PROG)

		if (PC_X11_FOUND AND PC_XEXT_FOUND)
			add_executable(nvamemtiming nvamemtiming.c set_timings.c vbios_mgmt.c libXNVCtrl/NVCtrl.c)
			target_link_libraries(nvamemtiming nva ${PC_PCIACCESS_LIBRARIES} ${PC_X11_LIBRARIES} ${PC_XEXT_LIBRARIES})
			install(TARGETS nvamemtiming
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib${LIB_SUFFIX}
			ARCHIVE DESTINATION lib${LIB_SUFFIX})
		else(PC_X11_FOUND AND PC_XEXT_FOUND)
			message("Warning: nvamemtiming won't be built because of un-met dependencies (libx11 and/or libxext)")
		endif(PC_X11_FOUND AND PC_XEXT_FOUND)

		if(PC_PYTHON_FOUND AND CYTHON_EXECUTABLE)
			include_directories(${PC_PYTHON_INCLUDE_DIRS})
			link_directories(${PC_PYTHON_LIBRARY_DIRS})
			add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nvapy.c
					   COMMAND ${CYTHON_EXECUTABLE}
					   ARGS -3 ${CMAKE_CURRENT_SOURCE_DIR}/nvapy.pyx -o ${CMAKE_CURRENT_BINARY_DIR}/nvapy.c
					   MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/nvapy.pyx)
			add_library(nvapy MODULE ${CMAKE_CURRENT_BINARY_DIR}/nvapy.c nva.c ../nvhw/chipset.c)
			set_target_properties(nvapy PROPERTIES PREFIX "")
			target_link_libraries(nvapy cgen)
			target_link_libraries(nvapy ${PC_PYTHON_LIBRARIES} ${PC_PCIACCESS_LIBRARIES})
			if (NVAPY_PATH)
				install(TARGETS nvapy DESTINATION ${NVAPY_PATH})
			endif (NVAPY_PATH)
		else(PC_PYTHON_FOUND AND CYTHON_EXECUTABLE)
			message("Warning: nvapy won't be built because of un-met dependencies (python3 and/or cython3)")
		endif(PC_PYTHON_FOUND AND CYTHON_EXECUTABLE)

		target_link_libraries(nvawatch ${CMAKE_THREAD_LIBS_INIT})
		target_link_libraries(nvacounter rt)
		install(TARGETS nva ${NVA_PROGS}
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib${LIB_SUFFIX}
			ARCHIVE DESTINATION lib${LIB_SUFFIX})

		install(FILES README DESTINATION ${DOC_PATH} RENAME README-nva)

	else(PC_PCIACCESS_FOUND)
		message("Warning: nva won't be built because of un-met dependencies (pciaccess)")
	endif(PC_PCIACCESS_FOUND)
endif (NOT DISABLE_NVA)
