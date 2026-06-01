cmake_minimum_required(VERSION 3.18)
project(laba_4 CUDA CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CUDA_STANDARD 14)

if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native)
endif()

add_executable(laba_44
        src/main.cu
        src/power_array.cu
        src/rotate_image.cu
)

target_include_directories(lab4 PRIVATE include)
