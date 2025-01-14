

add_library(random random.cpp randomStringGenerator.cpp)

target_compile_features(random PUBLIC cxx_std_20)