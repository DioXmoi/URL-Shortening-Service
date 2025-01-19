
add_library(url url.cpp)


target_compile_features(url PUBLIC cxx_std_20) # явно включаем c++20 дл€ библиотеки url
