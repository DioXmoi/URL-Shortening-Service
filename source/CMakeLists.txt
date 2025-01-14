
cmake_minimum_required(VERSION 3.14)

project(url_shortening_service VERSION 1.0)


set(cmake_cxx_standard cxx_std_20)
set(cmake_cxx_standard_required TRUE)

add_subdirectory(url)
add_subdirectory(random)

add_executable(url_shortening_service app.cpp)

target_link_libraries(url_shortening_service PRIVATE 
	url
	random
)

target_include_directories(url_shortening_service PRIVATE 
	"${PROJECT_SOURCE_DIR}/url"
	"${PROJECT_SOURCE_DIR}/random" 
)