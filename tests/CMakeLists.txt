cmake_minimum_required(VERSION 3.15)

project(URLShortenerTests VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
 googletest
 URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
 DOWNLOAD_EXTRACT_TIMESTAMP ON
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(URLShortenerTests 
 "TestURL.cpp" 
 "TestStringGenerator.cpp"
)

target_link_libraries(URLShortenerTests PRIVATE 
 gtest 
 gtest_main 
 url
 random)

target_include_directories(URLShortenerTests PRIVATE 
 "${CMAKE_SOURCE_DIR}/source/url"
 "${CMAKE_SOURCE_DIR}/source/random")