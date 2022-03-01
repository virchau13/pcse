project(pcse CXX)
cmake_minimum_required(VERSION 3.10.3)
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		add_link_options(-lstdc++ -static-libgcc -static-libstdc++)
	else()
		add_link_options(-static)
	endif()
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug") 
    add_compile_options(-ggdb)
endif()

if(MSVC)
	add_compile_options(/W4 /WX)
else()
	add_compile_options(-Wall -Wextra -Wpedantic -Wno-class-memaccess -Werror=return-type)
endif()

# tests
find_package(Catch2)
if(Catch2_FOUND)
	add_executable(tests EXCLUDE_FROM_ALL test/tests-main.cpp test/lexer.test.cpp test/utils.test.cpp test/fraction.test.cpp test/parser.test.cpp test/interpreter.test.cpp)
	target_link_libraries(tests Catch2::Catch2)
endif()

# main
add_executable(pcse src/main.cpp)
