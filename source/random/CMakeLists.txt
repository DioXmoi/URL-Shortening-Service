

add_library(random random.cpp random_string_generator.cpp)

target_compile_features(random PUBLIC cxx_std_20)