project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

add_executable(vstest vstest.c)
add_executable(predtest predtest.c)
add_executable(test264 test264.c)

target_link_libraries(vstest vstream)
target_link_libraries(predtest vstream)
target_link_libraries(test264 vstream)

add_test(vstest ${CMAKE_CURRENT_BINARY_DIR}/vstest)
add_test(predtest ${CMAKE_CURRENT_BINARY_DIR}/predtest)
add_test(test264 ${CMAKE_CURRENT_BINARY_DIR}/test264)
