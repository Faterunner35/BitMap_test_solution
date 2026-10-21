cmake_minimum_required(VERSION 3.16)
project(BitMap_STL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(bitmap STATIC Image.cpp NearestColour.cpp)
target_include_directories(bitmap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
    target_compile_options(bitmap PUBLIC /W4 /utf-8 /permissive-)
else()
    target_compile_options(bitmap PUBLIC -Wall -Wextra -Wpedantic)
endif()

# Программа: BitMap_STL <source.bmp> <palette.bmp> <output.bmp>, без аргументов — демо
add_executable(BitMap_STL main.cpp)
target_link_libraries(BitMap_STL PRIVATE bitmap)

# Тесты читают sample_1280x853.bmp и obraz-A.jpg, поэтому запускаются из корня репозитория
add_executable(bitmap_tests tests.cpp)
target_link_libraries(bitmap_tests PRIVATE bitmap)

enable_testing()
add_test(NAME bitmap_tests COMMAND bitmap_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME demo COMMAND BitMap_STL WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
