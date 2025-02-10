add_library(handler
  INTERFACE
)

target_link_libraries(handler INTERFACE
 random
 database
 spdlog::spdlog
 nlohmann_json::nlohmann_json
)

target_include_directories(handler INTERFACE 
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nlohmann_json_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

target_compile_features(handler INTERFACE cxx_std_20)