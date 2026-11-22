# Entrega Final - Vision Artificial

Playground Interactivo y Suite Modular de Procesamiento Digital de Imagenes y Vision por Computador.

* **Repositorio oficial:** https://github.com/Josebeb06/entrega-final-vision-artificial.git
* **Rama principal (Branch):** `main`
* **Lenguaje:** C++ (Estandar C++17)
* **Libreria principal:** OpenCV 4.x (Nucleo C++ / HighGUI / Imgproc / Calib3d)
* **Sistema de construccion:** CMake (>= 3.16)

---

## 1. Descripcion General y Arquitectura del Sistema

El proyecto esta disenado bajo una arquitectura desacoplada y modular que combina una **Suite Grafica Central (Playground UI)** con **modulos autonomos independientes**. 

### 1.1. Principios de Diseno Arquitectonico

* **Capa de Nucleo y Algoritmos:** Cada tecnica de vision artificial esta encapsulada en funciones modulares reutilizables que procesan matrices `cv::Mat`.
* **Capa de Presentacion e Interaccion:** Disenada sobre OpenCV HighGUI mediante un renderizador personalizado en modo oscuro (Dark Theme), gestionando eventos de raton (`cv::setMouseCallback`), botones con estado hover/activo, tarjetas dinamicas y visualizadores interactivos con barras de desplazamiento (`cv::createTrackbar`).
* **Portabilidad y Desacoplamiento de Rutas:** Ningun modulo contiene rutas absolutas locales ni rutas fijas de usuario. La resolucion de recursos multimedia opera de forma relativa (`assets/` o carpetas locales) y la localizacion de OpenCV en CMake utiliza heuristica adaptativa sin forzar rutas del sistema operativo del desarrollador.
* **Compilacion Dual:** Permite construir todos los proyectos simultaneamente desde el archivo raiz de CMake o compilar cualquiera de los 8 modulos de manera aislada.

### 1.2. Estructura de Directorios

```text
.
|-- CMakeLists.txt                # Configuracion global de CMake (Playground + Subproyectos)
|-- Ejecutar_Playground.bat       # Script lanzador en Windows con resolucion de entorno
|-- Ejecutar_FruitNinja.bat       # Lanzador directo del juego interactivo Fruit Ninja CV
|-- Ejecutar.txt                  # Guia rapida de ejecucion por terminal
|-- .gitignore                    # Exclusion estricta de binarios, temporales y caches
|-- assets/                       # Banco de imagenes de prueba estandarizadas
|   |-- binarizacion.jpg
|   |-- caracterizacion.jpg
|   |-- filtros.jpg
|   |-- histogramas.jpg
|   |-- lenna.jpg
|   |-- morfologia.jpg
|   |-- segmentacion.jpg
|   |-- tablero_calibracion.png
|-- playground/                   # Codigo fuente de la Suite Grafica Principal
|   `-- main_playground.cpp
|-- Binarizacion/                 # Modulo de Umbralizacion y Binarizacion
|   |-- CMakeLists.txt
|   |-- Ejecutar.txt
|   |-- input.jpg
|   `-- main.cpp
|-- Caracterizacion/              # Modulo de Descriptores de Forma y Contornos
|   |-- CMakeLists.txt
|   |-- Ejecutar.txt
|   |-- input.jpg
|   `-- main.cpp
|-- Filtros/                      # Modulo de Filtros Espaciales y Convolucion
|   |-- CMakeLists.txt
|   |-- Ejecutar.txt
|   |-- input.jpg
|   `-- main.cpp
|-- Frecuencia/                   # Modulo de Analisis en Dominio Frecuencial (FFT)
|   |-- CMakeLists.txt
|   |-- Ejecutar.txt
|   |-- lenna.jpg
|   `-- main.cpp
|-- Histogramas/                  # Modulo de Analisis y Ecualizacion de Histogramas
|   |-- CMakeLists.txt
|   |-- Ejecutar.txt
|   |-- input.jpg
|   `-- main.cpp
|-- MorfologiaMatematica/         # Modulo de Operaciones Morfologicas
|   |-- CMakeLists.txt
|   |-- Ejecutar.txt
|   |-- input.jpg
|   `-- main.cpp
|-- Segmentacion/                 # Modulo de Segmentacion (Color, Watershed, K-Means)
|   |-- CMakeLists.txt
|   |-- Ejecutar.txt
|   |-- input.jpg
|   `-- main.cpp
`-- TallerCalibracion/            # Modulo de Modelos de Camara, Pinhole y Homografia
    |-- CMakeLists.txt
    |-- Ejecutar.txt
    |-- tablero_calibracion.png
    `-- main.cpp
```

---

## 2. Modulos Implementados y Cobertura Tematica

### Modulo 1: Modelos de Camara y Calibracion (`TallerCalibracion/`)
* **Modelo Pinhole e Intrinsecos:** Deteccion subpixel de esquinas de tablero de ajedrez (9x6 esquinas interiores), calculo de la matriz intrinseca de camara $K$ (distancias focales $f_x, f_y$ y punto principal $c_x, c_y$), coeficientes de distorsion radial/tangencial ($k_1, k_2, p_1, p_2, k_3$), calculo del error de reproyeccion RMS y correccion geometrica (`cv::undistort`).
* **Rectificacion Proyectiva y Homografia:** Estimacion de la matriz de homografia plana $3\times3$ mediante RANSAC (`cv::findHomography`) y correccion de perspectiva frontal ortogonal (`cv::warpPerspective`).
* **Camara en Vivo:** Deteccion robusta de patron en tiempo real con algoritmos sector-based (`cv::findChessboardCornersSB`) y clasico.

### Modulo 2: Filtros Espaciales y Convolucion (`Filtros/`)
* **Suavizado (Filtros de Paso Bajo):** Box filter (`cv::blur`), filtro Gaussiano con parametrizacion de $\sigma$, filtro de mediana (`cv::medianBlur`) para eliminacion de ruido impulsivo (sal y pimienta), y filtro bilateral (`cv::bilateralFilter`) para preservacion de bordes.
* **Deteccion de Bordes y Derivadas (Filtros de Paso Alto):** Operadores de gradiente de Sobel en $X$ y en $Y$, magnitud combinada $\sqrt{G_x^2 + G_y^2}$, operador Laplaciano de segunda derivada y detector Canny con umbralizacion por histeresis.
* **Realce y Sharpening:** Convolucion de nitidez con kernel Laplaciano y algoritmo de Unsharp Masking.
* **Laboratorio con Sliders:** Control interactivo de tamano de kernel, desviacion estandar y umbrales en tiempo real.

### Modulo 3: Histogramas (`Histogramas/`)
* **Histograma Monocromatico:** Calculo de distribucion de frecuencias de 256 niveles (`cv::calcHist`) y evaluacion estadistica del brillo: media ($\mu$), desviacion estandar ($\sigma$), valor minimo y maximo.
* **Histograma Cromático RGB/BGR:** Visualizacion superpuesta de las tres curvas de color con cuadricula de referencia.
* **Mejora de Contraste:** Comparativa entre Ecualizacion Global (`cv::equalizeHist`) y Ecualizacion Adaptativa Limitada por Contraste (**CLAHE** - `cv::createCLAHE`).
* **Transformacion Lineal Interactiva:** Ajuste en vivo de contraste ($\alpha$) y brillo ($\beta$) mediante la ecuacion $g(x) = \alpha f(x) + \beta$ con actualizacion dinamica del histograma.

### Modulo 4: Umbralizacion y Binarizacion (`Binarizacion/`)
* **Umbralizacion Global Fija:** Binarizacion clasica por valor de corte estatico ($T = 127$).
* **Metodo de Otsu:** Calculo automatico del umbral optimo maximizando la varianza inter-clase, con trazado de la linea de umbral sobre el histograma de tonos.
* **Umbralizaciones Adaptativas:** Algoritmos locales de Media y Gaussiana ponderada para compensar iluminacion no uniforme.
* **Streaming en Vivo:** Binarizacion multiventana sobre video en tiempo real.

### Modulo 5: Morfologia Matematica (`MorfologiaMatematica/`)
* **Operaciones Fundamentales:** Erosion (adelgazamiento de objetos) y Dilatacion (expansion y llenado de huecos).
* **Operaciones Compuestas:** Apertura (eliminacion de ruido externo), Cierre (sellado de cavidades internas), Gradiente Morfologico (extraccion del perimetro exterior), Top-Hat (aislamiento de elementos claros sobre fondo oscuro) y Black-Hat (aislamiento de elementos oscuros sobre fondo claro).
* **Elementos Estructurantes:** Analisis comparativo de kernels con forma rectangular (`cv::MORPH_RECT`), en cruz (`cv::MORPH_CROSS`) y eliptica (`cv::MORPH_ELLIPSE`).
* **Laboratorio Dinamico:** Variacion interactiva de tamano de kernel e iteraciones morfológicas consecutivas.

### Modulo 6: Segmentacion de Imagenes (`Segmentacion/`)
* **Segmentacion Cromatica en Espacio HSV:** Filtrado por rangos (`cv::inRange`) en los canales Matiz (Hue), Saturacion (Saturation) y Brillo (Value), con soporte de mascara binaria y presets para colores primarios.
* **Algoritmo Watershed (Cuencas Hidrograficas):** Segmentacion de objetos superpuestos o en contacto mediante transformada de distancia euclidiana L2 (`cv::distanceTransform`), obtencion de marcadores de componentes conexos (`cv::connectedComponents`) y delimitacion de fronteras hidrograficas.
* **Clustering no Supervisado K-Means:** Cuantizacion y agrupamiento de pixeles en el espacio tridimensional de color (`cv::kmeans`) con particion de regiones segun colores dominantes.
* **Seguimiento en Vivo:** Deteccion y tracking del objeto de mayor area en stream de camara web.

### Modulo 7: Caracterizacion y Descriptores de Forma (`Caracterizacion/`)
* **Propiedades Geometricas:** Extraccion de contornos con jerarquia (`cv::findContours`), computo de area (`cv::contourArea`), perimetro (`cv::arcLength`), centroide mediante momentos espaciales ($m_{10}/m_{00}, m_{01}/m_{00}$), Bounding Box recto, Bounding Box rotado (`cv::minAreaRect`) y Circulo Minimo Circunscrito.
* **Descriptores Invariantes (Momentos de Hu):** Calculo de los 7 momentos invariantes a traslacion, escala y rotacion, transformados a escala logaritmica.
* **Envolvente Convexa y Defectos:** Calculo de la silueta convexa (`cv::convexHull`) y deteccion de concavidades pronunciadas (`cv::convexityDefects`).
* **Puntos Clave y Esquinas:** Deteccion de esquinas por algoritmo de Harris (`cv::cornerHarris`) y Shi-Tomasi Good Features to Track (`cv::goodFeaturesToTrack`).

### Modulo 8: Analisis de Frecuencia (`Frecuencia/`)
* **Transformada Discreta de Fourier 2D (DFT):** Conversion del dominio espacial al dominio frecuencial complejo, computo de la magnitud espectral en escala logaritmica $M = \log(1 + |F(u, v)|)$ y reordenamiento de cuadrantes (centering/shift de la componente DC al origen).
* **Espectro en Tiempo Real:** Visualizacion de la distribucion de frecuencias espaciales a partir del flujo de la camara web.

### Modulo Adicional: Fruit Ninja CV (`FruitNinjaCV/`)
* **Implementacion con IA (MediaPipe HandLandmarker - Recomendada):**
  - **Rastreo Neuronal 3D:** Deteccion precisa de la punta del dedo indice (Landmark 8) mediante red neuronal convolucional (LiteRT / XNNPACK). Totalmente inmune a marcos dorados de cuadros en la pared, iluminacion cambiante, muebles, ropa y rostros.
  - **Resolucion Full HD (1920x1080):** Canvas de alta resolucion con soporte para alternar a pantalla completa mediante la tecla `F`.
  - **Soporte Bimanual (Doble Katana):** Permite cortar con una o ambas manos de forma simultanea.
  - **Estela Katana de Neon:** Efecto visual de sable con nucleo blanco de corte, resplandor exterior y chispas balisticas.
  - **Generacion Procedural de Frutas HD:** Sprites vectoriales de alta definicion (Sandia con semillas y corteza, Naranja con gajos y albedo, Manzana roja con tallo y hoja, Platano curvado, Limon amarillo y Bomba con mecha ardiente animada).
  - **Fisica y Separacion Afin:** Trayectorias parabolicas con gravedad ($g = 0.58$), rotacion continua y separacion dinamica de mitades con velocidad angular al rebanar.
  - **Salpicaduras en Pantalla:** Manchas de jugo que se estampan en el lente virtual de la camara y se van desvaneciendo progresivamente.
  - **Sistema de Combos y Audio:** Deteccion de cortes multiples en intervalos inferiores a 350 ms ("COMBO x2", "COMBO x3", "MEGA COMBO x4+"), efectos de sonido sintetizados asincronos y sistema de 3 vidas.
  - **Menu Interactivo:** Inicio del juego y reinicio tras Game Over mediante el corte de una sandia flotante interactiva en pantalla.
* **Implementacion C++ Nativa:**
  - Segmentacion hibrida de piel en espacios YCrCb + HSV, supresion perimetral superior (evita deteccion de cuadros colgados en la pared) y mascara facial Haar Cascade.

---

## 3. Requisitos del Sistema y Dependencias

Para compilar y ejecutar el proyecto en cualquier maquina se requiere:

### Requisitos para el Playground Principal (C++)
1. **Compilador C++:**
   * **Windows:** Visual Studio 2019 o Visual Studio 2022 (con carga de trabajo "Desarrollo para el escritorio con C++" instalada, MSVC v142/v143).
   * **Linux:** GCC 9+ o Clang 10+ (`build-essential`).
   * **macOS:** Apple Clang / Xcode Command Line Tools.
2. **CMake:** Version 3.16 o superior.
3. **OpenCV:** Version 4.x (Nucleo C++ con librerias de enlace e includes).

### Requisitos para Fruit Ninja CV con IA (Python)
* **Python:** Version 3.9 o superior (probado y optimizado en Python 3.10 - 3.14).
* **Instalacion automatica de dependencias de Python:**
  ```bash
  pip install -r requirements.txt
  ```
  *(Instala `opencv-python`, `mediapipe` y `numpy`).*

---

### Instalacion rapida de dependencias de OpenCV C++ segun el Sistema Operativo

#### En Windows:
1. Descargar OpenCV precompilado para Windows desde el repositorio oficial: https://github.com/opencv/opencv/releases (por ejemplo OpenCV 4.10.0 o 4.14.0).
2. Extraer el instalador en `C:\opencv` (ruta estandar reconocida de forma automatica por el proyecto).
3. Opcional: Si se extrae en otra ruta, definir la variable de entorno `OpenCV_DIR` apuntando a la carpeta `build` (ejemplo: `D:\mi_ruta\opencv\build`).

#### En Linux (Ubuntu / Debian):
```bash
sudo apt update
sudo apt install -y build-essential cmake libopencv-dev python3-pip
pip install -r requirements.txt
```

#### En macOS (con Homebrew):
```bash
brew update
brew install cmake opencv python
pip3 install -r requirements.txt
```

---

## 4. Instrucciones de Compilacion y Ejecucion

### Metodo 1: Suite Grafica Principal (Playground UI)

#### En Windows:
Doble clic sobre el archivo [Ejecutar_Playground.bat](Ejecutar_Playground.bat). El script verifica la compilacion con CMake, copia las librerias dinamicas (`opencv_world*.dll`) e inicia la aplicacion global con los 8 modulos integrados.

#### Desde la terminal (Cualquier plataforma):
```bash
# 1. Configurar y generar el proyecto con CMake
cmake -B build

# 2. Compilar la solucion completa en modo Release
cmake --build build --config Release

# 3. Ejecutar la Suite Grafica Interactiva
# En Windows:
.\build\Release\playground_app.exe
# En Linux / macOS:
./build/playground_app
```

---

### Metodo 2: Fruit Ninja CV (Juego Standalone)

#### En Windows (Lanzador automatico dedicado):
Doble clic sobre el archivo [Ejecutar_FruitNinja.bat](Ejecutar_FruitNinja.bat). El lanzador abre de forma directa la version Full HD 1080p con rastreo neuronal MediaPipe (o compila la version C++ como alternativa de contingencia si no se dispone de Python).

#### Desde la terminal:
```bash
# Version con Inteligencia Artificial (MediaPipe):
python FruitNinjaCV/fruit_ninja.py

# Version C++ compilada:
cd FruitNinjaCV
cmake -B build
cmake --build build --config Release
.\build\Release\fruit_ninja_app.exe
```

---

### Metodo 2: Compilacion y Ejecucion de Modulos Individuales

Cada carpeta temática contiene su propio `CMakeLists.txt` y archivo de instrucciones `Ejecutar.txt`. Si se desea compilar un modulo especifico de forma aislada:

```bash
# Ejemplo para el modulo de Filtros
cd Filtros
cmake -B build
cmake --build build --config Release
.\build\Release\filtros_app.exe
```

Tabla resumen de binarios independientes generados:

| Modulo | Comando de Ejecucion Directa (desde la raiz tras compilar) |
| :--- | :--- |
| **Suite Principal (Playground UI)** | `.\build\Release\playground_app.exe` |
| **Modelos de Camara y Calibracion** | `.\build\TallerCalibracion\Release\calibracion_app.exe` |
| **Filtros Espaciales** | `.\build\Filtros\Release\filtros_app.exe` |
| **Histogramas** | `.\build\Histogramas\Release\histogramas_app.exe` |
| **Umbralizacion y Binarizacion** | `.\build\Binarización\Release\binarizacion_app.exe` |
| **Morfologia Matematica** | `.\build\MorfologiaMatematica\Release\morfologia_app.exe` |
| **Segmentacion** | `.\build\Segmentacion\Release\segmentacion_app.exe` |
| **Caracterizacion** | `.\build\Caracterizacion\Release\caracterizacion_app.exe` |
| **Frecuencia (Fourier)** | `.\build\Frecuencia\Release\frecuencia_app.exe` |
| **Fruit Ninja CV** | `.\FruitNinjaCV\build\Release\fruit_ninja_app.exe` (o doble clic en `Ejecutar_FruitNinja.bat`) |

---

## 5. Instrucciones de Navegacion en la Interfaz Grafica

* **Seleccion de Modulo:** Clic izquierdo sobre cualquiera de las 8 tarjetas del menu principal.
* **Navegacion entre Sub-Acciones:** Clic en las pestanas superiores del modulo seleccionado.
* **Retorno al Menu:** Clic en el boton `< VOLVER AL MENU` (esquina superior izquierda) o presionar la tecla `ESC`.
* **Modo Camara:** En la ultima pestana de cada modulo se habilita la captura de la webcam en tiempo real; para cerrar el flujo de video y retornar a la interfaz, presionar `ESC` o `Q`.
* **Cierre de la Aplicacion:** Presionar `ESC` o `Q` en el menu principal.

---

## 6. Video Demostrativo

El video explicativo del funcionamiento del software, la arquitectura implementada y la sustentacion del codigo se encuentra disponible en el siguiente enlace:

* **Enlace al video:** [URL_DEL_VIDEO_AQUI]
