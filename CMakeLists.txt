cmake_minimum_required(VERSION 3.0.0)
project(RetinaCpp VERSION 0.1.0)
find_package( OpenCV 3 REQUIRED )

include_directories( ${OpenCV_INCLUDE_DIRS} )
link_directories( lib )
FILE(GLOB SOURCES src/*.cpp)

include(CTest)
enable_testing()

add_executable(RetinaCpp main.cpp ${SOURCES})
target_include_directories(RetinaCpp PRIVATE include)
target_link_libraries( RetinaCpp ${OpenCV_LIBS} onnxruntime)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
