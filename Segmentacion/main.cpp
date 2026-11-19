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
// 2. OPCIÓN 1: SEGMENTACIÓN INTERACTIVA POR COLOR EN ESPACIO HSV
// ============================================================================
struct HsvLabData {
    cv::Mat src;
    cv::Mat hsv;
    int hMin = 0, hMax = 179;
    int sMin = 50, sMax = 255;
    int vMin = 50, vMax = 255;
    int preset = 0; // 0: Manual, 1: Rojo, 2: Verde, 3: Azul, 4: Amarillo
};

static void onHsvChange(int, void* userdata) {
    HsvLabData* data = static_cast<HsvLabData*>(userdata);

    cv::Mat mask;
    // Manejo especial para rojo en HSV (el tono rojo envuelve el 0 y el 180)
    if (data->hMin > data->hMax) {
        cv::Mat mask1, mask2;
        cv::inRange(data->hsv, cv::Scalar(data->hMin, data->sMin, data->vMin),
                    cv::Scalar(179, data->sMax, data->vMax), mask1);
        cv::inRange(data->hsv, cv::Scalar(0, data->sMin, data->vMin),
                    cv::Scalar(data->hMax, data->sMax, data->vMax), mask2);
        cv::bitwise_or(mask1, mask2, mask);
    } else {
        cv::inRange(data->hsv, cv::Scalar(data->hMin, data->sMin, data->vMin),
                    cv::Scalar(data->hMax, data->sMax, data->vMax), mask);
    }

    // Aplicar apertura morfológica leve para eliminar pequeños falsos positivos
    cv::Mat k = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, k);

    // Aislar objeto con bitwise_and
    cv::Mat resultado;
    cv::bitwise_and(data->src, data->src, resultado, mask);

    // Redimensionar para mostrar 3 vistas
    int w = 380, h = 280;
    cv::Mat rSrc, rMask, rRes;
    cv::resize(data->src, rSrc, cv::Size(w, h));
    cv::resize(mask, rMask, cv::Size(w, h));
    cv::resize(resultado, rRes, cv::Size(w, h));

    cv::cvtColor(rMask, rMask, cv::COLOR_GRAY2BGR);

    cv::putText(rSrc, "1. Imagen Original", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 0), 2);
    cv::putText(rMask, "2. Mascara Binaria HSV", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 180, 255), 2);
    cv::putText(rRes, "3. Objeto Segmentado", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 255), 2);

    cv::Mat visor;
    cv::hconcat(std::vector<cv::Mat>{rSrc, rMask, rRes}, visor);

    cv::imshow("Segmentacion por Color HSV", visor);
}

void ejecutarLaboratorioHSV(const cv::Mat& src) {
    HsvLabData data;
    double escala = 640.0 / src.cols;
    cv::resize(src, data.src, cv::Size(), escala, escala);
    cv::cvtColor(data.src, data.hsv, cv::COLOR_BGR2HSV);

    const std::string winName = "Segmentacion por Color HSV";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    cv::createTrackbar("H Min (0-179)", winName, &data.hMin, 179, onHsvChange, &data);
    cv::createTrackbar("H Max (0-179)", winName, &data.hMax, 179, onHsvChange, &data);
    cv::createTrackbar("S Min (0-255)", winName, &data.sMin, 255, onHsvChange, &data);
    cv::createTrackbar("S Max (0-255)", winName, &data.sMax, 255, onHsvChange, &data);
    cv::createTrackbar("V Min (0-255)", winName, &data.vMin, 255, onHsvChange, &data);
    cv::createTrackbar("V Max (0-255)", winName, &data.vMax, 255, onHsvChange, &data);

    std::cout << "\n--- LABORATORIO DE SEGMENTACION HSV ---\n";
    std::cout << "Atajos de Presets por teclado:\n";
    std::cout << " [R] Preset Rojo    [G] Preset Verde    [B] Preset Azul    [Y] Preset Amarillo\n";
    std::cout << " Presiona 'ESC' o 'q' para volver al menu.\n";

    onHsvChange(0, &data);

    while (true) {
        char key = static_cast<char>(cv::waitKey(20));
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        } else if (key == 'r' || key == 'R') { // Rojo
            data.hMin = 165; data.hMax = 15; data.sMin = 100; data.sMax = 255; data.vMin = 70; data.vMax = 255;
            cv::setTrackbarPos("H Min (0-179)", winName, data.hMin);
            cv::setTrackbarPos("H Max (0-179)", winName, data.hMax);
            onHsvChange(0, &data);
        } else if (key == 'g' || key == 'G') { // Verde
            data.hMin = 35; data.hMax = 85; data.sMin = 50; data.sMax = 255; data.vMin = 50; data.vMax = 255;
            cv::setTrackbarPos("H Min (0-179)", winName, data.hMin);
            cv::setTrackbarPos("H Max (0-179)", winName, data.hMax);
            onHsvChange(0, &data);
        } else if (key == 'b' || key == 'B') { // Azul
            data.hMin = 95; data.hMax = 135; data.sMin = 60; data.sMax = 255; data.vMin = 50; data.vMax = 255;
            cv::setTrackbarPos("H Min (0-179)", winName, data.hMin);
            cv::setTrackbarPos("H Max (0-179)", winName, data.hMax);
            onHsvChange(0, &data);
        } else if (key == 'y' || key == 'Y') { // Amarillo
            data.hMin = 20; data.hMax = 35; data.sMin = 80; data.sMax = 255; data.vMin = 80; data.vMax = 255;
            cv::setTrackbarPos("H Min (0-179)", winName, data.hMin);
            cv::setTrackbarPos("H Max (0-179)", winName, data.hMax);
            onHsvChange(0, &data);
        }
    }

    cv::destroyWindow(winName);
}

// ============================================================================
// 3. OPCIÓN 2: SEGMENTACIÓN POR ALGORITMO WATERSHED (CUENCAS HIDROGRÁFICAS)
// ============================================================================
void mostrarSegmentacionWatershed(const cv::Mat& src) {
    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    // 1. Binarización de Otsu invertida (asumiendo objetos más oscuros o claros)
    cv::Mat bin;
    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // 2. Eliminación de ruido con apertura morfológica
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::Mat opening;
    cv::morphologyEx(bin, opening, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 2);

    // 3. Fondo seguro (Sure Background) dilatando los objetos
    cv::Mat sure_bg;
    cv::dilate(opening, sure_bg, kernel, cv::Point(-1, -1), 3);

    // 4. Objetos seguros (Sure Foreground) con la Transformada de Distancia Euclidiana
    cv::Mat dist_transform;
    cv::distanceTransform(opening, dist_transform, cv::DIST_L2, 5);
    double minVal, maxVal;
    cv::minMaxLoc(dist_transform, &minVal, &maxVal);

    cv::Mat sure_fg;
    cv::threshold(dist_transform, sure_fg, 0.5 * maxVal, 255, 0);
    sure_fg.convertTo(sure_fg, CV_8U);

    // 5. Región desconocida (Unknown = Sure BG - Sure FG)
    cv::Mat unknown;
    cv::subtract(sure_bg, sure_fg, unknown);

    // 6. Etiquetado de marcadores de componentes conectados
    cv::Mat markers;
    int nLabels = cv::connectedComponents(sure_fg, markers);

    // Sumar 1 a todos los marcadores para que el fondo seguro sea 1 y no 0
    markers = markers + 1;
    // Marcar la región desconocida con 0
    for (int y = 0; y < unknown.rows; y++) {
        for (int x = 0; x < unknown.cols; x++) {
            if (unknown.at<uchar>(y, x) == 255) {
                markers.at<int>(y, x) = 0;
            }
        }
    }

    // 7. Aplicar algoritmo Watershed
    cv::Mat srcWatershed = src.clone();
    cv::watershed(srcWatershed, markers);

    // 8. Colorear las cuencas y marcar las líneas divisorias en rojo intenso
    cv::Mat resultadoWatershed = src.clone();
    std::vector<cv::Vec3b> colores;
    colores.push_back(cv::Vec3b(0, 0, 0)); // fondo
    for (int i = 1; i <= nLabels; i++) {
        colores.push_back(cv::Vec3b(rand() % 200 + 40, rand() % 200 + 40, rand() % 200 + 40));
    }

    cv::Mat cuencasColoreadas = cv::Mat::zeros(markers.size(), CV_8UC3);
    for (int i = 0; i < markers.rows; i++) {
        for (int j = 0; j < markers.cols; j++) {
            int idx = markers.at<int>(i, j);
            if (idx == -1) {
                resultadoWatershed.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 255); // Línea divisoria roja
                cuencasColoreadas.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 255);
            } else if (idx > 0 && idx <= nLabels) {
                cuencasColoreadas.at<cv::Vec3b>(i, j) = colores[idx % colores.size()];
            }
        }
    }

    // Visualización didáctica 2x2
    int w = 400, h = 300;
    cv::Mat rOrig, rDist, rCuencas, rFinal;
    cv::resize(src, rOrig, cv::Size(w, h));

    cv::Mat distNorm;
    cv::normalize(dist_transform, distNorm, 0, 255, cv::NORM_MINMAX);
    distNorm.convertTo(distNorm, CV_8U);
    cv::applyColorMap(distNorm, distNorm, cv::COLORMAP_JET);
    cv::resize(distNorm, rDist, cv::Size(w, h));

    cv::resize(cuencasColoreadas, rCuencas, cv::Size(w, h));
    cv::resize(resultadoWatershed, rFinal, cv::Size(w, h));

    cv::putText(rOrig, "1. Original", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
    cv::putText(rDist, "2. Transformada Distancia (Centros)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(255, 255, 255), 2);
    cv::putText(rCuencas, "3. Cuencas Marcadas (" + std::to_string(nLabels - 1) + " objetos)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(255, 255, 255), 2);
    cv::putText(rFinal, "4. Lineas Divisorias Watershed", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 0, 255), 2);

    cv::Mat fila1, fila2, mosaico;
    cv::hconcat(rOrig, rDist, fila1);
    cv::hconcat(rCuencas, rFinal, fila2);
    cv::vconcat(fila1, fila2, mosaico);

    cv::imshow("Segmentacion por Algoritmo Watershed", mosaico);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Segmentacion por Algoritmo Watershed");
}

// ============================================================================
// 4. OPCIÓN 3: SEGMENTACIÓN POR CUANTIZACIÓN K-MEANS
// ============================================================================
void mostrarSegmentacionKMeans(const cv::Mat& src) {
    // Redimensionar para acelerar el cálculo de clústeres
    cv::Mat resized;
    double scale = 400.0 / src.cols;
    cv::resize(src, resized, cv::Size(), scale, scale);

    // Convertir matriz a muestra 2D de Nx3 píxeles flotantes
    cv::Mat samples(resized.rows * resized.cols, 3, CV_32F);
    for (int y = 0; y < resized.rows; y++) {
        for (int x = 0; x < resized.cols; x++) {
            for (int z = 0; z < 3; z++) {
                samples.at<float>(y + x * resized.rows, z) = resized.at<cv::Vec3b>(y, x)[z];
            }
        }
    }

    std::vector<int> kValues = { 2, 3, 5 };
    std::vector<cv::Mat> resultadosK;

    cv::Mat rOrig;
    cv::resize(src, rOrig, cv::Size(380, 280));
    cv::putText(rOrig, "Original", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
    resultadosK.push_back(rOrig);

    cv::TermCriteria criteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0);

    for (int k : kValues) {
        cv::Mat labels;
        cv::Mat centers;
        cv::kmeans(samples, k, labels, criteria, 3, cv::KMEANS_PP_CENTERS, centers);

        cv::Mat segmented(resized.size(), resized.type());
        for (int y = 0; y < resized.rows; y++) {
            for (int x = 0; x < resized.cols; x++) {
                int cluster_idx = labels.at<int>(y + x * resized.rows, 0);
                segmented.at<cv::Vec3b>(y, x)[0] = cv::saturate_cast<uchar>(centers.at<float>(cluster_idx, 0));
                segmented.at<cv::Vec3b>(y, x)[1] = cv::saturate_cast<uchar>(centers.at<float>(cluster_idx, 1));
                segmented.at<cv::Vec3b>(y, x)[2] = cv::saturate_cast<uchar>(centers.at<float>(cluster_idx, 2));
            }
        }

        cv::Mat rSeg;
        cv::resize(segmented, rSeg, cv::Size(380, 280));
        cv::putText(rSeg, "K-Means (K = " + std::to_string(k) + " colores)", cv::Point(12, 28),
                    cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 255), 2);
        resultadosK.push_back(rSeg);
    }

    cv::Mat fila1, fila2, mosaico;
    cv::hconcat(resultadosK[0], resultadosK[1], fila1);
    cv::hconcat(resultadosK[2], resultadosK[3], fila2);
    cv::vconcat(fila1, fila2, mosaico);

    cv::imshow("Segmentacion por Clustered K-Means", mosaico);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Segmentacion por Clustered K-Means");
}

// ============================================================================
// 5. OPCIÓN 4: SEGMENTACIÓN DE COLOR EN TIEMPO REAL CON LA CÁMARA
// ============================================================================
void procesarCamaraSegmentacion() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "\n[ERROR] No se pudo abrir la camara web.\n";
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    std::cout << "\n[+] Camara iniciada.\n";
    std::cout << "Presiona teclas para cambiar de color objetivo:\n";
    std::cout << " [R] Detectar Rojo    [G] Detectar Verde    [B] Detectar Azul\n";
    std::cout << " Presiona 'ESC' o 'q' para volver al menu.\n";

    const std::string winName = "Camara en Vivo: Original con Tracking | Mascara HSV";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    char colorObjetivo = 'B'; // Azul por defecto

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat hsv, mask;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        std::string labelColor = "";
        if (colorObjetivo == 'R') {
            cv::Mat m1, m2;
            cv::inRange(hsv, cv::Scalar(0, 100, 70), cv::Scalar(10, 255, 255), m1);
            cv::inRange(hsv, cv::Scalar(170, 100, 70), cv::Scalar(179, 255, 255), m2);
            cv::bitwise_or(m1, m2, mask);
            labelColor = "Tracking: ROJO";
        } else if (colorObjetivo == 'G') {
            cv::inRange(hsv, cv::Scalar(35, 70, 50), cv::Scalar(85, 255, 255), mask);
            labelColor = "Tracking: VERDE";
        } else {
            cv::inRange(hsv, cv::Scalar(95, 80, 50), cv::Scalar(135, 255, 255), mask);
            labelColor = "Tracking: AZUL";
        }

        // Limpieza morfológica de la máscara
        cv::Mat k = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, k);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, k);

        // Encontrar contornos del objeto de mayor área para seguimiento
        std::vector<std::vector<cv::Point>> contornos;
        cv::findContours(mask.clone(), contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        cv::Mat frameSeg = frame.clone();
        double maxArea = 0;
        int idxMax = -1;
        for (size_t i = 0; i < contornos.size(); i++) {
            double a = cv::contourArea(contornos[i]);
            if (a > 500 && a > maxArea) {
                maxArea = a;
                idxMax = static_cast<int>(i);
            }
        }

        if (idxMax >= 0) {
            // Dibujar contorno y caja delimitadora
            cv::drawContours(frameSeg, contornos, idxMax, cv::Scalar(0, 255, 0), 2);
            cv::Rect bbox = cv::boundingRect(contornos[idxMax]);
            cv::rectangle(frameSeg, bbox, cv::Scalar(0, 255, 255), 2);

            // Centroide
            cv::Moments m = cv::moments(contornos[idxMax]);
            if (m.m00 > 0) {
                int cx = static_cast<int>(m.m10 / m.m00);
                int cy = static_cast<int>(m.m01 / m.m00);
                cv::circle(frameSeg, cv::Point(cx, cy), 5, cv::Scalar(0, 0, 255), -1);
                cv::putText(frameSeg, "(" + std::to_string(cx) + "," + std::to_string(cy) + ")",
                            cv::Point(cx + 8, cy - 8), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);
            }
        }

        cv::putText(frameSeg, labelColor, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.70, cv::Scalar(0, 255, 255), 2);

        cv::Mat maskBgr;
        cv::cvtColor(mask, maskBgr, cv::COLOR_GRAY2BGR);
        cv::putText(maskBgr, "Mascara Binaria", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.70, cv::Scalar(0, 255, 0), 2);

        int w = 400, h = 300;
        cv::resize(frameSeg, frameSeg, cv::Size(w, h));
        cv::resize(maskBgr, maskBgr, cv::Size(w, h));

        cv::Mat visor;
        cv::hconcat(frameSeg, maskBgr, visor);

        cv::imshow(winName, visor);

        int key = cv::waitKey(15);
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        } else if (key == 'r' || key == 'R') {
            colorObjetivo = 'R';
        } else if (key == 'g' || key == 'G') {
            colorObjetivo = 'G';
        } else if (key == 'b' || key == 'B') {
            colorObjetivo = 'B';
        }
    }

    cap.release();
    cv::destroyAllWindows();
}

// ============================================================================
// 6. MENÚ PRINCIPAL
// ============================================================================
int main() {
    int opcion = -1;
    std::string rutaImg = resolverRutaImagen("input.jpg");

    while (opcion != 0) {
        std::cout << "\n======================================================\n";
        std::cout << "  VISION ARTIFICIAL: TALLER DE SEGMENTACION           \n";
        std::cout << "======================================================\n";
        std::cout << "Imagen detectada: " << rutaImg << "\n\n";
        std::cout << "1. Segmentacion por Color en Espacio HSV (Trackbars y Presets)\n";
        std::cout << "2. Segmentacion por Algoritmo Watershed (Cuencas Hidrograficas)\n";
        std::cout << "3. Segmentacion por Cuantizacion K-Means (Clustered Color)\n";
        std::cout << "4. Segmentacion y Tracking de Color con Camara en Vivo\n";
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
                    ejecutarLaboratorioHSV(src);
                }
                break;
            case 2:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarSegmentacionWatershed(src);
                }
                break;
            case 3:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarSegmentacionKMeans(src);
                }
                break;
            case 4:
                procesarCamaraSegmentacion();
                break;
            case 0:
                std::cout << "\nSaliendo del taller de Segmentacion...\n";
                break;
            default:
                std::cout << "\n[!] Opcion no valida.\n";
                break;
        }
    }

    return 0;
}
