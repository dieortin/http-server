cmake_minimum_required(VERSION 3.13)
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

include_directories(core/include)

add_subdirectory(httputils)

add_subdirectory(httpserver)

add_subdirectory(picohttpparser)

add_subdirectory(mimetable)

add_subdirectory(queue)

add_subdirectory(readconfig)

add_subdirectory(server)

add_subdirectory(uthash)

add_executable(server-main core/src/main.c)
target_include_directories(server-main PUBLIC core/include httputils httpserver mimetable queue
        readconfig server uthash)
target_link_libraries(server-main ${CMAKE_THREAD_LIBS_INIT} httpserver)


add_executable(queue_test test/queue_test.c)
target_link_libraries(queue_test queue)

add_executable(mimetable_test test/mimetable_test.c)
target_link_libraries(mimetable_test mimetable)
