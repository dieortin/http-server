add_library(httpserver httpserver.c)
target_include_directories(httpserver INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(httpserver server httputils)