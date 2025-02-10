cmake_minimum_required(VERSION 3.15)

project(URLShortener VERSION 1.0)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT Boost_FOUND)
 message(FATAL_ERROR "Boost not found")
endif()

include(FetchContent)

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

configure_file(Config.h.in Config.h)

add_subdirectory(url)
add_subdirectory(random)
add_subdirectory(net)
add_subdirectory(database)
add_subdirectory(handler)

add_executable(URLShortener app.cpp)

target_link_libraries(URLShortener PRIVATE 
 url
 random
 net
 database
 handler
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
 "${PROJECT_SOURCE_DIR}/handler"
 "${Boost_INCLUDE_DIRS}"
 "${PostgreSQL_INCLUDE_DIRS}"
 "${CMAKE_CURRENT_SOURCE_DIR}"
 "${nlohmann_json_SOURCE_DIR}"
 "${PROJECT_BINARY_DIR}"
)