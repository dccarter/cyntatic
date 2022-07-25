CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
set(CSTAR_VERSION 0.1.0 CACHE STRING "The version of cync")

set(CMAKE_C_COMPILER "gcc-11")
set(CMAKE_CXX_COMPILER "g++-11")

project(cync
        VERSION ${CSTAR_VERSION}
        DESCRIPTION "C-star is a compiler for a language adding syntatic sugar to C"
        LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-writable-strings")

if (NOT CMAKE_BUILD_TYPE)
    # default build rdt is Debug
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

option(ENABLE_UNIT_TESTS    "Enable building of unit tests" ON)
set(CYN_VM_VERSION 0.1.0 CACHE STRING "The virtual machine version")
set(CYN_ASSEMBLER_VERSION 0.1.0 CACHE STRING "The assembler version")

# Configure path for loading project cmake scripts
set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include_directories(
        include
        ${CMAKE_CURRENT_BINARY_DIR})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions("-DCYN_DEBUG_TRACE=0"
                    "-DCYN_DEBUG_EXEC_TRACE=1"
                    "-DCYN_DEBUG_HEAP_TRACE=1")
endif()

set(CYN_COMMON_C_SOURCES
        src/allocator.c
        src/args.c
        src/buffer.c
        src/common.c
        src/map.c
        src/vector.c)

set(CYN_COMPILER_SOURCES
        src/compiler/heap.c
        src/compiler/log.c
        src/compiler/source.c)

set(CYN_VM_SOURCES
        src/vm/code.c
        src/vm/memory.c
        src/vm/builtins.c
        src/vm/utils.c
        src/vm/vm.c)

add_executable(cync
        ${CYN_COMPILER_SOURCES}
        ${CYN_COMMON_C_SOURCES}
        src/compiler/main.c)

add_executable(cynvm
        ${CYN_COMMON_C_SOURCES}
        ${CYN_VM_SOURCES}
        src/vm/main.c)

target_compile_definitions(cynvm PRIVATE
        -DCYN_APPLICATION_NAME=\"cynvm\"
        -DCYN_APPLICATION_VERSION=\"${CYN_VM_VERSION}\")

add_executable(cynas
        ${CYN_COMMON_C_SOURCES}
        ${CYN_VM_SOURCES}
        ${CYN_LEXER_SOURCES}
        src/asm/asm.cpp
        src/asm/main.c)

target_compile_definitions(cynas PRIVATE
        -DCYN_APPLICATION_NAME=\"cynas\"
        -DCYN_APPLICATION_VERSION=\"${CYN_ASSEMBLER_VERSION}\")

include_directories(include)

if (ENABLE_UNIT_TESTS)
    add_executable(cync-unit-test
            tests/main.cpp
            tests/shared.cpp)

    target_include_directories(cync-unit-test PRIVATE src)
    target_compile_definitions(cync-unit-test PRIVATE
            -DCYN_UNIT_TEST=1)
endif()
