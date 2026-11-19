#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <iomanip>

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
// 2. FUNCIÓN DE RENDERIZADO DE HISTOGRAMA 1 CANAL (GRISES)
// ============================================================================
cv::Mat renderizarHistogramaGrises(const cv::Mat& gray, int hist_w = 400, int hist_h = 300, const std::string& titulo = "Histograma") {
    int histSize = 256;
    float range[] = { 0, 256 };
    const float* histRange = { range };
    cv::Mat hist;

    cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
    cv::normalize(hist, hist, 0, hist_h - 40, cv::NORM_MINMAX);

    cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(25, 25, 25));
    int bin_w = cvRound((double)hist_w / histSize);

    // Dibujar cuadrícula de referencia tenue
    for (int y = 50; y < hist_h - 20; y += 50) {
        cv::line(histImage, cv::Point(0, y), cv::Point(hist_w, y), cv::Scalar(45, 45, 45), 1);
    }
    for (int x = 64; x < hist_w; x += 64) {
        cv::line(histImage, cv::Point(x, 0), cv::Point(x, hist_h - 20), cv::Scalar(45, 45, 45), 1);
    }

    // Dibujar barras del histograma
    for (int i = 0; i < histSize; i++) {
        int val = cvRound(hist.at<float>(i));
        cv::Scalar colorBarra = cv::Scalar(180, 180, 180);
        cv::rectangle(histImage,
                      cv::Point(i * bin_w, hist_h - 20),
                      cv::Point((i + 1) * bin_w, hist_h - 20 - val),
                      colorBarra, -1);
    }

    // Eje base
    cv::line(histImage, cv::Point(0, hist_h - 20), cv::Point(hist_w, hist_h - 20), cv::Scalar(100, 100, 100), 2);
    cv::putText(histImage, "0", cv::Point(5, hist_h - 5), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(160, 160, 160), 1);
    cv::putText(histImage, "128", cv::Point(hist_w / 2 - 12, hist_h - 5), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(160, 160, 160), 1);
    cv::putText(histImage, "255", cv::Point(hist_w - 30, hist_h - 5), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(160, 160, 160), 1);

    // Título superior
    cv::putText(histImage, titulo, cv::Point(15, 25), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 200), 2);

    return histImage;
}

// ============================================================================
// 3. OPCIÓN 1: HISTOGRAMA EN ESCALA DE GRISES Y ESTADÍSTICAS
// ============================================================================
void mostrarHistogramaGrises(const cv::Mat& src) {
    cv::Mat gray;
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = src.clone();
    }

    cv::Scalar media, desviacion;
    cv::meanStdDev(gray, media, desviacion);
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(gray, &minVal, &maxVal, &minLoc, &maxLoc);

    std::cout << "\n--- ESTADISTICAS DE LA IMAGEN (ESCALA DE GRISES) ---\n";
    std::cout << "  Intensidad Media (Media): " << std::fixed << std::setprecision(2) << media[0] << "\n";
    std::cout << "  Desviacion Estandar:      " << desviacion[0] << "\n";
    std::cout << "  Valor Minimo:             " << minVal << "\n";
    std::cout << "  Valor Maximo:             " << maxVal << "\n";
    std::cout << "----------------------------------------------------\n";

    cv::Mat lienzoHist = renderizarHistogramaGrises(gray, 480, 360, "Histograma de Grises");

    // Redimensionar imagen para emparejar alturas
    cv::Mat grayDisplay;
    cv::resize(gray, grayDisplay, cv::Size(480, 360));
    cv::cvtColor(grayDisplay, grayDisplay, cv::COLOR_GRAY2BGR);

    // Escribir estadísticas en la imagen
    cv::rectangle(grayDisplay, cv::Rect(0, 0, grayDisplay.cols, 55), cv::Scalar(20, 20, 20), -1);
    cv::putText(grayDisplay, "Imagen en Escala de Grises", cv::Point(12, 22), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 0), 2);
    std::string statsStr = "Media: " + std::to_string((int)media[0]) + " | Std: " + std::to_string((int)desviacion[0]) +
                           " | Min: " + std::to_string((int)minVal) + " | Max: " + std::to_string((int)maxVal);
    cv::putText(grayDisplay, statsStr, cv::Point(12, 45), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(200, 200, 200), 1);

    cv::Mat comparativa;
    cv::hconcat(grayDisplay, lienzoHist, comparativa);

    cv::imshow("Histograma de Grises con Estadisticas", comparativa);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para volver al menu.\n";
    cv::waitKey(0);
    cv::destroyWindow("Histograma de Grises con Estadisticas");
}

// ============================================================================
// 4. OPCIÓN 2: HISTOGRAMA POR CANALES DE COLOR (RGB / BGR)
// ============================================================================
void mostrarHistogramaColor(const cv::Mat& src) {
    if (src.channels() != 3) {
        std::cerr << "[!] La imagen no tiene 3 canales de color.\n";
        return;
    }

    std::vector<cv::Mat> bgr_planes;
    cv::split(src, bgr_planes);

    int histSize = 256;
    float range[] = { 0, 256 };
    const float* histRange = { range };

    cv::Mat b_hist, g_hist, r_hist;
    cv::calcHist(&bgr_planes[0], 1, 0, cv::Mat(), b_hist, 1, &histSize, &histRange);
    cv::calcHist(&bgr_planes[1], 1, 0, cv::Mat(), g_hist, 1, &histSize, &histRange);
    cv::calcHist(&bgr_planes[2], 1, 0, cv::Mat(), r_hist, 1, &histSize, &histRange);

    int hist_w = 512, hist_h = 380;
    cv::normalize(b_hist, b_hist, 0, hist_h - 40, cv::NORM_MINMAX);
    cv::normalize(g_hist, g_hist, 0, hist_h - 40, cv::NORM_MINMAX);
    cv::normalize(r_hist, r_hist, 0, hist_h - 40, cv::NORM_MINMAX);

    cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(20, 20, 20));
    int bin_w = cvRound((double)hist_w / histSize);

    // Dibujar curvas de cada canal
    for (int i = 1; i < histSize; i++) {
        // Azul
        cv::line(histImage,
                 cv::Point(bin_w * (i - 1), hist_h - 20 - cvRound(b_hist.at<float>(i - 1))),
                 cv::Point(bin_w * i, hist_h - 20 - cvRound(b_hist.at<float>(i))),
                 cv::Scalar(255, 60, 20), 2, cv::LINE_AA);
        // Verde
        cv::line(histImage,
                 cv::Point(bin_w * (i - 1), hist_h - 20 - cvRound(g_hist.at<float>(i - 1))),
                 cv::Point(bin_w * i, hist_h - 20 - cvRound(g_hist.at<float>(i))),
                 cv::Scalar(20, 220, 30), 2, cv::LINE_AA);
        // Rojo
        cv::line(histImage,
                 cv::Point(bin_w * (i - 1), hist_h - 20 - cvRound(r_hist.at<float>(i - 1))),
                 cv::Point(bin_w * i, hist_h - 20 - cvRound(r_hist.at<float>(i))),
                 cv::Scalar(30, 40, 255), 2, cv::LINE_AA);
    }

    // Eje y marcas
    cv::line(histImage, cv::Point(0, hist_h - 20), cv::Point(hist_w, hist_h - 20), cv::Scalar(100, 100, 100), 2);
    cv::putText(histImage, "0", cv::Point(5, hist_h - 5), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(160, 160, 160), 1);
    cv::putText(histImage, "128", cv::Point(hist_w / 2 - 12, hist_h - 5), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(160, 160, 160), 1);
    cv::putText(histImage, "255", cv::Point(hist_w - 30, hist_h - 5), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(160, 160, 160), 1);

    // Leyenda de colores
    cv::putText(histImage, "Histograma Canales RGB", cv::Point(15, 25), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 255, 255), 2);
    cv::putText(histImage, "Canal B (Azul)", cv::Point(15, 55), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(255, 100, 50), 1);
    cv::putText(histImage, "Canal G (Verde)", cv::Point(160, 55), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(50, 220, 50), 1);
    cv::putText(histImage, "Canal R (Rojo)", cv::Point(315, 55), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(50, 60, 255), 1);

    cv::Mat rSrc;
    cv::resize(src, rSrc, cv::Size(hist_w, hist_h));
    cv::putText(rSrc, "Imagen Original Color", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.70, cv::Scalar(0, 255, 0), 2);

    cv::Mat comparativa;
    cv::hconcat(rSrc, histImage, comparativa);

    cv::imshow("Histograma por Canales de Color BGR", comparativa);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para volver al menu.\n";
    cv::waitKey(0);
    cv::destroyWindow("Histograma por Canales de Color BGR");
}

// ============================================================================
// 5. OPCIÓN 3: ECUALIZACIÓN DE HISTOGRAMA (GLOBAL vs CLAHE)
// ============================================================================
void mostrarEcualizacion(const cv::Mat& src) {
    cv::Mat gray;
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = src.clone();
    }

    // 1. Ecualización Global
    cv::Mat eqGlobal;
    cv::equalizeHist(gray, eqGlobal);

    // 2. Ecualización Adaptativa Limitada por Contraste (CLAHE)
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));
    cv::Mat eqCLAHE;
    clahe->apply(gray, eqCLAHE);

    // Generar los 3 histogramas
    int w = 360, h = 240;
    cv::Mat hOrig = renderizarHistogramaGrises(gray, w, h, "Hist: Original");
    cv::Mat hGlobal = renderizarHistogramaGrises(eqGlobal, w, h, "Hist: Ecualizado Global");
    cv::Mat hCLAHE = renderizarHistogramaGrises(eqCLAHE, w, h, "Hist: CLAHE (Adaptativo)");

    cv::Mat rOrig, rGlobal, rCLAHE;
    cv::resize(gray, rOrig, cv::Size(w, h));
    cv::resize(eqGlobal, rGlobal, cv::Size(w, h));
    cv::resize(eqCLAHE, rCLAHE, cv::Size(w, h));

    cv::cvtColor(rOrig, rOrig, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rGlobal, rGlobal, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rCLAHE, rCLAHE, cv::COLOR_GRAY2BGR);

    cv::putText(rOrig, "1. Original", cv::Point(12, 25), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
    cv::putText(rGlobal, "2. Ecualizacion Global", cv::Point(12, 25), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);
    cv::putText(rCLAHE, "3. CLAHE (Local 8x8)", cv::Point(12, 25), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 100, 0), 2);

    cv::Mat filaImg, filaHist, mosaico;
    cv::hconcat(std::vector<cv::Mat>{rOrig, rGlobal, rCLAHE}, filaImg);
    cv::hconcat(std::vector<cv::Mat>{hOrig, hGlobal, hCLAHE}, filaHist);
    cv::vconcat(filaImg, filaHist, mosaico);

    cv::imshow("Comparativa de Ecualizacion de Histograma (Global vs CLAHE)", mosaico);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Comparativa de Ecualizacion de Histograma (Global vs CLAHE)");
}

// ============================================================================
// 6. OPCIÓN 4: AJUSTE INTERACTIVO DE CONTRASTE Y BRILLO CON HISTOGRAMA EN VIVO
// ============================================================================
struct BrightContrastData {
    cv::Mat srcGray;
    int alfa_slider = 10; // Contraste: 1 a 30 -> 0.1 a 3.0
    int beta_slider = 100; // Brillo: 0 a 200 -> centrado en 100 -> [-100, +100]
};

static void onBrightContrastChange(int, void* userdata) {
    BrightContrastData* data = static_cast<BrightContrastData*>(userdata);

    double alfa = data->alfa_slider / 10.0;
    int beta = data->beta_slider - 100;

    cv::Mat dst;
    data->srcGray.convertTo(dst, -1, alfa, beta);

    cv::Mat histImg = renderizarHistogramaGrises(dst, 420, 320, "Histograma Dinamico");

    cv::Mat dstDisplay;
    cv::resize(dst, dstDisplay, cv::Size(420, 320));
    cv::cvtColor(dstDisplay, dstDisplay, cv::COLOR_GRAY2BGR);

    cv::rectangle(dstDisplay, cv::Rect(0, 0, dstDisplay.cols, 42), cv::Scalar(20, 20, 20), -1);
    std::string infoStr = "Alfa (Contraste): " + std::to_string(alfa).substr(0, 3) +
                          " | Beta (Brillo): " + (beta >= 0 ? "+" : "") + std::to_string(beta);
    cv::putText(dstDisplay, infoStr, cv::Point(15, 27), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 255), 2);

    cv::Mat visor;
    cv::hconcat(dstDisplay, histImg, visor);
    cv::imshow("Ajuste Interactivo de Contraste y Brillo", visor);
}

void ejecutarAjusteInteractivo(const cv::Mat& src) {
    BrightContrastData data;
    if (src.channels() == 3) {
        cv::cvtColor(src, data.srcGray, cv::COLOR_BGR2GRAY);
    } else {
        data.srcGray = src.clone();
    }

    const std::string winName = "Ajuste Interactivo de Contraste y Brillo";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    cv::createTrackbar("Contraste (Alfa x10)", winName, &data.alfa_slider, 30, onBrightContrastChange, &data);
    cv::createTrackbar("Brillo (Beta - 100)", winName, &data.beta_slider, 200, onBrightContrastChange, &data);

    std::cout << "\n--- AJUSTE INTERACTIVO ACTIVADO ---\n";
    std::cout << "Mueve los sliders para modificar Contraste y Brillo.\n";
    std::cout << "Presiona 'ESC' o 'q' para salir al menu.\n";

    onBrightContrastChange(0, &data);

    while (true) {
        char key = static_cast<char>(cv::waitKey(20));
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }

    cv::destroyWindow(winName);
}

// ============================================================================
// 7. OPCIÓN 5: HISTOGRAMA Y ECUALIZACIÓN EN TIEMPO REAL CON LA CÁMARA
// ============================================================================
void procesarCamaraHistograma() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "\n[ERROR] No se pudo abrir la camara web.\n";
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    std::cout << "\n[+] Camara iniciada.\n";
    std::cout << "Mostrando: Cuadro original | Histograma en vivo | Ecualizado CLAHE.\n";
    std::cout << "Presiona 'ESC' o 'q' para volver al menu.\n";

    const std::string winName = "Camara: Original | Histograma Dinamico | CLAHE";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        cv::Mat eqClahe;
        clahe->apply(gray, eqClahe);

        int w = 360, h = 270;
        cv::Mat rOrig, rClahe;
        cv::resize(frame, rOrig, cv::Size(w, h));
        cv::resize(eqClahe, rClahe, cv::Size(w, h));
        cv::cvtColor(rClahe, rClahe, cv::COLOR_GRAY2BGR);

        cv::Mat hLive = renderizarHistogramaGrises(gray, w, h, "Histograma Vivo");

        cv::putText(rOrig, "Camara en Vivo", cv::Point(12, 25), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 0), 2);
        cv::putText(rClahe, "Mejora con CLAHE", cv::Point(12, 25), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 220, 255), 2);

        cv::Mat visor;
        cv::hconcat(std::vector<cv::Mat>{rOrig, hLive, rClahe}, visor);

        cv::imshow(winName, visor);

        char key = static_cast<char>(cv::waitKey(15));
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
}

// ============================================================================
// 8. MENÚ PRINCIPAL
// ============================================================================
int main() {
    int opcion = -1;
    std::string rutaImg = resolverRutaImagen("input.jpg");

    while (opcion != 0) {
        std::cout << "\n============================================\n";
        std::cout << "  VISION ARTIFICIAL: TALLER DE HISTOGRAMAS  \n";
        std::cout << "============================================\n";
        std::cout << "Imagen detectada: " << rutaImg << "\n\n";
        std::cout << "1. Histograma en Escala de Grises y Estadisticas\n";
        std::cout << "2. Histograma por Canales de Color (RGB / BGR)\n";
        std::cout << "3. Ecualizacion de Histograma (Global vs CLAHE)\n";
        std::cout << "4. Ajuste Interactivo de Contraste y Brillo (Trackbars)\n";
        std::cout << "5. Histograma y Ecualizacion con Camara en Vivo\n";
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
                    mostrarHistogramaGrises(src);
                }
                break;
            case 2:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarHistogramaColor(src);
                }
                break;
            case 3:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarEcualizacion(src);
                }
                break;
            case 4:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    ejecutarAjusteInteractivo(src);
                }
                break;
            case 5:
                procesarCamaraHistograma();
                break;
            case 0:
                std::cout << "\nSaliendo del taller de Histogramas...\n";
                break;
            default:
                std::cout << "\n[!] Opcion no valida.\n";
                break;
        }
    }

    return 0;
}
