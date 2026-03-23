cmake_minimum_required(VERSION 3.16)
project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CHESS_ENGINE_SOURCES
    src/position/array_board.cpp
    src/position/position.cpp
    src/position/pointer_board.cpp
    src/position/piece.cpp
    src/position/bitboard.cpp
    src/position/board_factory.cpp
    src/rules/rules.cpp
    src/movegen/movegen.cpp
    src/evaluation/eval.cpp
    src/search/search.cpp
    src/render/render.cpp
)

add_executable(chess_engine
    src/scripts/main.cpp
    ${CHESS_ENGINE_SOURCES}
)

add_executable(chess_benchmark
    src/scripts/benchmark_main.cpp
    ${CHESS_ENGINE_SOURCES}
)

target_include_directories(chess_engine PRIVATE src)
target_include_directories(chess_benchmark PRIVATE src)

# --- Google Test ---
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(chess_tests
    tests/test_main.cpp
    tests/test_position.cpp
    tests/test_movegen.cpp
    tests/test_rules.cpp
    tests/test_sequences.cpp
    ${CHESS_ENGINE_SOURCES}
)

target_include_directories(chess_tests PRIVATE src tests)
target_link_libraries(chess_tests GTest::gtest GTest::gmock)

include(GoogleTest)
gtest_discover_tests(chess_tests)
