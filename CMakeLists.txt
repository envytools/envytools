project(ENVYTOOLS C CXX)
cmake_minimum_required(VERSION 2.6)
enable_testing()

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=gnu11")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++11")
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif (NOT CMAKE_BUILD_TYPE)
if (NOT DOC_PATH)
	set(DOC_PATH "share/doc/envytools/")
endif (NOT DOC_PATH)

include_directories(include)
include_directories(${CMAKE_BINARY_DIR}/include-generated)

add_subdirectory(util)
add_subdirectory(cgen)
add_subdirectory(easm)
add_subdirectory(nva)
add_subdirectory(rnn)
add_subdirectory(envydis)
add_subdirectory(nvbios)
add_subdirectory(vstream)
add_subdirectory(vdpow)
add_subdirectory(nvhw)
add_subdirectory(hwtest)
add_subdirectory(demmt)
add_subdirectory(cupti_trace)

# Get the list of hwdoc files to install them in a flat directory
file(GLOB_RECURSE HWDOCS_SRC docs/*)
# Get the full patch to .gitignore and remove it from the list
file(GLOB GITIGNORE docs/.gitignore)
if(EXISTS ${GITIGNORE})
	list(REMOVE_ITEM HWDOCS_SRC ${GITIGNORE})
endif()

set(RNNDB_BUILD_DIR ${CMAKE_BINARY_DIR}/rnndb-generated)

file(MAKE_DIRECTORY ${RNNDB_BUILD_DIR})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include-generated)

add_custom_command(OUTPUT ${RNNDB_BUILD_DIR}/nvchipsets.xml
		COMMAND python3
		ARGS -m ssot.gen_rnndb_nvchipsets ${RNNDB_BUILD_DIR}/nvchipsets.xml
		MAIN_DEPENDENCY ssot/gpu.py
		DEPENDS ssot/gen_rnndb_nvchipsets.py
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
add_custom_target(rnndb-generated ALL DEPENDS ${RNNDB_BUILD_DIR}/nvchipsets.xml)

install(DIRECTORY include/ DESTINATION include/envytools)
install(DIRECTORY ${CMAKE_BINARY_DIR}/include-generated/ DESTINATION include/envytools)
install(DIRECTORY rnndb DESTINATION share PATTERN ".gitignore" EXCLUDE)
install(DIRECTORY ${RNNDB_BUILD_DIR}/ DESTINATION share/rnndb)
install(FILES ${HWDOCS_SRC} DESTINATION ${DOC_PATH}/hwdocs)
install(FILES README.rst DESTINATION ${DOC_PATH})
install(FILES docs/envydis/index.rst DESTINATION ${DOC_PATH} RENAME README-envydis)

message("")
message("-- Summary --")
message("")
message("General:")
message("	C-flags:	'${CMAKE_C_FLAGS}' (-DCMAKE_C_FLAGS=\"...\")")
message("	CXX-flags:	'${CMAKE_CXX_FLAGS}' (-DCMAKE_CXX_FLAGS=\"...\")")
message("	Build type:	${CMAKE_BUILD_TYPE} (-DCMAKE_BUILD_TYPE=\"...\")")
message("")
message("Install:")
message("	Prefix:		'${CMAKE_INSTALL_PREFIX}' (-DCMAKE_INSTALL_PREFIX=...)")
message("	Doc path:	'${DOC_PATH}' (-DDOC_PATH=...)")
message("	Python:		'${NVAPY_PATH}' (-DNVAPY_PATH)")
message("")
