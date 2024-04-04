cmake_minimum_required (VERSION 3.2...3.5)
project (NukedSC55 VERSION 0.1.1 LANGUAGES CXX)

include(FindPkgConfig)
include(GNUInstallDirs)
include(CheckCXXCompilerFlag)
include(CheckFunctionExists)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Prefer C++11 standard
set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

set(CMAKE_CXX_FLAGS "-O3 -g")

#===========================================================================================

function(nuked-sc55_find_SDL2)
    if(NOT TARGET ADLMIDI_SDL2)
        find_package(SDL2 REQUIRED)
        add_library(ADLMIDI_SDL2 INTERFACE)
        if(TARGET SDL2::SDL2)
            target_link_libraries(ADLMIDI_SDL2 INTERFACE SDL2::SDL2)
        else()
            string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)
            target_include_directories(ADLMIDI_SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
            target_link_libraries(ADLMIDI_SDL2 INTERFACE ${SDL2_LIBRARIES})
        endif()
    endif()
endfunction()

#===========================================================================================

if(NOT WIN32)
    check_function_exists(fstatat FILES_HAS_REALPATH)
    if(FILES_HAS_REALPATH)
        add_definitions(-DFILES_HAS_REALPATH)
    endif()
endif()

include(3rdparty/Utf8Main/utf8main.cmake)

find_package(SDL2 REQUIRED)

set(SC55_SRC
    src/mcu.cpp src/mcu.h # main() is here!
    src/lcd.cpp src/lcd.h src/lcd_font.h
    src/mcu_interrupt.cpp src/mcu_interrupt.h
    src/mcu_opcodes.cpp src/mcu_opcodes.h
    src/mcu_timer.cpp src/mcu_timer.h
    src/pcm.cpp src/pcm.h
    src/submcu.cpp src/submcu.h

    src/utils/files.cpp src/utils/files.h
)

add_library(nuked-sc55 STATIC ${SC55_SRC} ${UTF8MAIN_SRCS})

if(TARGET SDL2::SDL2)
    target_link_libraries(nuked-sc55 PRIVATE SDL2::SDL2)
else()
    string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)
    target_include_directories(nuked-sc55 PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(nuked-sc55 PRIVATE ${SDL2_LIBRARIES})
endif()

set(SC55_INSTALL_FILES)

# Copy ROM files into runtime directory
macro(copy_rom_target TargetName TargetFile)
    add_custom_target(sc55_copy_${TargetName} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${TargetFile}")
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TargetFile}"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/data/${TargetFile}"
                "${CMAKE_CURRENT_BINARY_DIR}/${TargetFile}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/${TargetFile}"
    )
    list(APPEND SC55_INSTALL_FILES ${CMAKE_CURRENT_BINARY_DIR}/${TargetFile})
endmacro()

copy_rom_target(back_data back.data)

install(TARGETS nuked-sc55
        EXPORT NukedSC55StaticTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

install(FILES ${SC55_INSTALL_FILES}
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/nuked-sc55")
