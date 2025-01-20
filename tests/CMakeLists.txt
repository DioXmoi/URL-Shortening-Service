cmake_minimum_required(VERSION 3.14)

project(URLShortenerTests VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
 googletest
 URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
 DOWNLOAD_EXTRACT_TIMESTAMP ON
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(URLShortenerTests 
 "TestURL.cpp" 
 "TestStringGenerator.cpp")

target_link_libraries(URLShortenerTests PRIVATE 
 gtest 
 gtest_main 
 url
 random)

target_include_directories(URLShortenerTests PRIVATE 
 "${CMAKE_SOURCE_DIR}/source/url"
 "${CMAKE_SOURCE_DIR}/source/random")