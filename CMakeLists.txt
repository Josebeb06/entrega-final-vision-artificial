cmake_minimum_required(VERSION 3.16)
project(VisionArtificialPlayground LANGUAGES CXX)

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
            set(OpenCV_DIR "${HINT}" CACHE PATH "Ruta de instalacion de OpenCV")
            break()
        endif()
    endforeach()
endif()

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV encontrado con exito: ${OpenCV_VERSION}")
message(STATUS "OpenCV Include Dirs: ${OpenCV_INCLUDE_DIRS}")

include_directories(${OpenCV_INCLUDE_DIRS})

# 1. EJECUTABLE PRINCIPAL: SUITE GRAFICA INTERACTIVA (UI GLOBAL)
add_executable(playground_app playground/main_playground.cpp)
target_link_libraries(playground_app PRIVATE ${OpenCV_LIBS})

# Copiar carpeta assets automáticamente junto al ejecutable final
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET playground_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:playground_app>/assets"
    )
endif()

# Copiar DLLs de OpenCV automáticamente junto al ejecutable para que no falle por dll faltante
set(OPENCV_BIN_DIRS
    "${OpenCV_DIR}/bin"
    "${OpenCV_DIR}/../bin"
    "${OpenCV_DIR}/x64/vc16/bin"
    "${OpenCV_DIR}/x64/vc15/bin"
    "C:/opencv/build/x64/vc16/bin"
    "C:/opencv/build/x64/vc15/bin"
    "C:/opencv/build/bin"
)

set(FOUND_OPENCV_DLLS "")
foreach(BDIR ${OPENCV_BIN_DIRS})
    if(EXISTS "${BDIR}")
        file(GLOB TEMP_DLLS "${BDIR}/*.dll")
        if(TEMP_DLLS)
            set(FOUND_OPENCV_DLLS ${TEMP_DLLS})
            break()
        endif()
    endif()
endforeach()

if(FOUND_OPENCV_DLLS)
    add_custom_command(TARGET playground_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${FOUND_OPENCV_DLLS}
        "$<TARGET_FILE_DIR:playground_app>"
    )
endif()

# 2. SUBPROYECTOS INDIVIDUALES AUTÓNOMOS
add_subdirectory("Binarización")
add_subdirectory(Frecuencia)
add_subdirectory(TallerCalibracion)
add_subdirectory(Filtros)
add_subdirectory(Histogramas)
add_subdirectory(MorfologiaMatematica)
add_subdirectory(Segmentacion)
add_subdirectory(Caracterizacion)
