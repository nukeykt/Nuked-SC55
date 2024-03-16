# message("Path to UTF8-Main is [${CMAKE_CURRENT_LIST_DIR}]")
include_directories(${CMAKE_CURRENT_LIST_DIR}/)

set(UTF8MAIN_SRCS)

if(WIN32)
    list(APPEND UTF8MAIN_SRCS
        ${CMAKE_CURRENT_LIST_DIR}/utf8main_win32.cpp
    )
endif()
