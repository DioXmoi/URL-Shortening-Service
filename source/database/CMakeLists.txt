cmake_minimum_required(VERSION 3.15)



find_package(PostgreSQL REQUIRED)

add_library(database 
 postgresql.cpp 
)


if(PostgreSQL_FOUND)
 message(STATUS "PostgreSQL found:")
 message(STATUS "  Include Dir: ${PostgreSQL_INCLUDE_DIRS}")
 message(STATUS "  Libraries:   ${PostgreSQL_LIBRARIES}")

 include_directories(${PostgreSQL_INCLUDE_DIRS})
 target_link_libraries(database PUBLIC
  ${PostgreSQL_LIBRARIES}
  Boost::system
 )

 target_include_directories(database PUBLIC
  ${PostgreSQL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
 )

 target_compile_features(database PUBLIC cxx_std_20)

else()
 message(FATAL_ERROR "PostgreSQL not found.")
endif()