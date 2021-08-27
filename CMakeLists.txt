cmake_minimum_required(VERSION 3.21)
project(flappy)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
	RAYLIB
	GIT_REPOSITORY "https://github.com/raysan5/raylib"
	GIT_TAG 3.7.0
)
FetchContent_GetProperties(RAYLIB)
if (NOT raylib_POPULATED)
	FetchContent_Populate(RAYLIB)
	add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} flappy.cpp)
target_link_libraries(${PROJECT_NAME} raylib m)
target_include_directories(${PROJECT_NAME} PRIVATE ${raylib_SOURCE_DIRS}/include)

if (APPLE)
	target_link_libraries(${PROJECT_NAME} "-framework IOKit")
	target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
	target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()	

