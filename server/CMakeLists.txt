cmake_minimum_required(VERSION 3.15)
project(practica1 C)

set(CMAKE_C_STANDARD 11)

find_package(Threads)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak -g")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -g")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -g")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory -g")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_executable(practica1 main.c httputils.c httputils.h readconfig.c readconfig.h
        constants.h picohttpparser.c picohttpparser.h server.c server.h colorcodes.h
        uthash.h queue.c queue.h mimetable.c mimetable.h)
target_link_libraries(practica1 ${CMAKE_THREAD_LIBS_INIT})

add_executable(queue_test queue.c queue.h test/queue_test.c)

add_executable(mimetable_test mimetable.c mimetable.h test/mimetable_test.c)

#add_executable(readconfig_test readconfig_test.c readconfig.c readconfig.h uthash.h)
