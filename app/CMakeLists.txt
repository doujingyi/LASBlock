cmake_minimum_required (VERSION 3.0)
cmake_policy(VERSION 3.0)

project(LASBlock C CXX)

find_library(LASLIB libLASlib.a HINTS ${LASlib_LIBRARY})
find_package(Boost REQUIRED COMPONENTS system thread)

include_directories(${LASlib_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${PROJECT_NAME} ${Boost_LIBRARY_DIRS})

file(GLOB SOURCES "*.cpp" "*.h")
set(SRCS_FILES ${SOURCES})

add_executable (${PROJECT_NAME} ${SRCS_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC ${LASLIB})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})

