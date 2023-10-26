project(ENVYTOOLS C)
cmake_minimum_required(VERSION 3.5)

set(CGEN_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include-generated)

set(CGEN_ITEMS fb bus gpu)
set(CGEN_SOURCES "")
set(CGEN_HEADERS "")

foreach(ITEM ${CGEN_ITEMS})
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/cgen/${ITEM}.c ${CGEN_INCLUDE_DIR}/cgen/${ITEM}.h
		COMMAND python3
		ARGS -m ssot.gen_c ssot.${ITEM} ${CMAKE_BINARY_DIR} ${CGEN_INCLUDE_DIR}
		MAIN_DEPENDENCY ../ssot/${ITEM}.py
		DEPENDS ../ssot/cgen.py ../ssot/gen_c.py
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)
set(CGEN_SOURCES ${CGEN_SOURCES} ${CMAKE_BINARY_DIR}/cgen/${ITEM}.c)
set(CGEN_HEADERS ${CGEN_HEADERS} ${CGEN_INCLUDE_DIR}/cgen/${ITEM}.h)
endforeach(ITEM)

add_library(cgen ${CGEN_SOURCES})
set_property(TARGET cgen PROPERTY POSITION_INDEPENDENT_CODE ON)
