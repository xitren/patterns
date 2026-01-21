cmake_minimum_required(VERSION 3.16)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.16.0)
FetchContent_MakeAvailable(googletest)
add_library(GTest::GTest INTERFACE IMPORTED)
target_link_libraries(GTest::GTest INTERFACE gtest_main)

include(GoogleTest)
find_package(Threads REQUIRED)

option(PATTERNS_GTEST_DISCOVERY "Use gtest_discover_tests to register individual test cases" ON)

file(GLOB TESTS *.cpp)

foreach (file ${TESTS})
    get_filename_component(tgt ${file} NAME_WE)
    message(STATUS "Adding test \"${tgt}\"")
    add_executable(${tgt} ${file})
    target_compile_features(${tgt} PUBLIC cxx_std_20)
    if (MSVC)
        target_compile_options(${tgt} PRIVATE /W4 /permissive-)
    else ()
        target_compile_options(${tgt} PRIVATE -Wall -Wextra -Wpedantic -Wc++20-compat -Wno-format-security
                -Woverloaded-virtual -Wsuggest-override)
    endif ()
    target_link_libraries(${tgt} PRIVATE ${LIBRARY_NAME} GTest::gtest GTest::GTest Threads::Threads)
    if (PATTERNS_GTEST_DISCOVERY)
        # Discover tests at test time, not at build time. This is more robust for
        # cross-compiling environments and sanitizer builds where executing the test
        # binary during the build can be problematic.
        gtest_discover_tests(
                ${tgt}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DISCOVERY_MODE PRE_TEST
        )
    else ()
        # Coarse-grained mode: register only the executable itself as a test.
        add_test(NAME ${tgt} COMMAND ${tgt})
        set_tests_properties(${tgt} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif ()
endforeach ()
