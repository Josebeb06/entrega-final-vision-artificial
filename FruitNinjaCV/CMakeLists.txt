cmake_minimum_required(VERSION 3.10)
project(FruitNinjaCV)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Si OpenCV está instalado en una ruta personalizada, descomenta la siguiente línea:
# set(OpenCV_DIR "C:/Program Files/opencv/build")

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(FruitNinja main.cpp)
target_link_libraries(FruitNinja ${OpenCV_LIBS})