cmake_minimum_required(VERSION 3.12)
enable_testing()

#set(CMAKE_CXX_FLAGS "-std=c++17")

set(CMAKE_BUILD_TYPE RelWithDebInfo)
#set(CMAKE_BUILD_TYPE Debug)

# set(CMAKE_EXPORT_COMPILE_COMMANDS true)
include_directories(PUBLIC ./include)

find_package(GTest)

# Unit test
set(SOURCES
	tests/pint_test.cpp
)

add_executable(pint_test ${SOURCES})

target_link_libraries(
	pint_test
		PUBLIC GTest::Main GTest::GTest
)

target_include_directories(
	pint_test
		PUBLIC ${GTest_INCLUDE_DIRS}
)

gtest_add_tests(pint_test '' AUTO)

# Benchmarks

add_executable(pint_bench tests/pint_bench.cpp)
