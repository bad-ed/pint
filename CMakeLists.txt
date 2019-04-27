cmake_minimum_required(VERSION 3.12)
enable_testing()

#set(CMAKE_CXX_FLAGS "-std=c++17")

# set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)

# set(CMAKE_EXPORT_COMPILE_COMMANDS true)

find_package(GTest)

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
		PUBLIC ${GTest_INCLUDE_DIRS} ./include
)

gtest_add_tests(pint_test '' AUTO)
