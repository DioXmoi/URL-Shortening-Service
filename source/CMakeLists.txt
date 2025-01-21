cmake_minimum_required(VERSION 3.14)
project(URLShortener VERSION 1.0)

set(cmake_cxx_standard 20)
set(cmake_cxx_standard_required TRUE)

include(FetchContent)
FetchContent_Declare(
    userver
    URL https://github.com/userver-framework/userver/archive/refs/tags/v2.6.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)

set(USERVER_FEATURE_TESTSUITE OFF CACHE BOOL "Disable testsuite" FORCE)

FetchContent_MakeAvailable(userver)


add_subdirectory(url)
add_subdirectory(random)

add_executable(URLShortener app.cpp)

target_link_libraries(URLShortener PRIVATE 
    url
    random
    userver-core 
    userver-postgresql
)

target_include_directories(URLShortener PRIVATE 
    "${PROJECT_SOURCE_DIR}/url"
    "${PROJECT_SOURCE_DIR}/random" 
)