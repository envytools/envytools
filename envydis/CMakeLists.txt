project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

include_directories(../include)

flex_target(easm_lex easm_lex.l ${CMAKE_CURRENT_BINARY_DIR}/easm_lex.c)
bison_target(easm_parse easm_parse.y ${CMAKE_CURRENT_BINARY_DIR}/easm_parse.c)
add_flex_bison_dependency(easm_lex easm_parse)

flex_target(ed2i_lex ed2i_lex.l ${CMAKE_CURRENT_BINARY_DIR}/ed2i_lex.c)
bison_target(ed2i_parse ed2i_parse.y ${CMAKE_CURRENT_BINARY_DIR}/ed2i_parse.c)
add_flex_bison_dependency(ed2i_lex ed2i_parse)

include_directories(.)

add_library(ed2
	${FLEX_ed2i_lex_OUTPUTS} ${BISON_ed2i_parse_OUTPUTS} ed2i_pre.c ed2i_funcs.c
	ed2_misc.c)

add_library(envy core.c nv50.c nvc0.c ctx.c fuc.c hwsq.c vp2.c vuc.c macro.c vp1.c)

add_library(easm easm.c ${FLEX_easm_lex_OUTPUTS} ${BISON_easm_parse_OUTPUTS} easm_print.c)

add_executable(envydis envydis.c)
add_executable(envyas envyas.c convert.c)
add_executable(ed2ishow ed2ishow.c)

target_link_libraries(ed2 envyutil)
target_link_libraries(easm envyutil)
target_link_libraries(envy envyutil ed2 easm)
target_link_libraries(envydis envy)
target_link_libraries(envyas envy envyutil)
target_link_libraries(ed2ishow ed2)

install(TARGETS envydis envyas ed2ishow ed2 envy
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
