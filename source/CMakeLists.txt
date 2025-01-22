cmake_minimum_required(VERSION 3.14)

project(URLShortener VERSION 1.0)

set(cmake_cxx_standard 20)
set(cmake_cxx_standard_required TRUE)


add_subdirectory(url)
add_subdirectory(random)
add_subdirectory(net)

add_executable(URLShortener app.cpp)

target_link_libraries(URLShortener PRIVATE 
 url
 random
 net
)

target_include_directories(URLShortener PRIVATE 
 "${PROJECT_SOURCE_DIR}/url"
 "${PROJECT_SOURCE_DIR}/random"
 "${PROJECT_SOURCE_DIR}/net"
)