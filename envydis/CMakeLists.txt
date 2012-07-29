project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")

add_library(envy core.c core-as.c core-dis.c nv50.c nvc0.c ctx.c fuc.c hwsq.c vp2.c vuc.c macro.c vp1.c)

add_executable(envydis envydis.c)
add_executable(envyas envyas.c convert.c)

target_link_libraries(envy envyutil easm)
target_link_libraries(envydis envy)
target_link_libraries(envyas envy envyutil)

install(TARGETS envydis envy envyas
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
install(FILES README DESTINATION share/doc/envytools RENAME README-envydis)
