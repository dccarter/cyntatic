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
    add_definitions("-DCYN_VM_BUILD_DEBUG=0")
endif()

set(CYN_COMMON_SOURCES
        src/allocator.c
        src/args.c
        src/buffer.c
        src/common.c
        src/e4c.c
        src/file.c
        src/map.c
        src/stream.c
        src/tree.c
        src/vector.c)

add_library(cyn-utils STATIC
        ${CYN_COMMON_SOURCES})

set(CYN_COMPILER_COMMON_SOURCES
        src/compiler/common/init.c
        src/compiler/common/itp.c
        src/compiler/common/heap.c
        src/compiler/common/lexer.c
        src/compiler/common/log.c
        src/compiler/common/source.c
        src/compiler/common/token.c
        src/compiler/common/timer.c
        src/compiler/common/ident.c)

add_library(cync-common STATIC
        ${CYN_COMPILER_COMMON_SOURCES})

set(CYN_VM_SOURCES
        src/vm/code.c
        src/vm/memory.c
        src/vm/builtins.c
        src/vm/utils.c
        src/vm/vm.c)

add_library(cynvm-lib
        ${CYN_VM_SOURCES})

add_executable(cync
        src/compiler/codegen.c
        src/compiler/parser.c
        src/compiler/type.c
        src/compiler/main.c)

target_link_libraries(cync  cyn-utils cync-common)

add_executable(cynvm
        src/vm/main.c)

target_link_libraries(cynvm cynvm-lib cyn-utils)
target_compile_definitions(cynvm PRIVATE
        -DCYN_APPLICATION_NAME=\"cynvm\"
        -DCYN_APPLICATION_VERSION=\"${CYN_VM_VERSION}\")

add_executable(cynas
        src/compiler/asm/asm.c
        src/compiler/asm/main.c)
target_link_libraries(cynas cync-common cynvm-lib cyn-utils)

target_compile_definitions(cynas PRIVATE
        -DCYN_APPLICATION_NAME=\"cynas\"
        -DCYN_APPLICATION_VERSION=\"${CYN_ASSEMBLER_VERSION}\"
        -DCYN_VM_BUILD_TOOL=1)

include_directories(include)

if (ENABLE_UNIT_TESTS)
    add_executable(cync-unit-test
            tests/main.cpp
            tests/shared.cpp)

    target_include_directories(cync-unit-test PRIVATE src)
    target_compile_definitions(cync-unit-test PRIVATE
            -DCYN_UNIT_TEST=1)
    set_target_properties(cync-unit-test PROPERTIES COMPILE_FLAGS "-fpermissive")
endif()
