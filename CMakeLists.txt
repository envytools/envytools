project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

SET(CMAKE_C_FLAGS "-g -O0 -Wall -Wno-pointer-sign -Wno-missing-braces")

add_subdirectory(nva)
add_subdirectory(rnn)
add_subdirectory(envydis)
add_subdirectory(nvbios)
add_subdirectory(vstream)
