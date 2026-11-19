#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// ============================================================================
// 1. LOCALIZADOR AUTOMÁTICO DE IMAGEN (PORTABLE Y RELATIVO)
// ============================================================================
std::string resolverRutaImagen(const std::string& preferida = "input.jpg") {
    std::vector<std::string> candidatos = {
        preferida,
        "../" + preferida,
        "../../" + preferida,
        "../../../" + preferida,
        "input.jpg",
        "../input.jpg",
        "../../input.jpg",
        "lenna.jpg",
        "../lenna.jpg"
    };

    for (const auto& ruta : candidatos) {
        if (fs::exists(ruta)) {
            return ruta;
        }
    }

    // Si no encontró por nombre, buscar el primer archivo de imagen en la carpeta actual o padre
    try {
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".bmp") {
                    return entry.path().string();
                }
            }
        }
        for (const auto& entry : fs::directory_iterator("..")) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".bmp") {
                    return entry.path().string();
                }
            }
        }
    } catch (...) {}

    return preferida;
}

// ============================================================================
// 2. FILTROS DE SUAVIZADO (PASO BAJO)
// ============================================================================
void mostrarFiltrosSuavizado(const cv::Mat& src) {
    cv::Mat blurBox, blurGauss, blurMedian, blurBilateral;

    // Filtro de media (Box Filter) 7x7
    cv::blur(src, blurBox, cv::Size(7, 7));

    // Filtro Gaussiano 7x7 con sigma = 1.5
    cv::GaussianBlur(src, blurGauss, cv::Size(7, 7), 1.5);

    // Filtro de Mediana 7x7 (especial para sal y pimienta)
    cv::medianBlur(src, blurMedian, 7);

    // Filtro Bilateral (preserva bordes nítidos)
    cv::bilateralFilter(src, blurBilateral, 9, 75, 75);

    // Escalar imágenes a dimensiones uniformes para cuadrícula 2x3
    int w = 380, h = 280;
    cv::Mat rOrig, rBox, rGauss, rMed, rBilateral;
    cv::resize(src, rOrig, cv::Size(w, h));
    cv::resize(blurBox, rBox, cv::Size(w, h));
    cv::resize(blurGauss, rGauss, cv::Size(w, h));
    cv::resize(blurMedian, rMed, cv::Size(w, h));
    cv::resize(blurBilateral, rBilateral, cv::Size(w, h));

    // Cuadro informativo
    cv::Mat rInfo(h, w, CV_8UC3, cv::Scalar(30, 30, 30));
    cv::putText(rInfo, "RESUMEN SUAVIZADO:", cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 255), 2);
    cv::putText(rInfo, "- Media: Promedio lineal.", cv::Point(20, 95), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(220, 220, 220), 1);
    cv::putText(rInfo, "- Gauss: Pesos normales.", cv::Point(20, 135), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(220, 220, 220), 1);
    cv::putText(rInfo, "- Mediana: Elimina ruido sal/pim.", cv::Point(20, 175), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(220, 220, 220), 1);
    cv::putText(rInfo, "- Bilateral: Cuida bordes.", cv::Point(20, 215), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(220, 220, 220), 1);

    // Tótulos
    cv::putText(rOrig, "Original", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    cv::putText(rBox, "1. Media (Box 7x7)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 165, 255), 2);
    cv::putText(rGauss, "2. Gaussiano (7x7, s=1.5)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 100, 0), 2);
    cv::putText(rMed, "3. Mediana (7x7)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 0, 255), 2);
    cv::putText(rBilateral, "4. Bilateral (Bordes intactos)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 255), 2);

    cv::Mat fila1, fila2, mosaico;
    cv::hconcat(std::vector<cv::Mat>{rOrig, rBox, rGauss}, fila1);
    cv::hconcat(std::vector<cv::Mat>{rMed, rBilateral, rInfo}, fila2);
    cv::vconcat(fila1, fila2, mosaico);

    cv::imshow("Filtros Espaciales de Suavizado", mosaico);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Filtros Espaciales de Suavizado");
}

// ============================================================================
// 3. FILTROS DE DETECCIÓN DE BORDES Y GRADIENTES (PASO ALTO)
// ============================================================================
void mostrarFiltrosBordes(const cv::Mat& src) {
    cv::Mat gray;
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = src.clone();
    }

    // Suavizado previo para reducir ruido en derivadas
    cv::Mat grayBlur;
    cv::GaussianBlur(gray, grayBlur, cv::Size(3, 3), 0);

    // 1. Sobel en X y Sobel en Y
    cv::Mat gradX, gradY;
    cv::Mat absGradX, absGradY, sobelTotal;
    cv::Sobel(grayBlur, gradX, CV_16S, 1, 0, 3);
    cv::Sobel(grayBlur, gradY, CV_16S, 0, 1, 3);
    cv::convertScaleAbs(gradX, absGradX);
    cv::convertScaleAbs(gradY, absGradY);
    cv::addWeighted(absGradX, 0.5, absGradY, 0.5, 0, sobelTotal);

    // 2. Laplaciano
    cv::Mat laplace, absLaplace;
    cv::Laplacian(grayBlur, laplace, CV_16S, 3);
    cv::convertScaleAbs(laplace, absLaplace);

    // 3. Detector Canny
    cv::Mat canny;
    cv::Canny(grayBlur, canny, 50, 150);

    // Redimensionar para mosaico
    int w = 380, h = 280;
    cv::Mat rOrig, rSobX, rSobY, rSobTot, rLap, rCanny;
    cv::resize(gray, rOrig, cv::Size(w, h));
    cv::resize(absGradX, rSobX, cv::Size(w, h));
    cv::resize(absGradY, rSobY, cv::Size(w, h));
    cv::resize(sobelTotal, rSobTot, cv::Size(w, h));
    cv::resize(absLaplace, rLap, cv::Size(w, h));
    cv::resize(canny, rCanny, cv::Size(w, h));

    // Convertir a BGR para títulos de color
    cv::Mat cOrig, cSobX, cSobY, cSobTot, cLap, cCanny;
    cv::cvtColor(rOrig, cOrig, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rSobX, cSobX, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rSobY, cSobY, cv::COLOR_GRAY2BGR);
    cv::cvtColor(sobelTotal, cSobTot, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rLap, cLap, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rCanny, cCanny, cv::COLOR_GRAY2BGR);

    cv::putText(cOrig, "Original (Grises)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
    cv::putText(cSobX, "Sobel Horizontal (Gx)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 165, 255), 2);
    cv::putText(cSobY, "Sobel Vertical (Gy)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 165, 255), 2);
    cv::putText(cSobTot, "Sobel Magnitud Combinada", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 200, 0), 2);
    cv::putText(cLap, "Laplaciano (2da Derivada)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 0, 255), 2);
    cv::putText(cCanny, "Canny (Histeresis 50-150)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 255), 2);

    cv::Mat fila1, fila2, mosaico;
    cv::hconcat(std::vector<cv::Mat>{cOrig, cSobX, cSobY}, fila1);
    cv::hconcat(std::vector<cv::Mat>{cSobTot, cLap, cCanny}, fila2);
    cv::vconcat(fila1, fila2, mosaico);

    cv::imshow("Deteccion de Bordes y Gradientes", mosaico);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Deteccion de Bordes y Gradientes");
}

// ============================================================================
// 4. FILTROS DE REALCE Y ENFOQUE (SHARPENING)
// ============================================================================
void mostrarFiltrosRealce(const cv::Mat& src) {
    // 1. Kernel de enfoque estándar Laplaciano
    // [  0, -1,  0 ]
    // [ -1,  5, -1 ]
    // [  0, -1,  0 ]
    cv::Mat kernelSharpen = (cv::Mat_<float>(3, 3) <<
         0, -1,  0,
        -1,  5, -1,
         0, -1,  0);
    cv::Mat enfocado;
    cv::filter2D(src, enfocado, src.depth(), kernelSharpen);

    // 2. Kernel de enfoque fuerte
    // [ -1, -1, -1 ]
    // [ -1,  9, -1 ]
    // [ -1, -1, -1 ]
    cv::Mat kernelSharpenStrong = (cv::Mat_<float>(3, 3) <<
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1);
    cv::Mat enfocadoFuerte;
    cv::filter2D(src, enfocadoFuerte, src.depth(), kernelSharpenStrong);

    // 3. Unsharp Masking tradicional: Imagen + factor * (Imagen - Gaussiano)
    cv::Mat suave, mascara, unsharp;
    cv::GaussianBlur(src, suave, cv::Size(0, 0), 3.0);
    cv::addWeighted(src, 1.5, suave, -0.5, 0, unsharp);

    int w = 380, h = 280;
    cv::Mat rOrig, rMod, rFuerte, rUnsharp;
    cv::resize(src, rOrig, cv::Size(w, h));
    cv::resize(enfocado, rMod, cv::Size(w, h));
    cv::resize(enfocadoFuerte, rFuerte, cv::Size(w, h));
    cv::resize(unsharp, rUnsharp, cv::Size(w, h));

    cv::putText(rOrig, "Original", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
    cv::putText(rMod, "Sharpen Estandar", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);
    cv::putText(rFuerte, "Sharpen Fuerte (9x9 c)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 100, 255), 2);
    cv::putText(rUnsharp, "Unsharp Masking (Gauss)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 0, 255), 2);

    cv::Mat fila1, fila2, mosaico;
    cv::hconcat(rOrig, rMod, fila1);
    cv::hconcat(rFuerte, rUnsharp, fila2);
    cv::vconcat(fila1, fila2, mosaico);

    cv::imshow("Realce y Enfoque de Imagenes", mosaico);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Realce y Enfoque de Imagenes");
}

// ============================================================================
// 5. LABORATORIO INTERACTIVO CON TRACKBARS
// ============================================================================
struct LabData {
    cv::Mat src;
    int tipoFiltro = 0;   // 0: Box, 1: Gauss, 2: Mediana, 3: Bilateral, 4: Sobel, 5: Laplace, 6: Canny, 7: Sharpen
    int ksize = 3;        // 1 a 31 (impar)
    int param2 = 50;      // Sigma o umbral Canny
};

static void onTrackbar(int, void* userdata) {
    LabData* data = static_cast<LabData*>(userdata);

    int k = data->ksize;
    if (k % 2 == 0) k += 1;
    if (k < 1) k = 1;

    cv::Mat dst;
    std::string nombreFiltro = "";

    switch (data->tipoFiltro) {
        case 0: // Box Blur
            cv::blur(data->src, dst, cv::Size(k, k));
            nombreFiltro = "Box Blur (Kernel: " + std::to_string(k) + "x" + std::to_string(k) + ")";
            break;
        case 1: { // Gaussiano
            double sigma = std::max(0.1, data->param2 / 10.0);
            cv::GaussianBlur(data->src, dst, cv::Size(k, k), sigma);
            nombreFiltro = "Gaussiano (k: " + std::to_string(k) + ", sigma: " + std::to_string(sigma).substr(0, 3) + ")";
            break;
        }
        case 2: // Mediana
            cv::medianBlur(data->src, dst, k);
            nombreFiltro = "Mediana (k: " + std::to_string(k) + ")";
            break;
        case 3: { // Bilateral
            int d = std::min(15, k);
            double sc = data->param2 * 1.5;
            cv::bilateralFilter(data->src, dst, d, sc, sc);
            nombreFiltro = "Bilateral (d: " + std::to_string(d) + ", sigmaColor: " + std::to_string((int)sc) + ")";
            break;
        }
        case 4: { // Sobel
            cv::Mat gray, gx, gy, agx, agy;
            cv::cvtColor(data->src, gray, cv::COLOR_BGR2GRAY);
            int kSob = std::min(7, k);
            if (kSob % 2 == 0) kSob = 3;
            cv::Sobel(gray, gx, CV_16S, 1, 0, kSob);
            cv::Sobel(gray, gy, CV_16S, 0, 1, kSob);
            cv::convertScaleAbs(gx, agx);
            cv::convertScaleAbs(gy, agy);
            cv::addWeighted(agx, 0.5, agy, 0.5, 0, dst);
            cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
            nombreFiltro = "Sobel (k: " + std::to_string(kSob) + ")";
            break;
        }
        case 5: { // Laplaciano
            cv::Mat gray, lap;
            cv::cvtColor(data->src, gray, cv::COLOR_BGR2GRAY);
            int kLap = std::min(7, k);
            if (kLap % 2 == 0) kLap = 3;
            cv::Laplacian(gray, lap, CV_16S, kLap);
            cv::convertScaleAbs(lap, dst);
            cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
            nombreFiltro = "Laplaciano (k: " + std::to_string(kLap) + ")";
            break;
        }
        case 6: { // Canny
            cv::Mat gray;
            cv::cvtColor(data->src, gray, cv::COLOR_BGR2GRAY);
            int tLow = data->param2;
            int tHigh = tLow * 2;
            cv::Canny(gray, dst, tLow, tHigh);
            cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
            nombreFiltro = "Canny (Umbrales: " + std::to_string(tLow) + " - " + std::to_string(tHigh) + ")";
            break;
        }
        case 7: { // Sharpen interactivo
            float alfa = 1.0f + (data->param2 / 20.0f);
            cv::Mat suave;
            cv::GaussianBlur(data->src, suave, cv::Size(k, k), 0);
            cv::addWeighted(data->src, alfa, suave, -(alfa - 1.0f), 0, dst);
            nombreFiltro = "Sharpening Dinamico (Alfa: " + std::to_string(alfa).substr(0, 3) + ")";
            break;
        }
        default:
            dst = data->src.clone();
            nombreFiltro = "Sin filtro";
            break;
    }

    // Dibujar banner informativo en la parte superior
    cv::Mat visual = dst.clone();
    cv::rectangle(visual, cv::Rect(0, 0, visual.cols, 42), cv::Scalar(20, 20, 20), -1);
    cv::putText(visual, nombreFiltro, cv::Point(15, 28), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 200), 2);

    cv::imshow("Laboratorio Interactivo de Filtros", visual);
}

void ejecutarLaboratorioInteractivo(const cv::Mat& src) {
    LabData data;
    // Redimensionar para interacción fluida en pantalla
    double escala = 640.0 / src.cols;
    cv::resize(src, data.src, cv::Size(), escala, escala);

    const std::string winName = "Laboratorio Interactivo de Filtros";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    cv::createTrackbar("Filtro [0-7]", winName, &data.tipoFiltro, 7, onTrackbar, &data);
    cv::createTrackbar("Kernel (1-31)", winName, &data.ksize, 31, onTrackbar, &data);
    cv::createTrackbar("Param / Sigma", winName, &data.param2, 100, onTrackbar, &data);

    std::cout << "\n--- LABORATORIO DE FILTROS ACTIVADO ---\n";
    std::cout << "Filtros: 0=Box, 1=Gauss, 2=Mediana, 3=Bilateral, 4=Sobel, 5=Laplace, 6=Canny, 7=Sharpen\n";
    std::cout << "Presiona 'ESC' o 'q' para salir al menu.\n";

    onTrackbar(0, &data);

    while (true) {
        char key = static_cast<char>(cv::waitKey(20));
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }

    cv::destroyWindow(winName);
}

// ============================================================================
// 6. FILTROS EN TIEMPO REAL CON LA CÁMARA
// ============================================================================
void procesarFiltrosCamara() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "\n[ERROR] No se pudo abrir la camara web.\n";
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    std::cout << "\n[+] Camara iniciada.\n";
    std::cout << "Controles en vivo (teclas):\n";
    std::cout << " [1] Original   [2] Box Blur   [3] Gaussiano   [4] Mediana\n";
    std::cout << " [5] Sobel      [6] Canny      [7] Sharpen     [8] Estilo Comic\n";
    std::cout << " Presiona 'ESC' o 'q' para volver al menu.\n";

    const std::string winCam = "Camara en Vivo: Original vs Filtro Seleccionado";
    cv::namedWindow(winCam, cv::WINDOW_AUTOSIZE);

    int modo = 3; // Iniciar en Gaussiano por defecto

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat procesado;
        std::string tag = "";

        switch (modo) {
            case 1:
                procesado = frame.clone();
                tag = "Sin filtro (Original)";
                break;
            case 2:
                cv::blur(frame, procesado, cv::Size(9, 9));
                tag = "Box Blur (9x9)";
                break;
            case 3:
                cv::GaussianBlur(frame, procesado, cv::Size(9, 9), 2.0);
                tag = "Gaussiano (9x9, s=2.0)";
                break;
            case 4:
                cv::medianBlur(frame, procesado, 7);
                tag = "Filtro Mediana (7x7)";
                break;
            case 5: {
                cv::Mat gray, gx, gy, agx, agy, sob;
                cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
                cv::Sobel(gray, gx, CV_16S, 1, 0, 3);
                cv::Sobel(gray, gy, CV_16S, 0, 1, 3);
                cv::convertScaleAbs(gx, agx);
                cv::convertScaleAbs(gy, agy);
                cv::addWeighted(agx, 0.5, agy, 0.5, 0, sob);
                cv::cvtColor(sob, procesado, cv::COLOR_GRAY2BGR);
                tag = "Bordes Sobel";
                break;
            }
            case 6: {
                cv::Mat gray, can;
                cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
                cv::Canny(gray, can, 60, 150);
                cv::cvtColor(can, procesado, cv::COLOR_GRAY2BGR);
                tag = "Bordes Canny (60, 150)";
                break;
            }
            case 7: {
                cv::Mat sharpK = (cv::Mat_<float>(3, 3) << 0, -1, 0, -1, 5, -1, 0, -1, 0);
                cv::filter2D(frame, procesado, frame.depth(), sharpK);
                tag = "Sharpening (Laplaciano)";
                break;
            }
            case 8: { // Filtro estilo Cómic / Toon: Bilateral + Canny invertido
                cv::Mat gray, cannyEdges, bilat;
                cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
                cv::medianBlur(gray, gray, 7);
                cv::Canny(gray, cannyEdges, 50, 150);
                cv::bitwise_not(cannyEdges, cannyEdges);
                cv::cvtColor(cannyEdges, cannyEdges, cv::COLOR_GRAY2BGR);

                cv::bilateralFilter(frame, bilat, 9, 75, 75);
                cv::bitwise_and(bilat, cannyEdges, procesado);
                tag = "Efecto Comic (Bilateral + Canny)";
                break;
            }
            default:
                procesado = frame.clone();
                tag = "Original";
                break;
        }

        // Títulos sobre las imágenes
        cv::putText(frame, "Original", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::putText(procesado, tag, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 200, 255), 2);

        cv::Mat comparativa;
        cv::hconcat(frame, procesado, comparativa);
        cv::imshow(winCam, comparativa);

        int key = cv::waitKey(15);
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        } else if (key >= '1' && key <= '8') {
            modo = key - '0';
        }
    }

    cap.release();
    cv::destroyAllWindows();
}

// ============================================================================
// 7. MENÚ PRINCIPAL
// ============================================================================
int main() {
    int opcion = -1;
    std::string rutaImg = resolverRutaImagen("input.jpg");

    while (opcion != 0) {
        std::cout << "\n============================================\n";
        std::cout << "  VISION ARTIFICIAL: TALLER DE FILTROS      \n";
        std::cout << "============================================\n";
        std::cout << "Imagen detectada: " << rutaImg << "\n\n";
        std::cout << "1. Filtros de Suavizado (Media, Gauss, Mediana, Bilateral)\n";
        std::cout << "2. Deteccion de Bordes y Gradientes (Sobel, Laplace, Canny)\n";
        std::cout << "3. Filtros de Realce y Enfoque (Sharpening / Unsharp Mask)\n";
        std::cout << "4. Laboratorio Interactivo con Trackbars (Sliders en vivo)\n";
        std::cout << "5. Filtros aplicados a la Camara en Tiempo Real\n";
        std::cout << "0. Salir\n";
        std::cout << "Selecciona una opcion: ";

        if (!(std::cin >> opcion)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            opcion = -1;
            continue;
        }

        cv::Mat src = cv::imread(rutaImg, cv::IMREAD_COLOR);

        switch (opcion) {
            case 1:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarFiltrosSuavizado(src);
                }
                break;
            case 2:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarFiltrosBordes(src);
                }
                break;
            case 3:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarFiltrosRealce(src);
                }
                break;
            case 4:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    ejecutarLaboratorioInteractivo(src);
                }
                break;
            case 5:
                procesarFiltrosCamara();
                break;
            case 0:
                std::cout << "\nSaliendo del taller de Filtros...\n";
                break;
            default:
                std::cout << "\n[!] Opcion no valida.\n";
                break;
        }
    }

    return 0;
}
