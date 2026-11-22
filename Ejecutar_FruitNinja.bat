@echo off
setlocal
cd /d "%~dp0"

echo ================================================================
echo   LANZADOR FRUIT NINJA CV - VISION ARTIFICIAL
echo ================================================================
echo.

:: 1. Agregar rutas de OpenCV al PATH de esta sesion
if exist "C:\opencv\build\x64\vc16\bin" set "PATH=C:\opencv\build\x64\vc16\bin;%PATH%"
if exist "C:\opencv\build\x64\vc15\bin" set "PATH=C:\opencv\build\x64\vc15\bin;%PATH%"
if defined OpenCV_DIR (
    if exist "%OpenCV_DIR%\bin" set "PATH=%OpenCV_DIR%\bin;%PATH%"
    if exist "%OpenCV_DIR%\..\bin" set "PATH=%OpenCV_DIR%\..\bin;%PATH%"
    if exist "%OpenCV_DIR%\..\..\bin" set "PATH=%OpenCV_DIR%\..\..\bin;%PATH%"
)

:: 2. Prioridad: Version con MediaPipe IA (Deteccion Inmune a Cuadros y Fondos)
where python >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo [*] Detectado entorno Python con IA.
    echo [*] Iniciando Fruit Ninja CV (MediaPipe AI HandLandmarker - Full HD 1080p)...
    echo [*] Muestra tu mano y usa tu dedo indice como espada katana!
    echo.
    python FruitNinjaCV\fruit_ninja.py
    if %ERRORLEVEL% equ 0 goto :fin
)

:: 3. Contingencia: Version C++ nativa
echo [*] Iniciando version C++ nativa...
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

cd /d "%~dp0FruitNinjaCV\build\Release"
start "" "fruit_ninja_app.exe"

:fin
