cmake_minimum_required(VERSION 3.16)
project(CaracterizacionOpenCV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Búsqueda dinámica y portable de OpenCV
if(NOT OpenCV_DIR AND NOT DEFINED ENV{OpenCV_DIR})
    set(OPENCV_HINTS
        "C:/opencv/build/x64/vc16/lib"
        "C:/opencv/build/x64/vc15/lib"
        "C:/opencv/build"
        "C:/opencv"
        "D:/opencv/build"
    )
    foreach(HINT ${OPENCV_HINTS})
        if(EXISTS "${HINT}")
            set(OpenCV_DIR "${HINT}")
            break()
        endif()
    endforeach()
endif()

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(caracterizacion_app main.cpp)
target_link_libraries(caracterizacion_app PRIVATE ${OpenCV_LIBS})

# Copiar imagen de prueba junto al ejecutable tras compilar
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/input.jpg")
    add_custom_command(TARGET caracterizacion_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/input.jpg"
        "$<TARGET_FILE_DIR:caracterizacion_app>/input.jpg"
    )
endif()
