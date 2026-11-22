@echo off
setlocal
cd /d "%~dp0"

echo ================================================================
echo   LANZADOR DEL PLAYGROUND - VISION ARTIFICIAL (GUI)
echo ================================================================
echo.

:: 1. Agregar rutas de OpenCV al PATH de esta sesion
if exist "C:\opencv\build\x64\vc16\bin" set "PATH=C:\opencv\build\x64\vc16\bin;%PATH%"
if exist "C:\opencv\build\x64\vc15\bin" set "PATH=C:\opencv\build\x64\vc15\bin;%PATH%"
if defined OpenCV_DIR (
    if exist "%OpenCV_DIR%\bin" set "PATH=%OpenCV_DIR%\bin;%PATH%"
)

:: 2. Si no esta compilado, compilarlo con CMake
if not exist "build\Release\playground_app.exe" (
    echo [*] Compilando el Playground con CMake...
    cmake -B build
    cmake --build build --config Release
)

:: 3. Asegurar que las DLLs de OpenCV esten en build\Release
if not exist "build\Release\opencv_world4140.dll" (
    echo [*] Copiando librerias de OpenCV a build\Release...
    if exist "C:\opencv\build\x64\vc16\bin\opencv_world4140.dll" (
        copy /Y "C:\opencv\build\x64\vc16\bin\*.dll" "build\Release\" >nul
    )
)

:: 4. Asegurar que los assets esten presentes
if not exist "build\Release\assets" (
    echo [*] Copiando recursos de prueba...
    xcopy /E /I /Y "assets" "build\Release\assets" >nul
)

:: 5. Iniciar la aplicacion entrando directamente a la carpeta Release
echo [*] Iniciando Suite Interactiva...
echo [*] Ubicacion: build\Release\playground_app.exe
cd /d "%~dp0build\Release"
start "" "playground_app.exe"
