project(UDPTest CXX)

file(GLOB_RECURSE sourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DUDPTEST_DEBUG")

add_executable(UDPTest ${sourceFiles})

target_include_directories(UDPTest
	PUBLIC ${CMAKE_SOURCE_DIR}/extern/cxxopts/include/)

target_link_libraries(UDPTest
	PUBLIC spdlog::spdlog
	PUBLIC libUDPTest)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	target_link_libraries(UDPTest PUBLIC Threads::Threads)
endif()