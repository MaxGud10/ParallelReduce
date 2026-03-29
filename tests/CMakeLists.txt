cmake_minimum_required(VERSION 3.20)

find_package(GTest REQUIRED CONFIG)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

include(GoogleTest)

add_executable(my_reduce_unit_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/sequential_reduce_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/partition_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/threshold_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/thread_pool_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/parallel_reduce_tests.cpp
)

target_include_directories(my_reduce_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(my_reduce_unit_tests PRIVATE
    my_reduce_headers
    GTest::gtest_main
    Threads::Threads
)

gtest_discover_tests(my_reduce_unit_tests)

add_test(
    NAME e2e_reduce_run_seq_int_small
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/e2e/run_reduce_e2e.py
            --bin $<TARGET_FILE:reduce_run>
            --input ${CMAKE_CURRENT_SOURCE_DIR}/e2e/fixtures/int_small.txt
            --mode seq
            --type int64
            --expected 15
)

add_test(
    NAME e2e_reduce_run_par_int_small
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/e2e/run_reduce_e2e.py
            --bin $<TARGET_FILE:reduce_run>
            --input ${CMAKE_CURRENT_SOURCE_DIR}/e2e/fixtures/int_small.txt
            --mode par
            --type int64
            --threads 4
            --expected 15
)

add_test(
    NAME e2e_reduce_bench_int
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/e2e/run_bench_e2e.py
            --bin $<TARGET_FILE:reduce_bench>
            --type int64
            --distribution uniform_int
            --size 10000
            --threads 4
            --repeats 2
            --warmup 1
)