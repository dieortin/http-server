add_library(readconfig readconfig.c)
target_include_directories(readconfig INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(readconfig uthash)