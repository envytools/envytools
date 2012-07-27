project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(easm_lex easm_lex.l ${CMAKE_CURRENT_BINARY_DIR}/easm_lex.c)
bison_target(easm_parse easm_parse.y ${CMAKE_CURRENT_BINARY_DIR}/easm_parse.c)
add_flex_bison_dependency(easm_lex easm_parse)

include_directories(.)

add_library(easm easm.c ${FLEX_easm_lex_OUTPUTS} ${BISON_easm_parse_OUTPUTS} easm_print.c)

target_link_libraries(easm envyutil)

install(TARGETS easm
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
