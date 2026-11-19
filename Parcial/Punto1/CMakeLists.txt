cmake_minimum_required(VERSION 3.16)
project(Punto1Convolucion LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ruta a OpenCV en tu sistema
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib")

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(Punto1_App main.cpp)
target_link_libraries(Punto1_App PRIVATE ${OpenCV_LIBS})