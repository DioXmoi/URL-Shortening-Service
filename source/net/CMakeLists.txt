

add_library(net 
 listener.cpp 
 session.cpp
)

target_include_directories(net PRIVATE 
 ${Boost_INCLUDE_DIRS}
)