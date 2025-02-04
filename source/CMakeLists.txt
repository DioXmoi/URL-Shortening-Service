cmake_minimum_required(VERSION 3.15)

project(URLShortener VERSION 1.0)

set(cmake_cxx_standard 20)
set(cmake_cxx_standard_required TRUE)

cmake_policy(SET CMP0144 NEW)

find_package(Boost REQUIRED system)

if(NOT Boost_FOUND)
 message(FATAL_ERROR "Boost not found")
endif()

include(FetchContent)

FetchContent_Declare(
 spdlog
 GIT_REPOSITORY https://github.com/gabime/spdlog.git
 GIT_TAG        v1.15.0
)

FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
 nlohmann_json
 GIT_REPOSITORY https://github.com/nlohmann/json.git
 GIT_TAG        v3.11.3
)

FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(url)
add_subdirectory(random)
add_subdirectory(net)
add_subdirectory(database)

add_executable(URLShortener app.cpp)

target_link_libraries(URLShortener PRIVATE 
 url
 random
 net
 database
 Boost::system
 spdlog::spdlog
 nlohmann_json::nlohmann_json
)

target_include_directories(URLShortener PRIVATE 
 "${PROJECT_SOURCE_DIR}" 
 "${PROJECT_SOURCE_DIR}/url"
 "${PROJECT_SOURCE_DIR}/random"
 "${PROJECT_SOURCE_DIR}/net"
 "${PROJECT_SOURCE_DIR}/database"
 "${Boost_INCLUDE_DIRS}"
 "${PostgreSQL_INCLUDE_DIRS}"
)