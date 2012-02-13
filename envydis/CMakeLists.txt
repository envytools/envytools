project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

include_directories(../include)

flex_target(lex lex.l ${CMAKE_CURRENT_BINARY_DIR}/lex.c)
bison_target(parse parse.y ${CMAKE_CURRENT_BINARY_DIR}/parse.c)
add_flex_bison_dependency(lex parse)
include_directories(.)

add_library(envy core.c nv50.c nvc0.c ctx.c fuc.c hwsq.c vp2.c vuc.c macro.c vp1.c print.c)

add_executable(envydis envydis.c)
add_executable(envyas envyas.c ${FLEX_lex_OUTPUTS} ${BISON_parse_OUTPUTS})

target_link_libraries(envydis envy)
target_link_libraries(envyas envy)

install(TARGETS envydis envy envyas
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
