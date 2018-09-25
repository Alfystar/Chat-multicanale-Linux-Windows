cmake_minimum_required(VERSION 3.9)
project(Chat_MultiLevel C)
set(CMAKE_C_STANDARD 11)
find_package(Threads)
find_package(Curses REQUIRED)

#Bring the headers, such as Student.h into the project
include_directories(src include)

#However, the file(GLOB...) allows for wildcard additions:
#file(GLOB HEADER "include/*.h")
file(GLOB SOURCES "src/*.c")

#file(GLOB HEADER "*.h")
file(GLOB SOURCES "*.c")

add_executable(Chat_MultiLevel main.c src/fileSystemUtylity.c include/fileSystemUtylity.h helpFunx.c helpFunx.h src/terminalShell.c include/terminalShell.h)
#add_executable(Chat_MultiLevel main.c ${HEADER} ${SOURCES})


target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})




