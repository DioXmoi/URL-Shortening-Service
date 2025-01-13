
cmake_minimum_required(VERSION 3.14)


project(url_shortening_service VERSION 1.0)

set(cmake_cxx_standard 20)
set(cmake_cxx_standard_required on)

add_executable(url_shortening_service app.cpp)