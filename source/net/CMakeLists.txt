
find_package(Boost REQUIRED)

if(NOT Boost_FOUND)
 message(FATAL_ERROR "Boost not found")
endif()

add_library(net 
 listener.cpp 
 session.cpp
)

target_include_directories(net PRIVATE 
 ${Boost_INCLUDE_DIRS}
)