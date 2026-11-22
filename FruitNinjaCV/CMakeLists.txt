cmake_minimum_required(VERSION 3.16)
project(FruitNinjaCV LANGUAGES CXX)

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

add_executable(fruit_ninja_app main.cpp)
target_link_libraries(fruit_ninja_app PRIVATE ${OpenCV_LIBS})

# Copiar clasificador de rostros y DLLs de OpenCV junto al ejecutable
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/haarcascade_frontalface_default.xml")
    add_custom_command(TARGET fruit_ninja_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/haarcascade_frontalface_default.xml"
        "$<TARGET_FILE_DIR:fruit_ninja_app>/haarcascade_frontalface_default.xml"
    )
endif()

# Copiar DLLs de OpenCV si están disponibles
set(OPENCV_BIN_DIRS
    "${OpenCV_DIR}/bin"
    "${OpenCV_DIR}/../bin"
    "${OpenCV_DIR}/x64/vc16/bin"
    "${OpenCV_DIR}/x64/vc15/bin"
    "C:/opencv/build/x64/vc16/bin"
    "C:/opencv/build/x64/vc15/bin"
    "C:/opencv/build/bin"
)

foreach(BDIR ${OPENCV_BIN_DIRS})
    if(EXISTS "${BDIR}")
        file(GLOB TEMP_DLLS "${BDIR}/*.dll")
        if(TEMP_DLLS)
            add_custom_command(TARGET fruit_ninja_app POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${TEMP_DLLS}
                "$<TARGET_FILE_DIR:fruit_ninja_app>"
            )
            break()
        endif()
    endif()
endforeach()
