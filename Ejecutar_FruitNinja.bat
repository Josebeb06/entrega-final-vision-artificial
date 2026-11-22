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

:: 2. Si no esta compilado, compilarlo con CMake
if not exist "FruitNinjaCV\build\Release\fruit_ninja_app.exe" (
    echo [*] Compilando FruitNinjaCV por primera vez...
    cmake -B FruitNinjaCV\build -S FruitNinjaCV
    cmake --build FruitNinjaCV\build --config Release
)

:: 3. Asegurar que las DLLs de OpenCV esten en Release
if not exist "FruitNinjaCV\build\Release\opencv_world4140.dll" (
    echo [*] Copiando librerias de OpenCV a Release...
    if exist "C:\opencv\build\x64\vc16\bin\opencv_world4140.dll" (
        copy /Y "C:\opencv\build\x64\vc16\bin\*.dll" "FruitNinjaCV\build\Release\" >nul
    )
)

:: 4. Asegurar que el clasificador facial este presente
if not exist "FruitNinjaCV\build\Release\haarcascade_frontalface_default.xml" (
    copy /Y "FruitNinjaCV\haarcascade_frontalface_default.xml" "FruitNinjaCV\build\Release\" >nul
)

:: 5. Iniciar Fruit Ninja CV
echo [*] Iniciando Fruit Ninja CV...
echo [*] Muestra tu mano y extiende tu dedo indice frente a la camara!
echo.
cd /d "%~dp0FruitNinjaCV\build\Release"
start "" "fruit_ninja_app.exe"
