project(pint CXX)
cmake_minimum_required(VERSION 3.12)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/include)
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

if (NOT CMAKE_CROSSCOMPILING)
	gtest_discover_tests(pint_test '' AUTO)
endif()

# Benchmarks

find_package(benchmark REQUIRED)
add_executable(pint_bench tests/pint_bench.cpp)
target_link_libraries(
	pint_bench
		PRIVATE benchmark::benchmark_main benchmark::benchmark
)
