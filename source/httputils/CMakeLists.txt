add_library(httputils httputils.c)
target_include_directories(httputils INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(httputils server picohttpparser)