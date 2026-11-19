#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// 1. LOCALIZADOR AUTOMÁTICO DE IMAGEN
// ============================================================================
std::string resolverRutaImagen(const std::string& preferida = "input.jpg") {
    std::vector<std::string> candidatos = {
        preferida,
        "../" + preferida,
        "../../" + preferida,
        "lenna.jpg",
        "../lenna.jpg",
        "recibo.jpg",
        "../recibo.jpg"
    };

    for (const auto& ruta : candidatos) {
        if (fs::exists(ruta)) {
            return ruta;
        }
    }

    // Si no encontró por nombre, buscar el primer .jpg o .png disponible
    for (const auto& entry : fs::directory_iterator(".")) {
        std::string ext = entry.path().extension().string();
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg") {
            return entry.path().string();
        }
    }
    for (const auto& entry : fs::directory_iterator("..")) {
        std::string ext = entry.path().extension().string();
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg") {
            return entry.path().string();
        }
    }

    return preferida;
}

// ============================================================================
// 2. GENERADOR DE HISTOGRAMA CON LÍNEA DE UMBRAL DE OTSU
// ============================================================================
cv::Mat generarImagenHistograma(const cv::Mat& gray, double umbralOtsu = -1.0) {
    int histSize = 256;
    float range[] = { 0, 256 };
    const float* histRange = { range };
    cv::Mat hist;

    cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
    cv::normalize(hist, hist, 0, 400, cv::NORM_MINMAX);

    int hist_w = 512, hist_h = 400;
    int bin_w = cvRound((double)hist_w / histSize);
    cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(245, 245, 245));

    // Dibujar curvas del histograma
    for (int i = 1; i < histSize; i++) {
        cv::line(histImage,
            cv::Point(bin_w * (i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
            cv::Point(bin_w * i, hist_h - cvRound(hist.at<float>(i))),
            cv::Scalar(40, 40, 40), 2, cv::LINE_AA);
    }

    // Dibujar línea del umbral óptimo de Otsu si está disponible
    if (umbralOtsu >= 0) {
        int xOtsu = cvRound(umbralOtsu * bin_w);
        cv::line(histImage, cv::Point(xOtsu, 0), cv::Point(xOtsu, hist_h), cv::Scalar(0, 0, 255), 2);
        cv::putText(histImage, "Umbral Otsu: " + std::to_string((int)umbralOtsu),
                    cv::Point(std::min(xOtsu + 10, hist_w - 180), 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 0, 200), 2);
    }

    return histImage;
}

// ============================================================================
// 3. COMPARATIVA EN PANTALLA: GLOBAL, OTSU Y ADAPTATIVAS
// ============================================================================
void procesarBinarizacionImagen(const std::string& ruta) {
    cv::Mat src = cv::imread(ruta, cv::IMREAD_COLOR);
    if (src.empty()) {
        std::cerr << "\n[ERROR] No se pudo abrir la imagen: " << ruta << "\n";
        return;
    }

    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    // 1. Umbralización Global Fija (T = 127)
    cv::Mat dstGlobal;
    cv::threshold(gray, dstGlobal, 127, 255, cv::THRESH_BINARY);

    // 2. Umbralización Global de Otsu (Cálculo automático de T óptimo)
    cv::Mat dstOtsu;
    double otsuThresh = cv::threshold(gray, dstOtsu, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    std::cout << "\n[+] Umbral optimo calculado por Otsu: " << otsuThresh << std::endl;

    // 3. Umbralización Adaptativa - Media (Vecindario local de 11x11, C = 2)
    cv::Mat dstAdaptMedia;
    cv::adaptiveThreshold(gray, dstAdaptMedia, 255, cv::ADAPTIVE_THRESH_MEAN_C,
                          cv::THRESH_BINARY, 11, 2);

    // 4. Umbralización Adaptativa - Gaussiana (Ponderación por campana de Gauss)
    cv::Mat dstAdaptGauss;
    cv::adaptiveThreshold(gray, dstAdaptGauss, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY, 11, 2);

    // Redimensionar las imágenes a tamaño uniforme para visualización 2x2
    int w = 420;
    int h = 340;
    cv::Mat gRes, otsuRes, mediaRes, gaussRes;
    cv::resize(dstGlobal, gRes, cv::Size(w, h));
    cv::resize(dstOtsu, otsuRes, cv::Size(w, h));
    cv::resize(dstAdaptMedia, mediaRes, cv::Size(w, h));
    cv::resize(dstAdaptGauss, gaussRes, cv::Size(w, h));

    // Convertir a 3 canales para agregar rótulos con color
    cv::Mat gColor, otsuColor, mediaColor, gaussColor;
    cv::cvtColor(gRes, gColor, cv::COLOR_GRAY2BGR);
    cv::cvtColor(otsuRes, otsuColor, cv::COLOR_GRAY2BGR);
    cv::cvtColor(mediaRes, mediaColor, cv::COLOR_GRAY2BGR);
    cv::cvtColor(gaussRes, gaussColor, cv::COLOR_GRAY2BGR);

    // Colocar etiquetas explicativas
    cv::putText(gColor, "1. Global Fija (T = 127)", cv::Point(15, 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 0, 255), 2);
    cv::putText(otsuColor, "2. Metodo Otsu (T = " + std::to_string((int)otsuThresh) + ")", cv::Point(15, 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 0, 0), 2);
    cv::putText(mediaColor, "3. Adaptativa Media (11x11)", cv::Point(15, 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 140, 0), 2);
    cv::putText(gaussColor, "4. Adaptativa Gaussiana (11x11)", cv::Point(15, 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 140, 0), 2);

    // Concatenar cuadrícula 2x2
    cv::Mat filaSup, filaInf, mosaicoFinal;
    cv::hconcat(gColor, otsuColor, filaSup);
    cv::hconcat(mediaColor, gaussColor, filaInf);
    cv::vconcat(filaSup, filaInf, mosaicoFinal);

    // Mostrar histograma e imagen binarizada
    cv::Mat lienzoHist = generarImagenHistograma(gray, otsuThresh);

    cv::imshow("Comparativa de Binarizacion (Global vs Otsu vs Adaptativas)", mosaicoFinal);
    cv::imshow("Histograma de Frecuencia de Tonos (Otsu)", lienzoHist);

    std::cout << "[+] Presiona cualquier tecla en las ventanas para volver al menu.\n";
    cv::waitKey(0);
    cv::destroyAllWindows();
}

// ============================================================================
// 4. BINARIZACIÓN EN TIEMPO REAL CON LA CÁMARA
// ============================================================================
void procesarBinarizacionCamara() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "\n[ERROR] No se pudo abrir la camara web.\n";
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    std::cout << "\n[+] Camara iniciada.\n";
    std::cout << "[+] Presiona 'ESC' o 'q' para salir al menu.\n";

    std::string nombreVentana = "Camara en Vivo: Original | Otsu | Adaptativa Gaussiana";
    cv::namedWindow(nombreVentana, cv::WINDOW_AUTOSIZE);

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Calcular Otsu y Adaptativa Gaussiana por fotograma
        cv::Mat bOtsu, bGauss;
        cv::threshold(gray, bOtsu, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::adaptiveThreshold(gray, bGauss, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY, 11, 2);

        // Convertir para concatenar lado a lado
        cv::Mat fOtsu, fGauss;
        cv::cvtColor(bOtsu, fOtsu, cv::COLOR_GRAY2BGR);
        cv::cvtColor(bGauss, fGauss, cv::COLOR_GRAY2BGR);

        // Etiquetas
        cv::putText(frame, "Original", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::putText(fOtsu, "Otsu (Global)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
        cv::putText(fGauss, "Adaptativa Gauss", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 100, 0), 2);

        cv::Mat visor;
        cv::hconcat(frame, fOtsu, visor);
        cv::hconcat(visor, fGauss, visor);

        // Ajustar escala si queda muy ancha en la pantalla
        cv::resize(visor, visor, cv::Size(), 0.7, 0.7);

        cv::imshow(nombreVentana, visor);

        char key = static_cast<char>(cv::waitKey(20));
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
}

// ============================================================================
// 5. MENÚ PRINCIPAL
// ============================================================================
int main() {
    int opcion = -1;

    while (opcion != 0) {
        std::cout << "\n============================================\n";
        std::cout << "  VISION ARTIFICIAL: TALLER DE BINARIZACION \n";
        std::cout << "============================================\n";
        std::cout << "1. Binarizar imagen (Global vs Otsu vs Adaptativas)\n";
        std::cout << "2. Binarizacion con camara en tiempo real\n";
        std::cout << "0. Salir\n";
        std::cout << "Selecciona una opcion: ";

        if (!(std::cin >> opcion)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            opcion = -1;
            continue;
        }

        switch (opcion) {
            case 1: {
                std::string rutaImg = resolverRutaImagen("input.jpg");
                std::cout << "\n[+] Procesando imagen encontrada: " << rutaImg << std::endl;
                procesarBinarizacionImagen(rutaImg);
                break;
            }
            case 2:
                procesarBinarizacionCamara();
                break;
            case 0:
                std::cout << "\nSaliendo...\n";
                break;
            default:
                std::cout << "\n[!] Opcion no valida.\n";
                break;
        }
    }

    return 0;
}