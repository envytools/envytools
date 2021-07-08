project(ENVYTOOLS C)
cmake_minimum_required(VERSION 3.5)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")

add_library(envy core.c core-as.c core-dis.c g80.c gf100.c gk110.c gm107.c ctx.c falcon.c hwsq.c xtensa.c vuc.c macro.c vp1.c vcomp.c sched.c sched-gm107.c)

add_executable(envydis envydis.c)
add_executable(envyas envyas.c)
add_executable(envysched envysched.c)

target_link_libraries(envy envyutil easm)
target_link_libraries(envydis envy)
target_link_libraries(envyas envy envyutil)
target_link_libraries(envysched envy envyutil)

install(TARGETS envydis envy envyas envysched
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})

add_subdirectory(test)
