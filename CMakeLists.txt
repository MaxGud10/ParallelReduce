cmake_minimum_required(VERSION 3.20)

project(ParallelReduce LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MY_REDUCE_ENABLE_TESTS "Build tests" ON)
option(MY_REDUCE_ENABLE_WARNINGS "Enable warnings" ON)
option(MY_REDUCE_ENABLE_SANITIZERS "Enable sanitizers" OFF)

include(CTest)

set(CONAN_GENERATORS_DIR
    "${CMAKE_SOURCE_DIR}/third_party/build/${CMAKE_BUILD_TYPE}/generators"
)

if(EXISTS "${CONAN_GENERATORS_DIR}")
    list(PREPEND CMAKE_PREFIX_PATH "${CONAN_GENERATORS_DIR}")
endif()

find_package(Threads REQUIRED)

set(MY_REDUCE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(MY_REDUCE_SRC_DIR     ${CMAKE_SOURCE_DIR}/src)
set(MY_REDUCE_TESTS_DIR   ${CMAKE_SOURCE_DIR}/tests)
set(MY_REDUCE_BENCH_DIR   ${CMAKE_SOURCE_DIR}/bench)

file(MAKE_DIRECTORY ${MY_REDUCE_BENCH_DIR}/data)
file(MAKE_DIRECTORY ${MY_REDUCE_BENCH_DIR}/reports)

add_library(my_reduce_headers INTERFACE)
target_include_directories(my_reduce_headers
    INTERFACE
        ${MY_REDUCE_INCLUDE_DIR}
)

function(my_reduce_apply_warnings target_name)
    if(NOT MY_REDUCE_ENABLE_WARNINGS)
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Woverloaded-virtual
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
        )
    elseif(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
        )
    endif()
endfunction()

function(my_reduce_apply_sanitizers target_name)
    if(NOT MY_REDUCE_ENABLE_SANITIZERS)
        return()
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target_name} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target_name} PRIVATE
                -fsanitize=address,undefined
            )
        endif()
    endif()
endfunction()


add_executable(reduce_run
    ${MY_REDUCE_SRC_DIR}/main.cpp
)

target_link_libraries(reduce_run PRIVATE my_reduce_headers Threads::Threads)
my_reduce_apply_warnings(reduce_run)
my_reduce_apply_sanitizers(reduce_run)

add_executable(reduce_bench
    ${MY_REDUCE_SRC_DIR}/bench.cpp
)
target_link_libraries(reduce_bench PRIVATE my_reduce_headers Threads::Threads)
my_reduce_apply_warnings(reduce_bench)
my_reduce_apply_sanitizers(reduce_bench)

if(MY_REDUCE_ENABLE_TESTS)
    add_subdirectory(tests)
endif()