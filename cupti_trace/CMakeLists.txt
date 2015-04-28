project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(CUDA)
find_program(VALGRIND_EXECUTABLE valgrind)

if (CUDA_FOUND AND VALGRIND_EXECUTABLE)
	add_executable(cupti_trace cupti_trace.c)
	target_link_libraries(cupti_trace rnn envyutil)

	configure_file(cupti_trace_path.h.in cupti_trace_path.h ESCAPE_QUOTES)
	include_directories(${PROJECT_BINARY_DIR})

	install(TARGETS cupti_trace
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib${LIB_SUFFIX}
		ARCHIVE DESTINATION lib${LIB_SUFFIX})
else (CUDA_FOUND AND VALGRIND_EXECUTABLE)
	message("warning: cupti_trace won't be built because of un-met "
		"dependencies (cuda and/or valgrind-mmt)")
endif (CUDA_FOUND AND VALGRIND_EXECUTABLE)
