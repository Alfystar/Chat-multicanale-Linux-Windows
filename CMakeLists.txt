cmake_minimum_required(VERSION 3.9)
project(Chat_MultiLevel C)
set(CMAKE_C_STANDARD 11)
find_package(Threads)
find_package(Curses REQUIRED)

#Bring the headers, such as Student.h into the project
include_directories(src include)

#However, the file(GLOB...) allows for wildcard additions:

#file(GLOB HEADER treeFunx/include/*)

#file(GLOB SOURCE src/*)
#file(GLOB SOURCE treeFunx/src/*)


set(SOURCE src/fileSystemUtylity.c src/terminalShell.c src/mexData.c src/tableFile.c helpFunx.c src/thFunx.c src/socketConnect.c treeFunx/src/avl.c treeFunx/src/dlist.c treeFunx/src/log.c treeFunx/src/queue.c treeFunx/src/stack.c treeFunx/src/tree.c)

set(HEADER include/fileSystemUtylity.h include/terminalShell.h include/mexData.h include/tableFile.h helpFunx.h include/thFunx.h include/socketConnect.h treeFunx/include/avl.h treeFunx/include/common.h treeFunx/include/dlist.h treeFunx/include/log.h treeFunx/include/queue.h treeFunx/include/stack.h treeFunx/include/tree.h)


add_executable(Chat_MultiLevel main.c globalSet.h ${HEADER} ${SOURCE})

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})




