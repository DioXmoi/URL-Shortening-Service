
add_library(url url.cpp)


target_compile_features(url PUBLIC cxx_std_20) # явно включаем c++20 дл€ библиотеки url

target_include_directories(url PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) # добавл€ем папку url дл€ поиска заголовков