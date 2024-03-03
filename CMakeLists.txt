cmake_minimum_required(VERSION 3.16)

set(LIBRARY_NAME
    patterns_lib
)

set(GLOBAL_NAMESPACE
    xitren
)

project(${LIBRARY_NAME}
	DESCRIPTION "Patterns library"
    VERSION 1.0.0
	LANGUAGES CXX
)

add_library(${LIBRARY_NAME} INTERFACE)
add_library(${GLOBAL_NAMESPACE}::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_include_directories(
        ${LIBRARY_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

add_subdirectory(third_party/crc)
target_link_libraries(${LIBRARY_NAME} INTERFACE ${GLOBAL_NAMESPACE}::crc_lib)

message(STATUS "Adding library project \"${LIBRARY_NAME}\"")

enable_testing()
add_subdirectory(tests)