cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0144 NEW)

find_package(Boost REQUIRED system)

project(URLShorteningService 
 LANGUAGES CXX
)

add_subdirectory(tests)

add_subdirectory(source)