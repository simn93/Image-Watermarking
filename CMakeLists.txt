cmake_minimum_required(VERSION 3.5)
project(spm_final)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-pthread")# -g -DTRACE_FASTFLOW
cmake_minimum_required (VERSION 2.6)

include_directories(fastflow)

add_executable(spm_final watermark/main.cpp watermark/CImg.h watermark/BlockingQueue.cpp watermark/stuff.cpp)

set(YOU_NEED_JPG 1)

if(${YOU_NEED_JPG} EQUAL 1)
    message(STATUS "Looking for libjpg...")
    find_package(JPEG REQUIRED)
    include_directories(${JPEG_INCLUDE_DIR})
    target_link_libraries (spm_final ${JPEG_LIBRARY})
    target_compile_definitions(spm_final PRIVATE cimg_use_jpeg=1)
endif()
