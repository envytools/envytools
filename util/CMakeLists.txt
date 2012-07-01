project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

include_directories(../include)
include_directories(.)

add_library(envyutil path.c)
