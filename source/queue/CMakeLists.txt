add_library(queue queue.c)
target_include_directories(queue INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(queue ${CMAKE_THREAD_LIBS_INIT})