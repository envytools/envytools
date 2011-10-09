project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

include_directories(../include)

flex_target(ed2a_lex ed2a_lex.l ${CMAKE_CURRENT_BINARY_DIR}/ed2a_lex.c)
bison_target(ed2a_parse ed2a_parse.y ${CMAKE_CURRENT_BINARY_DIR}/ed2a_parse.c)
add_flex_bison_dependency(ed2a_lex ed2a_parse)

flex_target(ed2i_lex ed2i_lex.l ${CMAKE_CURRENT_BINARY_DIR}/ed2i_lex.c)
bison_target(ed2i_parse ed2i_parse.y ${CMAKE_CURRENT_BINARY_DIR}/ed2i_parse.c)
add_flex_bison_dependency(ed2i_lex ed2i_parse)

include_directories(.)

add_library(envy core.c nv50.c nvc0.c ctx.c fuc.c hwsq.c vp2.c vp3m.c macro.c print.c convert.c)
add_library(ed2
	${FLEX_ed2a_lex_OUTPUTS} ${BISON_ed2a_parse_OUTPUTS} ed2a_funcs.c ed2a_print.c
	${FLEX_ed2i_lex_OUTPUTS} ${BISON_ed2i_parse_OUTPUTS} ed2i_pre.c ed2i_funcs.c
	ed2s_hash.c ed2s_symtab.c
	ed2v_funcs.c
	ed2_misc.c ed2_parse.c)

add_executable(envydis envydis.c)
add_executable(envyas envyas.c)
add_executable(ed2acat ed2acat.c)
add_executable(ed2itest ed2itest.c)

target_link_libraries(envy ed2)
target_link_libraries(envydis envy)
target_link_libraries(envyas envy)
target_link_libraries(ed2acat envy)
target_link_libraries(ed2itest ed2)

install(TARGETS envydis envy envyas
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
