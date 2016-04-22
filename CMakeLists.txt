cmake_minimum_required (VERSION 2.8)
project (MCRedis)

if(NOT CMAKE_BUILD_TYPE)
	message(FATAL_ERROR "Usage : cmake -DHIREDIS_PATH=path -CMAKE_BUILD_TYPE=[Debug|Release] .")
endif()

find_library(hiredisLib NAMES hiredis PATHS ${HIREDIS_PATH} NO_DEFAULT_PATH)
if(${hiredisLib} STREQUAL "hiredisLib-NOTFOUND")
	message("hiredis not found")
	message(FATAL_ERROR "Usage : cmake -DHIREDIS_PATH=path -CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} .")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -std=c++14")

message("Build mode : ${CMAKE_BUILD_TYPE}")
if(${CMAKE_BUILD_TYPE} STREQUAL Release)
	message("Compile flags : ${CMAKE_CXX_FLAGS_RELEASE}")
else()
	message("Compile flags : ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

include_directories("MCRedis")
include_directories(${HIREDIS_PATH})

file(GLOB src "./src/*.cpp")

add_library(${PROJECT_NAME} ${src})
