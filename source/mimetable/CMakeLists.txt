add_library(mimetable mimetable.c)
target_include_directories(mimetable INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(mimetable uthash)