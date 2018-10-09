cmake_minimum_required(VERSION 3.9)
project(Chat_MultiLevel C)
set(CMAKE_C_STANDARD 11)
find_package(Threads)
find_package(Curses REQUIRED)

#Bring the headers, such as Student.h into the project
#include_directories(src include)

#However, the file(GLOB...) allows for wildcard additions:
#file(GLOB HEADER "include/*.h")
#file(GLOB SOURCES "src/*.c")

#file(GLOB HEADER "*.h")
#file(GLOB SOURCES "*.c")

#file(GLOB HEADER include/*)
file(GLOB HEADER treeFunx/include/*)
#set(HEADER helpFunx.h defineSets.h)

#file(GLOB SOURCE src/*)
file(GLOB SOURCE treeFunx/src/*)
#set(SOURCE helpFunx.c)  #fuori da ogni cartella

#add_executable(avlTree avl_Thread_Safe_Test.c ${HEADER} ${SOURCE})

set(SOURCE src/fileSystemUtylity.c src/terminalShell.c src/mexData.c src/tableFile.c helpFunx.c src/thFunx.c)
set(HEADER include/fileSystemUtylity.h include/terminalShell.h include/mexData.h include/tableFile.h helpFunx.h include/thFunx.h)

#add_executable(Chat_MultiLevel main.c src/fileSystemUtylity.c include/fileSystemUtylity.h src/terminalShell.c include/terminalShell.h src/mexData.c include/mexData.h src/tableFile.c include/tableFile.h helpFunx.c helpFunx.h )

add_executable(Chat_MultiLevel main.c ${HEADER} ${SOURCE})

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})




