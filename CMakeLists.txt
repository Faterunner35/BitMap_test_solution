cmake_minimum_required(VERSION 3.20)
project(BitMap_STL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release by default for single-configuration generators (Ninja, Makefiles).
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# -----------------------------------------------------------------------------
#  Library: BMP reading/writing and nearest-colour search (src/).
# -----------------------------------------------------------------------------
add_library(bitmap STATIC
    src/Image.cpp       src/Image.h
    src/NearestColour.cpp src/NearestColour.h)
target_include_directories(bitmap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(MSVC)
    target_compile_options(bitmap PUBLIC /W4 /utf-8 /permissive-)
else()
    target_compile_options(bitmap PUBLIC -Wall -Wextra -Wpedantic)
endif()

# Program: BitMap_STL <source.bmp> <palette.bmp> <output.bmp>; without arguments — demo.
add_executable(BitMap_STL src/main.cpp)
target_link_libraries(BitMap_STL PRIVATE bitmap)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BitMap_STL)

add_executable(bitmap_tests tests/bitmap_tests.cpp)
target_link_libraries(bitmap_tests PRIVATE bitmap)

# -----------------------------------------------------------------------------
#  Tests. The sample images are copied into the build tree, and both tests run
#  there, so the files they write (gradient.bmp, test_*.bmp) never land in the
#  source tree.
# -----------------------------------------------------------------------------
set(BITMAP_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/ DESTINATION ${BITMAP_TEST_DIR})

enable_testing()
add_test(NAME bitmap_tests COMMAND bitmap_tests WORKING_DIRECTORY ${BITMAP_TEST_DIR})
add_test(NAME demo         COMMAND BitMap_STL   WORKING_DIRECTORY ${BITMAP_TEST_DIR})
