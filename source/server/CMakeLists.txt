add_library(server server.c)
target_include_directories(server INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(server readconfig mimetable queue)