cmake_minimum_required(VERSION 3.12)
enable_testing()

#set(CMAKE_CXX_FLAGS "-std=c++17")

set(CMAKE_BUILD_TYPE RelWithDebInfo)
#set(CMAKE_BUILD_TYPE Debug)

# set(CMAKE_EXPORT_COMPILE_COMMANDS true)
include_directories(./include)

find_package(GTest REQUIRED)

# Unit test
set(SOURCES
	tests/pint_test.cpp
)

add_executable(pint_test ${SOURCES})

target_link_libraries(
	pint_test
		PRIVATE GTest::Main GTest::GTest
)

target_include_directories(
	pint_test
		PRIVATE ${GTest_INCLUDE_DIRS}
)

gtest_discover_tests(pint_test '' AUTO)

# Benchmarks

find_package(benchmark REQUIRED)
add_executable(pint_bench tests/pint_bench.cpp)
target_link_libraries(
	pint_bench
		PRIVATE benchmark::benchmark_main benchmark::benchmark
)
