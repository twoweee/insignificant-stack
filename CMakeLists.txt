cmake_minimum_required(VERSION 3.16)

project(insubstantial-stack CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main
    src/main.cpp
    src/Args.cpp
    src/PacketReader.cpp
)

target_compile_options(main PRIVATE
    -Wall # common warnings
    -Wextra # extra warnings
    -Wpedantic # only standard things allowed
    -O2 # optimization lvl 2
)

# cmake -DCMAKE_BUILD_TYPE=Debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(main PRIVATE
        -g
        -fsanitize=address,undefined # runtime error detect
    )
    target_link_options(main PRIVATE
        -fsanitize=address,undefined
    )
endif()