
cmake_minimum_required(VERSION 3.14)

project(URLShortener VERSION 1.0)


set(cmake_cxx_standard 20)
set(cmake_cxx_standard_required TRUE)

add_subdirectory(url)
add_subdirectory(random)

add_executable(URLShortener app.cpp)

target_link_libraries(URLShortener PRIVATE 
	url
	random
)

target_include_directories(URLShortener PRIVATE 
	"${PROJECT_SOURCE_DIR}/url"
	"${PROJECT_SOURCE_DIR}/random" 
)


# target_include_directories(url PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # добавляем папку url для поиска заголовков