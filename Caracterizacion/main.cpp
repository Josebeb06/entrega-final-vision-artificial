#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <cmath>

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
// 2. CLASIFICADOR HEURÍSTICO DE FORMA
// ============================================================================
std::string estimarFormaGeometria(const std::vector<cv::Point>& contorno, double area, double perimetro) {
    if (perimetro <= 0) return "Desconocido";

    std::vector<cv::Point> approx;
    double epsilon = 0.03 * perimetro;
    cv::approxPolyDP(contorno, approx, epsilon, true);

    int vertices = static_cast<int>(approx.size());
    double circularidad = 4.0 * CV_PI * area / (perimetro * perimetro);

    if (circularidad > 0.82) {
        return "Circulo";
    }

    if (vertices == 3) {
        return "Triangulo";
    } else if (vertices == 4) {
        cv::Rect r = cv::boundingRect(approx);
        double relAspecto = static_cast<double>(r.width) / r.height;
        if (relAspecto >= 0.88 && relAspecto <= 1.12) {
            return "Cuadrado";
        } else {
            return "Rectangulo";
        }
    } else if (vertices == 5) {
        return "Pentagono";
    } else if (vertices == 6) {
        return "Hexagono";
    } else if (vertices > 6) {
        if (circularidad < 0.50) {
            return "Estrella/Compleja";
        } else {
            return "Poligono / Elipse";
        }
    }

    return "Poligono";
}

// ============================================================================
// 3. OPCIÓN 1: EXTRACCIÓN Y ANÁLISIS GEOMÉTRICO DE CONTORNOS
// ============================================================================
void mostrarAnalisisGeometrico(const cv::Mat& src) {
    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    cv::Mat bin;
    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // Si la imagen tiene fondo claro y objetos oscuros, Otsu invertido los hace blancos
    // Limpieza de ruido fino
    cv::Mat k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(bin, bin, cv::MORPH_OPEN, k);

    std::vector<std::vector<cv::Point>> contornos;
    std::vector<cv::Vec4i> jerarquia;
    cv::findContours(bin, contornos, jerarquia, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat lienzo = src.clone();

    std::cout << "\n========================================================================================\n";
    std::cout << "                        TABLA DE CARACTERIZACION GEOMETRICA                             \n";
    std::cout << "========================================================================================\n";
    std::cout << std::left << std::setw(4) << "ID"
              << std::setw(16) << "Forma"
              << std::setw(12) << "Area(px)"
              << std::setw(14) << "Perimetro(px)"
              << std::setw(16) << "Centroide(X,Y)"
              << std::setw(12) << "Compacidad"
              << "Orientacion" << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    int id = 1;
    for (size_t i = 0; i < contornos.size(); i++) {
        double area = cv::contourArea(contornos[i]);
        if (area < 400) continue; // Filtrar motas pequeñas o texto

        double perimetro = cv::arcLength(contornos[i], true);
        cv::Moments m = cv::moments(contornos[i]);
        int cx = 0, cy = 0;
        if (m.m00 > 0) {
            cx = static_cast<int>(m.m10 / m.m00);
            cy = static_cast<int>(m.m01 / m.m00);
        }

        std::string forma = estimarFormaGeometria(contornos[i], area, perimetro);
        double compacidad = (perimetro > 0) ? (4.0 * CV_PI * area / (perimetro * perimetro)) : 0.0;

        // Bounding Box Recto (Verde)
        cv::Rect bbox = cv::boundingRect(contornos[i]);
        cv::rectangle(lienzo, bbox, cv::Scalar(0, 255, 0), 2);

        // Bounding Box Rotado (Amarillo)
        cv::RotatedRect rRect = cv::minAreaRect(contornos[i]);
        cv::Point2f vertices[4];
        rRect.points(vertices);
        for (int j = 0; j < 4; j++) {
            cv::line(lienzo, vertices[j], vertices[(j + 1) % 4], cv::Scalar(0, 255, 255), 2);
        }

        // Círculo Mínimo Circunscrito (Cian)
        cv::Point2f center;
        float radius = 0;
        cv::minEnclosingCircle(contornos[i], center, radius);
        cv::circle(lienzo, center, cvRound(radius), cv::Scalar(255, 255, 0), 1);

        // Centroide (Rojo)
        cv::drawMarker(lienzo, cv::Point(cx, cy), cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 14, 2);

        // Rótulo sobre la imagen
        std::string etiqueta = "#" + std::to_string(id) + " " + forma;
        cv::putText(lienzo, etiqueta, cv::Point(bbox.x, std::max(20, bbox.y - 8)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(20, 20, 20), 3);
        cv::putText(lienzo, etiqueta, cv::Point(bbox.x, std::max(20, bbox.y - 8)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 255, 255), 1);

        // Imprimir en consola
        std::string strCent = "(" + std::to_string(cx) + ", " + std::to_string(cy) + ")";
        std::cout << std::left << std::setw(4) << id
                  << std::setw(16) << forma
                  << std::setw(12) << std::fixed << std::setprecision(1) << area
                  << std::setw(14) << perimetro
                  << std::setw(16) << strCent
                  << std::setw(12) << std::setprecision(2) << compacidad
                  << std::setprecision(1) << rRect.angle << " deg\n";
        id++;
    }
    std::cout << "========================================================================================\n";
    std::cout << "Leyenda visual: [Verde] Bounding Box | [Amarillo] Rotated Box | [Cian] Min Enclosing Circle\n";

    cv::namedWindow("Caracterizacion Geometrica de Formas", cv::WINDOW_AUTOSIZE);
    cv::imshow("Caracterizacion Geometrica de Formas", lienzo);
    std::cout << "\n[+] Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Caracterizacion Geometrica de Formas");
}

// ============================================================================
// 4. OPCIÓN 2: DESCRIPTORES INVARIANTES - MOMENTOS DE HU
// ============================================================================
void mostrarMomentosHu(const cv::Mat& src) {
    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    cv::Mat bin;
    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    std::vector<std::vector<cv::Point>> contornos;
    cv::findContours(bin, contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat lienzo = src.clone();

    std::cout << "\n============================================================================\n";
    std::cout << "               MOMENTOS DE HU (LOGARITMICOS INVARIANTES)                    \n";
    std::cout << "============================================================================\n";
    std::cout << "Invarianza probada ante traslacion, escala y rotacion.\n\n";

    int id = 1;
    for (size_t i = 0; i < contornos.size(); i++) {
        double area = cv::contourArea(contornos[i]);
        if (area < 500) continue;

        cv::Moments m = cv::moments(contornos[i]);
        double hu[7];
        cv::HuMoments(m, hu);

        // Escala logarítmica para visualización humana: -1 * copysign(log10(abs(hu[i])))
        double huLog[7];
        for (int j = 0; j < 7; j++) {
            huLog[j] = -1.0 * std::copysign(1.0, hu[j]) * std::log10(std::abs(hu[j]) + 1e-30);
        }

        std::string forma = estimarFormaGeometria(contornos[i], area, cv::arcLength(contornos[i], true));
        std::cout << "Figura #" << id << " (" << forma << ", Area: " << area << " px):\n";
        std::cout << "  Hu[0..6] = [ ";
        for (int j = 0; j < 7; j++) {
            std::cout << std::fixed << std::setprecision(2) << huLog[j] << (j < 6 ? ", " : " ]\n\n");
        }

        cv::Rect bbox = cv::boundingRect(contornos[i]);
        cv::rectangle(lienzo, bbox, cv::Scalar(0, 200, 255), 2);
        cv::putText(lienzo, "#" + std::to_string(id) + " Hu[0]=" + std::to_string(huLog[0]).substr(0, 4),
                    cv::Point(bbox.x, std::max(20, bbox.y - 8)), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(255, 0, 0), 2);
        id++;
    }

    cv::namedWindow("Momentos de Hu Invariantes", cv::WINDOW_AUTOSIZE);
    cv::imshow("Momentos de Hu Invariantes", lienzo);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Momentos de Hu Invariantes");
}

// ============================================================================
// 5. OPCIÓN 3: ENVOLVENTE CONVEXA (CONVEX HULL) Y DEFECTOS DE CONVEXIDAD
// ============================================================================
void mostrarConvexHull(const cv::Mat& src) {
    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    cv::Mat bin;
    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    std::vector<std::vector<cv::Point>> contornos;
    cv::findContours(bin, contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat lienzo = src.clone();

    for (size_t i = 0; i < contornos.size(); i++) {
        if (cv::contourArea(contornos[i]) < 500) continue;

        // Calcular Envolvente Convexa
        std::vector<cv::Point> hullPoints;
        std::vector<int> hullIndices;
        cv::convexHull(contornos[i], hullPoints, false);
        cv::convexHull(contornos[i], hullIndices, false);

        // Dibujar contorno original en verde
        cv::polylines(lienzo, contornos[i], true, cv::Scalar(0, 255, 0), 2);

        // Dibujar Convex Hull en azul
        cv::polylines(lienzo, hullPoints, true, cv::Scalar(255, 0, 0), 2);

        // Calcular Defectos de Convexidad si hay suficientes puntos
        if (hullIndices.size() > 3 && contornos[i].size() > 3) {
            std::vector<cv::Vec4i> defects;
            try {
                cv::convexityDefects(contornos[i], hullIndices, defects);
                for (const auto& d : defects) {
                    float depth = d[3] / 256.0f; // La profundidad viene multiplicada por 256
                    if (depth > 10.0f) { // Filtrar defectos pequeños de ruido
                        int farIdx = d[2];
                        cv::Point farPt = contornos[i][farIdx];
                        // Marcar defecto profundo con círculo rojo
                        cv::circle(lienzo, farPt, 6, cv::Scalar(0, 0, 255), -1);
                    }
                }
            } catch (...) {}
        }
    }

    cv::rectangle(lienzo, cv::Rect(0, 0, lienzo.cols, 40), cv::Scalar(25, 25, 25), -1);
    cv::putText(lienzo, "[Verde] Contorno Original | [Azul] Convex Hull | [Rojo] Defectos Concavos",
                cv::Point(15, 26), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 255), 2);

    cv::namedWindow("Convex Hull y Defectos de Convexidad", cv::WINDOW_AUTOSIZE);
    cv::imshow("Convex Hull y Defectos de Convexidad", lienzo);
    std::cout << "\n[+] Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Convex Hull y Defectos de Convexidad");
}

// ============================================================================
// 6. OPCIÓN 4: DETECCIÓN DE PUNTOS CARACTERÍSTICOS (HARRIS Y SHI-TOMASI)
// ============================================================================
void mostrarPuntosCaracteristicos(const cv::Mat& src) {
    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    // 1. Esquinas de Harris
    cv::Mat harrisDst, harrisNorm;
    cv::cornerHarris(gray, harrisDst, 2, 3, 0.04);
    cv::normalize(harrisDst, harrisNorm, 0, 255, cv::NORM_MINMAX, CV_32FC1, cv::Mat());

    cv::Mat harrisView = src.clone();
    for (int j = 0; j < harrisNorm.rows; j++) {
        for (int i = 0; i < harrisNorm.cols; i++) {
            if (static_cast<int>(harrisNorm.at<float>(j, i)) > 150) {
                cv::circle(harrisView, cv::Point(i, j), 4, cv::Scalar(0, 0, 255), -1);
            }
        }
    }

    // 2. Esquinas Shi-Tomasi (Good Features to Track)
    std::vector<cv::Point2f> corners;
    cv::goodFeaturesToTrack(gray, corners, 100, 0.01, 10, cv::Mat(), 3, false, 0.04);

    cv::Mat shiTomasiView = src.clone();
    for (const auto& pt : corners) {
        cv::circle(shiTomasiView, pt, 5, cv::Scalar(0, 255, 0), -1);
        cv::circle(shiTomasiView, pt, 7, cv::Scalar(255, 0, 0), 1);
    }

    int w = 420, h = 320;
    cv::resize(harrisView, harrisView, cv::Size(w, h));
    cv::resize(shiTomasiView, shiTomasiView, cv::Size(w, h));

    cv::putText(harrisView, "Esquinas de Harris (Rojo)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 255), 2);
    cv::putText(shiTomasiView, "Shi-Tomasi Corners (Verde)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 255), 2);

    cv::Mat comparativa;
    cv::hconcat(harrisView, shiTomasiView, comparativa);

    cv::imshow("Deteccion de Puntos Caracteristicos / Esquinas", comparativa);
    std::cout << "\n[+] Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Deteccion de Puntos Caracteristicos / Esquinas");
}

// ============================================================================
// 7. OPCIÓN 5: CARACTERIZACIÓN EN TIEMPO REAL CON CÁMARA
// ============================================================================
void procesarCamaraCaracterizacion() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "\n[ERROR] No se pudo abrir la camara web.\n";
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    std::cout << "\n[+] Camara iniciada.\n";
    std::cout << "Caracterizando objetos en tiempo real.\n";
    std::cout << "Presiona 'ESC' o 'q' para volver al menu.\n";

    const std::string winName = "Camara: Caracterizacion y Tracking de Figuras";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat gray, bin;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);
        cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

        std::vector<std::vector<cv::Point>> contornos;
        cv::findContours(bin, contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        cv::Mat display = frame.clone();

        for (const auto& c : contornos) {
            double area = cv::contourArea(c);
            if (area < 1200) continue; // Filtrar ruido

            double perimetro = cv::arcLength(c, true);
            cv::Moments m = cv::moments(c);
            if (m.m00 <= 0) continue;

            int cx = static_cast<int>(m.m10 / m.m00);
            int cy = static_cast<int>(m.m01 / m.m00);

            std::string forma = estimarFormaGeometria(c, area, perimetro);

            cv::Rect bbox = cv::boundingRect(c);
            cv::rectangle(display, bbox, cv::Scalar(0, 255, 0), 2);
            cv::drawMarker(display, cv::Point(cx, cy), cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 12, 2);

            std::string info = forma + " (A: " + std::to_string((int)area) + "px)";
            cv::putText(display, info, cv::Point(bbox.x, std::max(25, bbox.y - 8)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 255), 2);
        }

        cv::rectangle(display, cv::Rect(0, 0, display.cols, 35), cv::Scalar(20, 20, 20), -1);
        cv::putText(display, "Caracterizacion en Vivo: Forma, Area y Centroide",
                    cv::Point(15, 23), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 255, 255), 1);

        cv::imshow(winName, display);

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
        std::cout << "\n======================================================\n";
        std::cout << "  VISION ARTIFICIAL: TALLER DE CARACTERIZACION        \n";
        std::cout << "======================================================\n";
        std::cout << "Imagen detectada: " << rutaImg << "\n\n";
        std::cout << "1. Extraccion y Analisis Geometrico de Contornos (Tabla y Bounding Boxes)\n";
        std::cout << "2. Descriptores Invariantes: Momentos de Hu (Logaritmicos)\n";
        std::cout << "3. Envolvente Convexa (Convex Hull) y Defectos de Convexidad\n";
        std::cout << "4. Deteccion de Puntos Caracteristicos (Harris y Shi-Tomasi)\n";
        std::cout << "5. Caracterizacion de Formas en Tiempo Real con Camara\n";
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
                    mostrarAnalisisGeometrico(src);
                }
                break;
            case 2:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarMomentosHu(src);
                }
                break;
            case 3:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarConvexHull(src);
                }
                break;
            case 4:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarPuntosCaracteristicos(src);
                }
                break;
            case 5:
                procesarCamaraCaracterizacion();
                break;
            case 0:
                std::cout << "\nSaliendo del taller de Caracterizacion...\n";
                break;
            default:
                std::cout << "\n[!] Opcion no valida.\n";
                break;
        }
    }

    return 0;
}
