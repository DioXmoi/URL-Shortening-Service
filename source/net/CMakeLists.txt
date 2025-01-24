

add_library(net 
 INTERFACE
)

target_include_directories(net INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(net INTERFACE
    ${Boost_INCLUDE_DIRS}
)