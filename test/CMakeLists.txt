cmake_minimum_required(VERSION 3.10)
project(ConcurrentMemoryPoolTest LANGUAGES CXX)

find_package(Threads REQUIRED)

add_executable(UnitTest
    UnitTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ThreadCache.cpp
)

target_compile_features(UnitTest PRIVATE cxx_std_11)

target_include_directories(UnitTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../head
)

target_link_libraries(UnitTest PRIVATE Threads::Threads)
