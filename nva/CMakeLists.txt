project(ENVYTOOLS C)
cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_PCIACCESS pciaccess REQUIRED)

include_directories(../include)

add_library(nva nva.c)
add_executable(nvalist nvalist.c)
add_executable(nvapeek nvapeek.c)
add_executable(nvapoke nvapoke.c)
add_executable(nvapeek8 nvapeek8.c)
add_executable(nvapoke8 nvapoke8.c)
add_executable(nvagetbios nvagetbios.c)
add_executable(nvafakebios nvafakebios.c)
add_executable(nvascan nvascan.c)
add_executable(nvatiming nvatiming.c)
add_executable(nvapms nvapms.c)
add_executable(nvafuzz nvafuzz.c)

target_link_libraries(nvalist nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeek nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapoke nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapeek8 nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapoke8 nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvagetbios nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvafakebios nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvascan nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvatiming nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvapms nva ${PC_PCIACCESS_LIBRARIES})
target_link_libraries(nvafuzz nva ${PC_PCIACCESS_LIBRARIES})

install(TARGETS nva nvalist nvapeek nvapoke nvapeek8 nvapoke8 nvagetbios nvafakebios nvascan nvatiming nvapms
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib${LIB_SUFFIX}
	ARCHIVE DESTINATION lib${LIB_SUFFIX})
