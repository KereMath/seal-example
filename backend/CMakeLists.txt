cmake_minimum_required(VERSION 3.13)
project(SEALServer)

find_package(SEAL 4.1 REQUIRED)

add_executable(seal_server src/server.cpp)
target_link_libraries(seal_server SEAL::seal)

# Enable C++17
target_compile_features(seal_server PUBLIC cxx_std_17)
