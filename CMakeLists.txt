cmake_minimum_required(VERSION 3.16)

set(LIBRARY_NAME
    patterns_lib
)

set(GLOBAL_NAMESPACE
    xitren
)

project(${LIBRARY_NAME}
	DESCRIPTION "Patterns library"
    VERSION 1.0.6
	LANGUAGES CXX
)

add_library(${LIBRARY_NAME} INTERFACE)
add_library(${GLOBAL_NAMESPACE}::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
target_compile_features(${LIBRARY_NAME} INTERFACE cxx_std_20)

target_include_directories(
        ${LIBRARY_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

include(FetchContent)
FetchContent_Declare(
		crc
		GIT_REPOSITORY https://github.com/xitren/crc.git
		GIT_TAG        v1.0.1
)
# Avoid pulling crc's own tests into this project (important for sanitizer builds and faster CI).
FetchContent_GetProperties(crc)
if (NOT crc_POPULATED)
    FetchContent_Populate(crc)
endif ()

add_library(crc_lib INTERFACE)
add_library(${GLOBAL_NAMESPACE}::crc_lib ALIAS crc_lib)
target_include_directories(crc_lib INTERFACE $<BUILD_INTERFACE:${crc_SOURCE_DIR}/include>)

target_link_libraries(${LIBRARY_NAME} INTERFACE ${GLOBAL_NAMESPACE}::crc_lib)

message(STATUS "Adding library project \"${LIBRARY_NAME}\"")

enable_testing()
add_subdirectory(tests)