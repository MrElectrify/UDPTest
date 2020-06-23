cmake_minimum_required(VERSION 3.10)

project(UDPTest VERSION 0.01)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(extern)
add_subdirectory(src)
add_subdirectory(UDPTest)

find_package(Doxygen)
if (DOXYGEN_FOUND)
	add_subdirectory(docs)
else()
	message(STATUS "Doxygen not found, skipping docs")
endif()
