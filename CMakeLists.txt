cmake_minimum_required(VERSION 3.13)
project(legacynes)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions("-DCURRENT_COMMIT=${GIT_COMMIT_HASH}")
add_definitions("-DCURRENT_BRANCH=${GIT_BRANCH}")

add_executable(legacynes
        src/APU.cpp
        src/APU.h
        src/Cartridge.h
        src/CPU6502.cpp
        src/CPU6502.h
        src/CPUInstructions.h
        src/EntryPoint.cpp
        src/InputManager.cpp
        src/InputManager.h
        src/MainSystem.cpp
        src/MainSystem.h
        src/MemoryManager.cpp
        src/MemoryManager.h
        src/MemoryMappers.cpp
        src/PPU.cpp
        src/PPU.h
        src/ProjectInfo.h)

find_package(SFML 2.5.1 REQUIRED audio graphics window system )

if (SFML_FOUND)
    set(SFML_LIBRARIES sfml-audio sfml-graphics sfml-window sfml-system)
    target_link_libraries(legacynes ${SFML_LIBRARIES})
else()
    message(SFML NOT FOUND)
endif()
