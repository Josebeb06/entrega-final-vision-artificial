@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

title Fruit Ninja CV - Vision Artificial

echo ================================================================
echo   FRUIT NINJA CV - VISION ARTIFICIAL (IA MEDIAPIPE)
echo ================================================================
echo.

:: 1. Localizar interprete de Python
set "PYTHON_EXE="
if exist "C:\Python314\python.exe" (
    set "PYTHON_EXE=C:\Python314\python.exe"
) else (
    for /f "delims=" %%i in ('where python 2^>nul') do (
        if not defined PYTHON_EXE set "PYTHON_EXE=%%i"
    )
)

if not defined PYTHON_EXE (
    for /f "delims=" %%i in ('where py 2^>nul') do (
        if not defined PYTHON_EXE set "PYTHON_EXE=%%i -3"
    )
)

:: 2. Si se encontro Python, ejecutar la version IA Full HD 1080p
if defined PYTHON_EXE (
    echo [*] Interprete localizado: !PYTHON_EXE!
    echo [*] Iniciando Fruit Ninja CV con MediaPipe HandLandmarker...
    echo [*] Resolucion: Full HD 1080p - Pulsa F para Pantalla Completa
    echo [*] Usa tu dedo indice frente a la camara para cortar las frutas
    echo.
    "!PYTHON_EXE!" "FruitNinjaCV\fruit_ninja.py"
    if !ERRORLEVEL! equ 0 goto :exito
    echo.
    echo [!] La version de Python cerro con codigo !ERRORLEVEL!.
) else (
    echo [!] No se encontro un interprete de Python en el sistema.
)

:: 3. Plan de contingencia: Ejecutar la version C++ compilada
echo.
echo [*] Intentando iniciar la version C++ compilada como alternativa...

if exist "C:\opencv\build\x64\vc16\bin" set "PATH=C:\opencv\build\x64\vc16\bin;%PATH%"
if exist "C:\opencv\build\x64\vc15\bin" set "PATH=C:\opencv\build\x64\vc15\bin;%PATH%"

if not exist "FruitNinjaCV\build\Release\fruit_ninja_app.exe" (
    echo [*] Compilando binario C++ de FruitNinjaCV...
    cmake -B FruitNinjaCV\build -S FruitNinjaCV
    cmake --build FruitNinjaCV\build --config Release
)

if not exist "FruitNinjaCV\build\Release\opencv_world4140.dll" (
    if exist "C:\opencv\build\x64\vc16\bin\opencv_world4140.dll" (
        copy /Y "C:\opencv\build\x64\vc16\bin\*.dll" "FruitNinjaCV\build\Release\" >nul
    )
)

if not exist "FruitNinjaCV\build\Release\haarcascade_frontalface_default.xml" (
    copy /Y "FruitNinjaCV\haarcascade_frontalface_default.xml" "FruitNinjaCV\build\Release\" >nul
)

if exist "FruitNinjaCV\build\Release\fruit_ninja_app.exe" (
    echo [*] Iniciando fruit_ninja_app.exe...
    cd /d "%~dp0FruitNinjaCV\build\Release"
    start "" "fruit_ninja_app.exe"
    goto :exito
)

echo.
echo [ERROR] No fue posible iniciar ni la version Python ni la version C++.
echo Presiona cualquier tecla para salir...
pause >nul
exit /b 1

:exito
exit /b 0
