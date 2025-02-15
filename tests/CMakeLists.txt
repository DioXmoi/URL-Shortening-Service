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

FetchContent_Declare(
 spdlog
 GIT_REPOSITORY https://github.com/gabime/spdlog.git
 GIT_TAG        v1.15.0
)

FetchContent_Declare(
 nlohmann_json
 GIT_REPOSITORY https://github.com/nlohmann/json.git
 GIT_TAG        v3.11.3
)

FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(nlohmann_json)

configure_file(TestConfig.h.in TestConfig.h)

add_executable(URLShortenerTests 
 "TestURL.cpp" 
 "TestStringGenerator.cpp"
 "TestHandler.cpp")

target_link_libraries(URLShortenerTests PRIVATE 
 gtest 
 gtest_main 
 Boost::system
 url
 random
 database
 spdlog::spdlog
 nlohmann_json::nlohmann_json)

target_include_directories(URLShortenerTests PRIVATE 
 "${CMAKE_SOURCE_DIR}/source/url"
 "${CMAKE_SOURCE_DIR}/source/random"
 "${CMAKE_SOURCE_DIR}/source/handler"
 "${CMAKE_SOURCE_DIR}/source/database"
 "${Boost_INCLUDE_DIRS}"
 "${PROJECT_BINARY_DIR}"
)