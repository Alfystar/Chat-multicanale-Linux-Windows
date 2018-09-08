cmake_minimum_required(VERSION 3.9)
project(Chat_MultiLevel C)
set(CMAKE_C_STANDARD 11)
add_executable(Chat_MultiLevel main.c fileSystemUtylity.c fileSystemUtylity.h helpFunx.c helpFunx.h)
find_package(Threads)
find_package(Curses REQUIRED)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})

