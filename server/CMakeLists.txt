cmake_minimum_required(VERSION 3.15)
project(practica1 C)

set(CMAKE_C_STANDARD 11)

find_package(Threads)

add_executable(practica1 main.c httputils.c httputils.h readconfig.c readconfig.h
        constants.h picohttpparser.c picohttpparser.h server.c server.h colorcodes.h
        uthash.h queue.c queue.h)
target_link_libraries(practica1 ${CMAKE_THREAD_LIBS_INIT})

add_executable(queue_test queue.c queue.h queue_test.c)

#add_executable(readconfig_test readconfig_test.c readconfig.c readconfig.h uthash.h)
