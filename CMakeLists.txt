cmake_minimum_required(VERSION 3.9)
project(Chat_MultiLevel C)
set(CMAKE_C_STANDARD 11)
find_package(Threads)
find_package(Curses REQUIRED)

#Bring the headers, such as Student.h into the project
include_directories(src include)

#However, the file(GLOB...) allows for wildcard additions:

file(GLOB HEADER include/*)
file(GLOB HEADER_TREE treeFunx/include/*)

file(GLOB SOURCE src/*)
file(GLOB SOURCE_TREE treeFunx/src/*)

set(MAIN_DIR_INCLUDE globalSet.h helpFunx.h helpFunx.c)


add_executable(Chat_MultiLevel main.c ${MAIN_DIR_INCLUDE} ${HEADER} ${HEADER_TREE} ${SOURCE} ${SOURCE_TREE})

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})




