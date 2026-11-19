#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cmath>

namespace fs = std::filesystem;

// ============================================================================
// 1. SISTEMA PORTABLE DE RESOLUCIÓN DE RECURSOS (ASSETS)
// ============================================================================
std::string resolverAsset(const std::string& nombre) {
    std::vector<std::string> candidatos = {
        "assets/" + nombre,
        "../assets/" + nombre,
        "../../assets/" + nombre,
        "../../../assets/" + nombre,
        nombre,
        "../" + nombre,
        "../../" + nombre
    };

    for (const auto& ruta : candidatos) {
        if (fs::exists(ruta)) {
            return ruta;
        }
    }

    // Búsqueda en carpetas temáticas correspondientes
    if (nombre == "filtros.jpg" || nombre == "input.jpg") {
        std::vector<std::string> sub = { "Filtros/input.jpg", "../Filtros/input.jpg", "../../Filtros/input.jpg" };
        for (const auto& r : sub) if (fs::exists(r)) return r;
    } else if (nombre == "histogramas.jpg") {
        std::vector<std::string> sub = { "Histogramas/input.jpg", "../Histogramas/input.jpg", "../../Histogramas/input.jpg" };
        for (const auto& r : sub) if (fs::exists(r)) return r;
    } else if (nombre == "binarizacion.jpg") {
        std::vector<std::string> sub = { "Binarización/input.jpg", "../Binarización/input.jpg", "../../Binarización/input.jpg" };
        for (const auto& r : sub) if (fs::exists(r)) return r;
    } else if (nombre == "morfologia.jpg") {
        std::vector<std::string> sub = { "MorfologiaMatematica/input.jpg", "../MorfologiaMatematica/input.jpg", "../../MorfologiaMatematica/input.jpg" };
        for (const auto& r : sub) if (fs::exists(r)) return r;
    } else if (nombre == "segmentacion.jpg") {
        std::vector<std::string> sub = { "Segmentacion/input.jpg", "../Segmentacion/input.jpg", "../../Segmentacion/input.jpg" };
        for (const auto& r : sub) if (fs::exists(r)) return r;
    } else if (nombre == "caracterizacion.jpg") {
        std::vector<std::string> sub = { "Caracterizacion/input.jpg", "../Caracterizacion/input.jpg", "../../Caracterizacion/input.jpg" };
        for (const auto& r : sub) if (fs::exists(r)) return r;
    } else if (nombre == "lenna.jpg") {
        std::vector<std::string> sub = { "Frecuencia/lenna.jpg", "../Frecuencia/lenna.jpg", "../../Frecuencia/lenna.jpg" };
        for (const auto& r : sub) if (fs::exists(r)) return r;
    } else if (nombre == "tablero_calibracion.png") {
        std::vector<std::string> sub = { "TallerCalibracion/tablero_calibracion.png", "../TallerCalibracion/tablero_calibracion.png", "../../TallerCalibracion/tablero_calibracion.png" };
        for (const auto& r : sub) if (fs::exists(r)) return r;
    }

    return nombre;
}

// ============================================================================
// 2. PALETA DE COLORES Y ESTILOS MODERNOS (DARK THEME)
// ============================================================================
namespace Theme {
    const cv::Scalar BG_DARK        = cv::Scalar(24, 20, 18);
    const cv::Scalar HEADER_BG      = cv::Scalar(34, 28, 24);
    const cv::Scalar CARD_BG        = cv::Scalar(40, 33, 28);
    const cv::Scalar CARD_BORDER    = cv::Scalar(65, 54, 46);
    const cv::Scalar CARD_HOVER_BG  = cv::Scalar(58, 48, 40);
    const cv::Scalar ACCENT_GOLD    = cv::Scalar(0, 205, 255);
    const cv::Scalar ACCENT_CYAN    = cv::Scalar(255, 215, 0);
    const cv::Scalar ACCENT_BLUE    = cv::Scalar(255, 120, 30);
    const cv::Scalar ACCENT_GREEN   = cv::Scalar(60, 220, 80);
    const cv::Scalar TEXT_WHITE     = cv::Scalar(245, 245, 245);
    const cv::Scalar TEXT_GRAY      = cv::Scalar(170, 170, 170);
    const cv::Scalar TEXT_MUTED     = cv::Scalar(115, 115, 115);
    const cv::Scalar TAB_ACTIVE     = cv::Scalar(0, 140, 255);
    const cv::Scalar TAB_INACTIVE   = cv::Scalar(48, 40, 34);
    const cv::Scalar TAB_HOVER      = cv::Scalar(75, 62, 54);
}

// Estructuras de la UI
struct CardInfo {
    int id;
    std::string tag;
    std::string titulo;
    std::string descripcion;
    cv::Scalar colorAcento;
    cv::Rect bounds;
    bool hover = false;
};

struct TabInfo {
    int id;
    std::string label;
    cv::Rect bounds;
    bool hover = false;
};

enum AppState {
    STATE_MAIN_MENU,
    STATE_MODULE_VIEW
};

// Estado global de la aplicación interactiva
struct AppContext {
    AppState estado = STATE_MAIN_MENU;
    int moduloActual = 0;
    int subAccionActual = 0;

    int mouseX = 0;
    int mouseY = 0;
    bool necesitaRedibujar = true;
    bool salirApp = false;

    cv::Rect btnVolver = cv::Rect(20, 14, 180, 42);
    bool hoverVolver = false;

    std::vector<CardInfo> tarjetas;
    std::vector<TabInfo> tabs;

    cv::Mat canvas;
    const std::string windowName = "Vision Artificial - Suite de Mini Proyectos";
};

AppContext g_app;

// ============================================================================
// 3. AUXILIARES DE DIBUJO DE INTERFAZ
// ============================================================================
void dibujarBoton(cv::Mat& img, const cv::Rect& r, const std::string& texto, 
                  bool hover, bool active, cv::Scalar colorBase, cv::Scalar colorHover) {
    cv::Scalar colorFondo = active ? colorHover : (hover ? colorHover : colorBase);
    cv::rectangle(img, r, colorFondo, -1);
    cv::rectangle(img, r, hover ? Theme::ACCENT_GOLD : Theme::CARD_BORDER, 2);

    int baseline = 0;
    cv::Size textSize = cv::getTextSize(texto, cv::FONT_HERSHEY_SIMPLEX, 0.52, 1, &baseline);
    int textX = r.x + (r.width - textSize.width) / 2;
    int textY = r.y + (r.height + textSize.height) / 2;
    cv::putText(img, texto, cv::Point(textX, textY), cv::FONT_HERSHEY_SIMPLEX, 0.52, Theme::TEXT_WHITE, 1, cv::LINE_AA);
}

void dibujarTarjetaMenu(cv::Mat& img, CardInfo& card) {
    cv::Scalar fondo = card.hover ? Theme::CARD_HOVER_BG : Theme::CARD_BG;
    cv::Scalar borde = card.hover ? card.colorAcento : Theme::CARD_BORDER;

    cv::rectangle(img, card.bounds, fondo, -1);
    cv::rectangle(img, card.bounds, borde, card.hover ? 3 : 1);

    // Barra de acento lateral
    cv::Rect barraAcento(card.bounds.x, card.bounds.y, 6, card.bounds.height);
    cv::rectangle(img, barraAcento, card.colorAcento, -1);

    // Tag superior
    cv::putText(img, card.tag, cv::Point(card.bounds.x + 18, card.bounds.y + 30),
                cv::FONT_HERSHEY_SIMPLEX, 0.45, card.colorAcento, 1, cv::LINE_AA);

    // Título
    cv::putText(img, card.titulo, cv::Point(card.bounds.x + 18, card.bounds.y + 60),
                cv::FONT_HERSHEY_DUPLEX, 0.65, Theme::TEXT_WHITE, 1, cv::LINE_AA);

    // Separador sutil
    cv::line(img, cv::Point(card.bounds.x + 18, card.bounds.y + 75),
                  cv::Point(card.bounds.x + card.bounds.width - 18, card.bounds.y + 75),
                  Theme::CARD_BORDER, 1);

    // Descripción multilínea
    std::string linea1 = card.descripcion;
    std::string linea2 = "";
    size_t posSep = card.descripcion.find('|');
    if (posSep != std::string::npos) {
        linea1 = card.descripcion.substr(0, posSep);
        linea2 = card.descripcion.substr(posSep + 1);
    }

    cv::putText(img, linea1, cv::Point(card.bounds.x + 18, card.bounds.y + 105),
                cv::FONT_HERSHEY_SIMPLEX, 0.45, Theme::TEXT_GRAY, 1, cv::LINE_AA);
    if (!linea2.empty()) {
        cv::putText(img, linea2, cv::Point(card.bounds.x + 18, card.bounds.y + 128),
                    cv::FONT_HERSHEY_SIMPLEX, 0.45, Theme::TEXT_GRAY, 1, cv::LINE_AA);
    }

    // Botón inferior "Abrir Módulo ->"
    cv::Rect btnAbrir(card.bounds.x + 18, card.bounds.y + card.bounds.height - 48, card.bounds.width - 36, 32);
    cv::Scalar btnBg = card.hover ? card.colorAcento : cv::Scalar(50, 42, 36);
    cv::Scalar btnText = card.hover ? cv::Scalar(20, 20, 20) : Theme::TEXT_WHITE;
    cv::rectangle(img, btnAbrir, btnBg, -1);
    cv::putText(img, "Explorar Modulo ->", cv::Point(btnAbrir.x + 22, btnAbrir.y + 21),
                cv::FONT_HERSHEY_SIMPLEX, 0.48, btnText, card.hover ? 2 : 1, cv::LINE_AA);
}

// ============================================================================
// 4. MÓDULOS DE PROCESAMIENTO DE IMÁGENES
// ============================================================================

// --- 4.1. MODELOS DE CÁMARA Y CALIBRACIÓN ---
cv::Mat procesarVistaCalibracion(int subAccion) {
    std::string ruta = resolverAsset("tablero_calibracion.png");
    cv::Mat base = cv::imread(ruta, cv::IMREAD_COLOR);
    if (base.empty()) {
        base = cv::Mat(480, 640, CV_8UC3, cv::Scalar(30, 30, 30));
        cv::putText(base, "No se encontro patron de ajedrez", cv::Point(50, 240), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
        return base;
    }

    int w = 550, h = 420;

    if (subAccion == 0) { // Calibración Pinhole e Intrínsicos
        cv::Size patternSize(9, 6);
        std::vector<cv::Point3f> objP;
        for (int i = 0; i < patternSize.height; i++) {
            for (int j = 0; j < patternSize.width; j++) {
                objP.push_back(cv::Point3f(j * 25.0f, i * 25.0f, 0.0f));
            }
        }

        std::vector<std::vector<cv::Point3f>> objectPoints;
        std::vector<std::vector<cv::Point2f>> imagePoints;

        cv::Mat gray;
        cv::cvtColor(base, gray, cv::COLOR_BGR2GRAY);
        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCornersSB(gray, patternSize, corners, cv::CALIB_CB_EXHAUSTIVE);

        cv::Mat viewCorners = base.clone();
        if (found) {
            cv::drawChessboardCorners(viewCorners, patternSize, corners, true);
            imagePoints.push_back(corners);
            objectPoints.push_back(objP);
        }

        cv::Mat camMat = cv::Mat::eye(3, 3, CV_64F);
        cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
        std::vector<cv::Mat> rvecs, tvecs;
        double rms = 0.0;
        if (!imagePoints.empty()) {
            rms = cv::calibrateCamera(objectPoints, imagePoints, base.size(), camMat, distCoeffs, rvecs, tvecs);
        }

        cv::Mat undistorted;
        cv::undistort(base, undistorted, camMat, distCoeffs);

        cv::Mat r1, r2;
        cv::resize(viewCorners, r1, cv::Size(w, h));
        cv::resize(undistorted, r2, cv::Size(w, h));

        cv::putText(r1, "1. Deteccion Subpixel (54 esquinas)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(r2, "2. Imagen Corregida (Undistort - RMS: " + std::to_string(rms).substr(0, 4) + "px)",
                    cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 220, 255), 2);

        cv::Mat out;
        cv::hconcat(r1, r2, out);
        return out;
    } else { // Rectificación Proyectiva y Homografía 3x3
        cv::Size patternSize(9, 6);
        cv::Mat gray;
        cv::cvtColor(base, gray, cv::COLOR_BGR2GRAY);
        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCornersSB(gray, patternSize, corners, cv::CALIB_CB_EXHAUSTIVE);

        cv::Mat warped;
        if (found) {
            std::vector<cv::Point2f> dstCorners = {
                cv::Point2f(60, 60), cv::Point2f(500, 60),
                cv::Point2f(500, 360), cv::Point2f(60, 360)
            };
            std::vector<cv::Point2f> srcCorners = {
                corners.front(), corners[patternSize.width - 1],
                corners.back(), corners[corners.size() - patternSize.width]
            };
            cv::Mat H = cv::findHomography(srcCorners, dstCorners);
            cv::warpPerspective(base, warped, H, cv::Size(w, h), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(240, 240, 240));
        } else {
            warped = cv::Mat(h, w, CV_8UC3, cv::Scalar(30, 30, 30));
        }

        cv::Mat rOrig;
        cv::resize(base, rOrig, cv::Size(w, h));
        cv::putText(rOrig, "Tablero Original Inclinado", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(warped, "Encuadre Frontal Rectificado (Homografia)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 200, 255), 2);

        cv::Mat out;
        cv::hconcat(rOrig, warped, out);
        return out;
    }
}

// --- 4.2. FILTROS ESPACIALES ---
cv::Mat procesarVistaFiltros(int subAccion) {
    std::string ruta = resolverAsset("filtros.jpg");
    cv::Mat src = cv::imread(ruta, cv::IMREAD_COLOR);
    if (src.empty()) src = cv::Mat(500, 700, CV_8UC3, cv::Scalar(40, 40, 40));

    int w = 550, h = 260;

    if (subAccion == 0) { // Suavizado
        cv::Mat bBox, bGauss, bMed, bBilateral;
        cv::blur(src, bBox, cv::Size(7, 7));
        cv::GaussianBlur(src, bGauss, cv::Size(7, 7), 2.0);
        cv::medianBlur(src, bMed, 7);
        cv::bilateralFilter(src, bBilateral, 9, 75, 75);

        cv::resize(bBox, bBox, cv::Size(w, h));
        cv::resize(bGauss, bGauss, cv::Size(w, h));
        cv::resize(bMed, bMed, cv::Size(w, h));
        cv::resize(bBilateral, bBilateral, cv::Size(w, h));

        cv::putText(bBox, "1. Media (Box Blur 7x7)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);
        cv::putText(bGauss, "2. Desenfoque Gaussiano (s=2.0)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(bMed, "3. Filtro Mediana (Quita Sal y Pimienta)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 100, 0), 2);
        cv::putText(bBilateral, "4. Bilateral (Conserva Bordes Nitidos)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 0, 255), 2);

        cv::Mat f1, f2, out;
        cv::hconcat(bBox, bGauss, f1);
        cv::hconcat(bMed, bBilateral, f2);
        cv::vconcat(f1, f2, out);
        return out;
    } else if (subAccion == 1) { // Detección de Bordes
        cv::Mat gray;
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        cv::Mat gx, gy, agx, agy, sobelTot, lap, canny;
        cv::Sobel(gray, gx, CV_16S, 1, 0, 3);
        cv::Sobel(gray, gy, CV_16S, 0, 1, 3);
        cv::convertScaleAbs(gx, agx);
        cv::convertScaleAbs(gy, agy);
        cv::addWeighted(agx, 0.5, agy, 0.5, 0, sobelTot);

        cv::Laplacian(gray, lap, CV_16S, 3);
        cv::convertScaleAbs(lap, lap);

        cv::Canny(gray, canny, 50, 150);

        cv::cvtColor(gray, gray, cv::COLOR_GRAY2BGR);
        cv::cvtColor(sobelTot, sobelTot, cv::COLOR_GRAY2BGR);
        cv::cvtColor(lap, lap, cv::COLOR_GRAY2BGR);
        cv::cvtColor(canny, canny, cv::COLOR_GRAY2BGR);

        cv::resize(gray, gray, cv::Size(w, h));
        cv::resize(sobelTot, sobelTot, cv::Size(w, h));
        cv::resize(lap, lap, cv::Size(w, h));
        cv::resize(canny, canny, cv::Size(w, h));

        cv::putText(gray, "1. Grises Original", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(sobelTot, "2. Magnitud Sobel (Gx + Gy)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);
        cv::putText(lap, "3. Laplaciano (2da Derivada)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 100, 0), 2);
        cv::putText(canny, "4. Canny Edge Detector (Histeresis)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 0, 255), 2);

        cv::Mat f1, f2, out;
        cv::hconcat(gray, sobelTot, f1);
        cv::hconcat(lap, canny, f2);
        cv::vconcat(f1, f2, out);
        return out;
    } else { // Realce / Sharpening
        cv::Mat kSharp = (cv::Mat_<float>(3, 3) << 0, -1, 0, -1, 5, -1, 0, -1, 0);
        cv::Mat enfocado, unsharp, suave;
        cv::filter2D(src, enfocado, src.depth(), kSharp);

        cv::GaussianBlur(src, suave, cv::Size(0, 0), 3.0);
        cv::addWeighted(src, 1.6, suave, -0.6, 0, unsharp);

        cv::Mat rOrig, rSharp, rUnsharp;
        cv::resize(src, rOrig, cv::Size(360, 480));
        cv::resize(enfocado, rSharp, cv::Size(360, 480));
        cv::resize(unsharp, rUnsharp, cv::Size(360, 480));

        cv::putText(rOrig, "Original", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(rSharp, "Sharpening Laplaciano", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 200, 255), 2);
        cv::putText(rUnsharp, "Unsharp Masking (Gauss)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(255, 100, 0), 2);

        cv::Mat out;
        cv::hconcat(std::vector<cv::Mat>{rOrig, rSharp, rUnsharp}, out);
        return out;
    }
}

// --- 4.3. HISTOGRAMAS ---
cv::Mat renderizarHistGrisesMat(const cv::Mat& gray, int hist_w = 540, int hist_h = 360, const std::string& titulo = "Histograma") {
    int histSize = 256;
    float range[] = { 0, 256 };
    const float* histRange = { range };
    cv::Mat hist;
    cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
    cv::normalize(hist, hist, 0, hist_h - 40, cv::NORM_MINMAX);

    cv::Mat histImg(hist_h, hist_w, CV_8UC3, cv::Scalar(25, 25, 25));
    int bin_w = cvRound((double)hist_w / histSize);

    for (int i = 0; i < histSize; i++) {
        int val = cvRound(hist.at<float>(i));
        cv::rectangle(histImg, cv::Point(i * bin_w, hist_h - 20),
                               cv::Point((i + 1) * bin_w, hist_h - 20 - val),
                               cv::Scalar(190, 190, 190), -1);
    }
    cv::line(histImg, cv::Point(0, hist_h - 20), cv::Point(hist_w, hist_h - 20), cv::Scalar(100, 100, 100), 2);
    cv::putText(histImg, titulo, cv::Point(15, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 200), 2);
    return histImg;
}

cv::Mat procesarVistaHistogramas(int subAccion) {
    std::string ruta = resolverAsset("histogramas.jpg");
    cv::Mat src = cv::imread(ruta, cv::IMREAD_COLOR);
    if (src.empty()) src = cv::Mat(500, 700, CV_8UC3, cv::Scalar(40, 40, 40));

    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    if (subAccion == 0) { // Grises y Estadísticas
        cv::Scalar media, stddev;
        cv::meanStdDev(gray, media, stddev);
        double minV, maxV;
        cv::minMaxLoc(gray, &minV, &maxV);

        cv::Mat rGray;
        cv::cvtColor(gray, rGray, cv::COLOR_GRAY2BGR);
        cv::resize(rGray, rGray, cv::Size(540, 480));
        cv::rectangle(rGray, cv::Rect(0, 0, rGray.cols, 55), cv::Scalar(20, 20, 20), -1);
        cv::putText(rGray, "Escala de Grises", cv::Point(15, 25), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        std::string info = "Media: " + std::to_string((int)media[0]) + " | Std: " + std::to_string((int)stddev[0]) +
                           " | Min: " + std::to_string((int)minV) + " | Max: " + std::to_string((int)maxV);
        cv::putText(rGray, info, cv::Point(15, 47), cv::FONT_HERSHEY_SIMPLEX, 0.50, cv::Scalar(200, 200, 200), 1);

        cv::Mat hMat = renderizarHistGrisesMat(gray, 540, 480, "Distribucion de Intensidades (0-255)");
        cv::Mat out;
        cv::hconcat(rGray, hMat, out);
        return out;
    } else if (subAccion == 1) { // RGB por Canales
        std::vector<cv::Mat> planes;
        cv::split(src, planes);
        int histSize = 256;
        float range[] = { 0, 256 };
        const float* histRange = { range };
        cv::Mat bH, gH, rH;
        cv::calcHist(&planes[0], 1, 0, cv::Mat(), bH, 1, &histSize, &histRange);
        cv::calcHist(&planes[1], 1, 0, cv::Mat(), gH, 1, &histSize, &histRange);
        cv::calcHist(&planes[2], 1, 0, cv::Mat(), rH, 1, &histSize, &histRange);

        int hw = 540, hh = 480;
        cv::normalize(bH, bH, 0, hh - 40, cv::NORM_MINMAX);
        cv::normalize(gH, gH, 0, hh - 40, cv::NORM_MINMAX);
        cv::normalize(rH, rH, 0, hh - 40, cv::NORM_MINMAX);

        cv::Mat hColor(hh, hw, CV_8UC3, cv::Scalar(20, 20, 20));
        int bin_w = cvRound((double)hw / histSize);
        for (int i = 1; i < histSize; i++) {
            cv::line(hColor, cv::Point(bin_w * (i - 1), hh - 20 - cvRound(bH.at<float>(i - 1))),
                             cv::Point(bin_w * i, hh - 20 - cvRound(bH.at<float>(i))), cv::Scalar(255, 60, 20), 2);
            cv::line(hColor, cv::Point(bin_w * (i - 1), hh - 20 - cvRound(gH.at<float>(i - 1))),
                             cv::Point(bin_w * i, hh - 20 - cvRound(gH.at<float>(i))), cv::Scalar(20, 220, 30), 2);
            cv::line(hColor, cv::Point(bin_w * (i - 1), hh - 20 - cvRound(rH.at<float>(i - 1))),
                             cv::Point(bin_w * i, hh - 20 - cvRound(rH.at<float>(i))), cv::Scalar(30, 40, 255), 2);
        }
        cv::putText(hColor, "Histograma BGR Superpuesto", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 255, 255), 2);
        cv::putText(hColor, "[Azul] Canal B  |  [Verde] Canal G  |  [Rojo] Canal R", cv::Point(15, 60), cv::FONT_HERSHEY_SIMPLEX, 0.48, cv::Scalar(200, 200, 200), 1);

        cv::Mat rSrc;
        cv::resize(src, rSrc, cv::Size(hw, hh));
        cv::putText(rSrc, "Imagen Color BGR", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);

        cv::Mat out;
        cv::hconcat(rSrc, hColor, out);
        return out;
    } else { // Ecualización Global vs CLAHE
        cv::Mat eqGlobal, eqCLAHE;
        cv::equalizeHist(gray, eqGlobal);
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));
        clahe->apply(gray, eqCLAHE);

        int w = 360, h = 480;
        cv::Mat rOrig, rGlob, rClahe;
        cv::cvtColor(gray, rOrig, cv::COLOR_GRAY2BGR);
        cv::cvtColor(eqGlobal, rGlob, cv::COLOR_GRAY2BGR);
        cv::cvtColor(eqCLAHE, rClahe, cv::COLOR_GRAY2BGR);

        cv::resize(rOrig, rOrig, cv::Size(w, h));
        cv::resize(rGlob, rGlob, cv::Size(w, h));
        cv::resize(rClahe, rClahe, cv::Size(w, h));

        cv::putText(rOrig, "1. Original Oscura", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(rGlob, "2. Ecualizacion Global", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);
        cv::putText(rClahe, "3. CLAHE Adaptativo (8x8)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 100, 0), 2);

        cv::Mat out;
        cv::hconcat(std::vector<cv::Mat>{rOrig, rGlob, rClahe}, out);
        return out;
    }
}

// --- 4.4. UMBRALIZACIÓN Y BINARIZACIÓN ---
cv::Mat procesarVistaBinarizacion(int subAccion) {
    std::string ruta = resolverAsset("binarizacion.jpg");
    cv::Mat src = cv::imread(ruta, cv::IMREAD_COLOR);
    if (src.empty()) src = cv::Mat(500, 700, CV_8UC3, cv::Scalar(40, 40, 40));

    cv::Mat gray;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

    if (subAccion == 0) { // Cuadrícula 2x2: Global, Otsu, Adaptativa Media, Adaptativa Gauss
        cv::Mat dGlobal, dOtsu, dMedia, dGauss;
        cv::threshold(gray, dGlobal, 127, 255, cv::THRESH_BINARY);
        double otsuT = cv::threshold(gray, dOtsu, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::adaptiveThreshold(gray, dMedia, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 11, 2);
        cv::adaptiveThreshold(gray, dGauss, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);

        int w = 540, h = 250;
        cv::Mat cG, cO, cM, cGa;
        cv::cvtColor(dGlobal, cG, cv::COLOR_GRAY2BGR);
        cv::cvtColor(dOtsu, cO, cv::COLOR_GRAY2BGR);
        cv::cvtColor(dMedia, cM, cv::COLOR_GRAY2BGR);
        cv::cvtColor(dGauss, cGa, cv::COLOR_GRAY2BGR);

        cv::resize(cG, cG, cv::Size(w, h));
        cv::resize(cO, cO, cv::Size(w, h));
        cv::resize(cM, cM, cv::Size(w, h));
        cv::resize(cGa, cGa, cv::Size(w, h));

        cv::putText(cG, "1. Global Fija (T = 127)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 0, 255), 2);
        cv::putText(cO, "2. Metodo Otsu (Optimo T = " + std::to_string((int)otsuT) + ")", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 100, 0), 2);
        cv::putText(cM, "3. Adaptativa Media (11x11, C=2)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 220, 255), 2);
        cv::putText(cGa, "4. Adaptativa Gaussiana (11x11, C=2)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);

        cv::Mat f1, f2, out;
        cv::hconcat(cG, cO, f1);
        cv::hconcat(cM, cGa, f2);
        cv::vconcat(f1, f2, out);
        return out;
    } else { // Histograma con Umbral Óptimo de Otsu
        cv::Mat dOtsu;
        double otsuT = cv::threshold(gray, dOtsu, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        int hw = 540, hh = 480;
        cv::Mat hist = renderizarHistGrisesMat(gray, hw, hh, "Histograma con Umbral Otsu");
        int bin_w = cvRound((double)hw / 256.0);
        int xOtsu = cvRound(otsuT * bin_w);
        cv::line(hist, cv::Point(xOtsu, 0), cv::Point(xOtsu, hh - 20), cv::Scalar(0, 0, 255), 2);
        cv::putText(hist, "Corte Otsu T=" + std::to_string((int)otsuT), cv::Point(xOtsu + 10, 50),
                    cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 100, 255), 2);

        cv::Mat cOtsu;
        cv::cvtColor(dOtsu, cOtsu, cv::COLOR_GRAY2BGR);
        cv::resize(cOtsu, cOtsu, cv::Size(hw, hh));
        cv::putText(cOtsu, "Imagen Binarizada Otsu", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);

        cv::Mat out;
        cv::hconcat(hist, cOtsu, out);
        return out;
    }
}

// --- 4.5. MORFOLOGÍA MATEMÁTICA ---
cv::Mat procesarVistaMorfologia(int subAccion) {
    std::string ruta = resolverAsset("morfologia.jpg");
    cv::Mat src = cv::imread(ruta, cv::IMREAD_GRAYSCALE);
    if (src.empty()) src = cv::Mat(500, 700, CV_8UC1, cv::Scalar(40));

    cv::Mat bin;
    cv::threshold(src, bin, 127, 255, cv::THRESH_BINARY);
    cv::Mat k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));

    int w = 540, h = 250;

    if (subAccion == 0) { // Erosión y Dilatación
        cv::Mat ero, dil, borde;
        cv::erode(bin, ero, k);
        cv::dilate(bin, dil, k);
        cv::subtract(dil, ero, borde);

        cv::Mat cB, cE, cD, cGrad;
        cv::cvtColor(bin, cB, cv::COLOR_GRAY2BGR);
        cv::cvtColor(ero, cE, cv::COLOR_GRAY2BGR);
        cv::cvtColor(dil, cD, cv::COLOR_GRAY2BGR);
        cv::cvtColor(borde, cGrad, cv::COLOR_GRAY2BGR);

        cv::resize(cB, cB, cv::Size(w, h));
        cv::resize(cE, cE, cv::Size(w, h));
        cv::resize(cD, cD, cv::Size(w, h));
        cv::resize(cGrad, cGrad, cv::Size(w, h));

        cv::putText(cB, "1. Binaria Base", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(cE, "2. Erosion (Adelgazar objetos)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 180, 255), 2);
        cv::putText(cD, "3. Dilatacion (Engrosar objetos)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 100, 0), 2);
        cv::putText(cGrad, "4. Borde Morfologico (Dilat - Erod)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 0, 255), 2);

        cv::Mat f1, f2, out;
        cv::hconcat(cB, cE, f1);
        cv::hconcat(cD, cGrad, f2);
        cv::vconcat(f1, f2, out);
        return out;
    } else if (subAccion == 1) { // Apertura, Cierre, Top-Hat
        cv::Mat ap, cl, th, bh;
        cv::morphologyEx(bin, ap, cv::MORPH_OPEN, k);
        cv::morphologyEx(bin, cl, cv::MORPH_CLOSE, k);
        cv::morphologyEx(bin, th, cv::MORPH_TOPHAT, k);
        cv::morphologyEx(bin, bh, cv::MORPH_BLACKHAT, k);

        cv::Mat cA, cC, cT, cB;
        cv::cvtColor(ap, cA, cv::COLOR_GRAY2BGR);
        cv::cvtColor(cl, cC, cv::COLOR_GRAY2BGR);
        cv::cvtColor(th, cT, cv::COLOR_GRAY2BGR);
        cv::cvtColor(bh, cB, cv::COLOR_GRAY2BGR);

        cv::resize(cA, cA, cv::Size(w, h));
        cv::resize(cC, cC, cv::Size(w, h));
        cv::resize(cT, cT, cv::Size(w, h));
        cv::resize(cB, cB, cv::Size(w, h));

        cv::putText(cA, "1. Apertura (Elimina ruido exterior)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(cC, "2. Cierre (Tapa agujeros internos)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);
        cv::putText(cT, "3. Top-Hat (Elementos mas claros)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 100, 0), 2);
        cv::putText(cB, "4. Black-Hat (Elementos mas oscuros)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 0, 255), 2);

        cv::Mat f1, f2, out;
        cv::hconcat(cA, cC, f1);
        cv::hconcat(cT, cB, f2);
        cv::vconcat(f1, f2, out);
        return out;
    } else { // Comparativa Elementos Estructurantes
        int sz = 13;
        cv::Mat elR = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(sz, sz));
        cv::Mat elC = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(sz, sz));
        cv::Mat elE = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(sz, sz));

        cv::Mat dR, dC, dE;
        cv::dilate(bin, dR, elR);
        cv::dilate(bin, dC, elC);
        cv::dilate(bin, dE, elE);

        cv::Mat cR, cC, cE;
        cv::cvtColor(dR, cR, cv::COLOR_GRAY2BGR);
        cv::cvtColor(dC, cC, cv::COLOR_GRAY2BGR);
        cv::cvtColor(dE, cE, cv::COLOR_GRAY2BGR);

        int sw = 360, sh = 480;
        cv::resize(cR, cR, cv::Size(sw, sh));
        cv::resize(cC, cC, cv::Size(sw, sh));
        cv::resize(cE, cE, cv::Size(sw, sh));

        cv::putText(cR, "Kernel: Rectangulo (13x13)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 0), 2);
        cv::putText(cC, "Kernel: Cruz (13x13)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 200, 255), 2);
        cv::putText(cE, "Kernel: Elipse (13x13)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(255, 100, 0), 2);

        cv::Mat out;
        cv::hconcat(std::vector<cv::Mat>{cR, cC, cE}, out);
        return out;
    }
}

// --- 4.6. SEGMENTACIÓN ---
cv::Mat procesarVistaSegmentacion(int subAccion) {
    std::string ruta = resolverAsset("segmentacion.jpg");
    cv::Mat src = cv::imread(ruta, cv::IMREAD_COLOR);
    if (src.empty()) src = cv::Mat(500, 700, CV_8UC3, cv::Scalar(40, 40, 40));

    int w = 540, h = 480;

    if (subAccion == 0) { // HSV Segmentación (Presets color)
        cv::Mat hsv, mask, obj;
        cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
        // Preset Azul
        cv::inRange(hsv, cv::Scalar(95, 60, 50), cv::Scalar(135, 255, 255), mask);
        cv::bitwise_and(src, src, obj, mask);

        cv::Mat cMask;
        cv::cvtColor(mask, cMask, cv::COLOR_GRAY2BGR);
        cv::resize(cMask, cMask, cv::Size(w, h));
        cv::resize(obj, obj, cv::Size(w, h));

        cv::putText(cMask, "1. Mascara Binaria HSV (Azul)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(obj, "2. Objeto Aislado Segmentado", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);

        cv::Mat out;
        cv::hconcat(cMask, obj, out);
        return out;
    } else if (subAccion == 1) { // Watershed (Cuencas Hidrográficas)
        cv::Mat gray, bin, opening, sure_bg, dist, sure_fg, unknown, markers;
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

        cv::Mat k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(bin, opening, cv::MORPH_OPEN, k, cv::Point(-1, -1), 2);
        cv::dilate(opening, sure_bg, k, cv::Point(-1, -1), 3);

        cv::distanceTransform(opening, dist, cv::DIST_L2, 5);
        double maxVal;
        cv::minMaxLoc(dist, nullptr, &maxVal);
        cv::threshold(dist, sure_fg, 0.5 * maxVal, 255, 0);
        sure_fg.convertTo(sure_fg, CV_8U);

        cv::subtract(sure_bg, sure_fg, unknown);
        cv::connectedComponents(sure_fg, markers);
        markers = markers + 1;
        for (int y = 0; y < unknown.rows; y++) {
            for (int x = 0; x < unknown.cols; x++) {
                if (unknown.at<uchar>(y, x) == 255) markers.at<int>(y, x) = 0;
            }
        }

        cv::Mat srcW = src.clone();
        cv::watershed(srcW, markers);

        for (int i = 0; i < markers.rows; i++) {
            for (int j = 0; j < markers.cols; j++) {
                if (markers.at<int>(i, j) == -1) {
                    srcW.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 255); // Bordes rojos
                }
            }
        }

        cv::Mat distCol;
        cv::normalize(dist, distCol, 0, 255, cv::NORM_MINMAX);
        distCol.convertTo(distCol, CV_8U);
        cv::applyColorMap(distCol, distCol, cv::COLORMAP_JET);

        cv::resize(distCol, distCol, cv::Size(w, h));
        cv::resize(srcW, srcW, cv::Size(w, h));

        cv::putText(distCol, "1. Transformada de Distancia L2", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 255, 255), 2);
        cv::putText(srcW, "2. Segmentacion Watershed (Bordes)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);

        cv::Mat out;
        cv::hconcat(distCol, srcW, out);
        return out;
    } else { // K-Means Clustering
        cv::Mat samples(src.rows * src.cols, 3, CV_32F);
        for (int y = 0; y < src.rows; y++) {
            for (int x = 0; x < src.cols; x++) {
                for (int z = 0; z < 3; z++) samples.at<float>(y + x * src.rows, z) = src.at<cv::Vec3b>(y, x)[z];
            }
        }

        cv::Mat labels, centers;
        cv::kmeans(samples, 3, labels, cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
                   3, cv::KMEANS_PP_CENTERS, centers);

        cv::Mat seg(src.size(), src.type());
        for (int y = 0; y < src.rows; y++) {
            for (int x = 0; x < src.cols; x++) {
                int c = labels.at<int>(y + x * src.rows, 0);
                seg.at<cv::Vec3b>(y, x)[0] = cv::saturate_cast<uchar>(centers.at<float>(c, 0));
                seg.at<cv::Vec3b>(y, x)[1] = cv::saturate_cast<uchar>(centers.at<float>(c, 1));
                seg.at<cv::Vec3b>(y, x)[2] = cv::saturate_cast<uchar>(centers.at<float>(c, 2));
            }
        }

        cv::Mat rSrc, rSeg;
        cv::resize(src, rSrc, cv::Size(w, h));
        cv::resize(seg, rSeg, cv::Size(w, h));

        cv::putText(rSrc, "Imagen Original Color", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
        cv::putText(rSeg, "K-Means (K = 3 Clústeres Dominantes)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);

        cv::Mat out;
        cv::hconcat(rSrc, rSeg, out);
        return out;
    }
}

// --- 4.7. CARACTERIZACIÓN ---
cv::Mat procesarVistaCaracterizacion(int subAccion) {
    std::string ruta = resolverAsset("caracterizacion.jpg");
    cv::Mat src = cv::imread(ruta, cv::IMREAD_COLOR);
    if (src.empty()) src = cv::Mat(500, 700, CV_8UC3, cv::Scalar(40, 40, 40));

    cv::Mat gray, bin;
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    std::vector<std::vector<cv::Point>> contornos;
    cv::findContours(bin, contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    int w = 1080, h = 480;

    if (subAccion == 0) { // Análisis Geométrico (Bounding Boxes, Centroides, Área)
        cv::Mat lienzo = src.clone();
        int id = 1;
        for (const auto& c : contornos) {
            double a = cv::contourArea(c);
            if (a < 500) continue;
            double p = cv::arcLength(c, true);

            cv::Rect bbox = cv::boundingRect(c);
            cv::rectangle(lienzo, bbox, cv::Scalar(0, 255, 0), 2);

            cv::RotatedRect rRect = cv::minAreaRect(c);
            cv::Point2f pts[4];
            rRect.points(pts);
            for (int j = 0; j < 4; j++) cv::line(lienzo, pts[j], pts[(j + 1) % 4], cv::Scalar(0, 255, 255), 2);

            cv::Moments m = cv::moments(c);
            if (m.m00 > 0) {
                int cx = static_cast<int>(m.m10 / m.m00);
                int cy = static_cast<int>(m.m01 / m.m00);
                cv::drawMarker(lienzo, cv::Point(cx, cy), cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 14, 2);
            }

            cv::putText(lienzo, "#" + std::to_string(id) + " A=" + std::to_string((int)a) + "px",
                        cv::Point(bbox.x, std::max(22, bbox.y - 8)), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(20, 20, 20), 3);
            cv::putText(lienzo, "#" + std::to_string(id) + " A=" + std::to_string((int)a) + "px",
                        cv::Point(bbox.x, std::max(22, bbox.y - 8)), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 255, 255), 1);
            id++;
        }

        cv::resize(lienzo, lienzo, cv::Size(w, h));
        cv::rectangle(lienzo, cv::Rect(0, 0, lienzo.cols, 40), cv::Scalar(20, 20, 20), -1);
        cv::putText(lienzo, "Caracterizacion Geometrica: [Verde] Bounding Box | [Amarillo] Rotated Box | [Rojo] Centroide",
                    cv::Point(15, 27), cv::FONT_HERSHEY_SIMPLEX, 0.58, cv::Scalar(0, 255, 255), 2);
        return lienzo;
    } else if (subAccion == 1) { // Momentos de Hu Invariantes
        cv::Mat lienzo = src.clone();
        int id = 1;
        for (const auto& c : contornos) {
            double a = cv::contourArea(c);
            if (a < 600) continue;
            cv::Moments m = cv::moments(c);
            double hu[7];
            cv::HuMoments(m, hu);
            double hu0 = -1.0 * std::copysign(1.0, hu[0]) * std::log10(std::abs(hu[0]) + 1e-30);

            cv::Rect bbox = cv::boundingRect(c);
            cv::rectangle(lienzo, bbox, cv::Scalar(0, 165, 255), 2);
            cv::putText(lienzo, "#" + std::to_string(id) + " Hu[0]=" + std::to_string(hu0).substr(0, 4),
                        cv::Point(bbox.x, std::max(22, bbox.y - 8)), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 0, 0), 2);
            id++;
        }
        cv::resize(lienzo, lienzo, cv::Size(w, h));
        cv::rectangle(lienzo, cv::Rect(0, 0, lienzo.cols, 40), cv::Scalar(20, 20, 20), -1);
        cv::putText(lienzo, "Momentos de Hu (Descriptores Topologicos Invariantes a Escala y Rotacion)",
                    cv::Point(15, 27), cv::FONT_HERSHEY_SIMPLEX, 0.58, cv::Scalar(0, 255, 200), 2);
        return lienzo;
    } else { // Convex Hull y Defectos
        cv::Mat lienzo = src.clone();
        for (const auto& c : contornos) {
            if (cv::contourArea(c) < 600) continue;
            std::vector<cv::Point> hull;
            cv::convexHull(c, hull);
            cv::polylines(lienzo, c, true, cv::Scalar(0, 255, 0), 2);
            cv::polylines(lienzo, hull, true, cv::Scalar(255, 0, 0), 2);
        }
        cv::resize(lienzo, lienzo, cv::Size(w, h));
        cv::rectangle(lienzo, cv::Rect(0, 0, lienzo.cols, 40), cv::Scalar(20, 20, 20), -1);
        cv::putText(lienzo, "[Verde] Contorno Original | [Azul] Envolvente Convexa (Convex Hull)",
                    cv::Point(15, 27), cv::FONT_HERSHEY_SIMPLEX, 0.58, cv::Scalar(0, 255, 255), 2);
        return lienzo;
    }
}

// --- 4.8. ANÁLISIS DE FRECUENCIA ---
cv::Mat calcularFFTMagnitudMat(const cv::Mat& srcGray) {
    cv::Mat padded;
    int m = cv::getOptimalDFTSize(srcGray.rows);
    int n = cv::getOptimalDFTSize(srcGray.cols);
    cv::copyMakeBorder(srcGray, padded, 0, m - srcGray.rows, 0, n - srcGray.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));

    cv::Mat planes[] = { cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F) };
    cv::Mat complexImg;
    cv::merge(planes, 2, complexImg);
    cv::dft(complexImg, complexImg);

    cv::split(complexImg, planes);
    cv::Mat mag;
    cv::magnitude(planes[0], planes[1], mag);
    mag += cv::Scalar::all(1);
    cv::log(mag, mag);

    mag = mag(cv::Rect(0, 0, mag.cols & -2, mag.rows & -2));
    int cx = mag.cols / 2;
    int cy = mag.rows / 2;
    cv::Mat q0(mag, cv::Rect(0, 0, cx, cy));
    cv::Mat q1(mag, cv::Rect(cx, 0, cx, cy));
    cv::Mat q2(mag, cv::Rect(0, cy, cx, cy));
    cv::Mat q3(mag, cv::Rect(cx, cy, cx, cy));

    cv::Mat tmp;
    q0.copyTo(tmp); q3.copyTo(q0); tmp.copyTo(q3);
    q1.copyTo(tmp); q2.copyTo(q1); tmp.copyTo(q2);

    cv::normalize(mag, mag, 0, 1, cv::NORM_MINMAX);
    return mag;
}

cv::Mat procesarVistaFrecuencia(int subAccion) {
    std::string ruta = resolverAsset("lenna.jpg");
    cv::Mat src = cv::imread(ruta, cv::IMREAD_GRAYSCALE);
    if (src.empty()) src = cv::Mat(400, 400, CV_8UC1, cv::Scalar(128));

    cv::Mat fftMag = calcularFFTMagnitudMat(src);

    int w = 540, h = 480;
    cv::Mat rOrig, rFFT;
    cv::resize(src, rOrig, cv::Size(w, h));
    cv::resize(fftMag, rFFT, cv::Size(w, h));

    cv::cvtColor(rOrig, rOrig, cv::COLOR_GRAY2BGR);
    rFFT.convertTo(rFFT, CV_8U, 255.0);
    cv::cvtColor(rFFT, rFFT, cv::COLOR_GRAY2BGR);

    cv::putText(rOrig, "Dominio Espacial (Lena)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
    cv::putText(rFFT, "Espectro de Magnitud Fourier (DFT 2D)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);

    cv::Mat out;
    cv::hconcat(rOrig, rFFT, out);
    return out;
}

// ============================================================================
// 5. NAVEGACIÓN Y RENDERIZADO PRINCIPAL
// ============================================================================
void inicializarTarjetasMenu() {
    g_app.tarjetas.clear();

    // 8 Módulos dispuestos en 4 columnas x 2 filas
    int w = 270, h = 260;
    int gapX = 22, gapY = 22;
    int startX = (1200 - (4 * w + 3 * gapX)) / 2;
    int startY = 115;

    std::vector<CardInfo> items = {
        { 0, "MODULO 1", "Camara & Calibracion", "Modelos Pinhole | Homografia 3x3 y Undistort", Theme::ACCENT_GOLD, cv::Rect() },
        { 1, "MODULO 2", "Filtros Espaciales", "Suavizado, Sobel, Canny | Sharpening y Realce", Theme::ACCENT_GREEN, cv::Rect() },
        { 2, "MODULO 3", "Histogramas", "Grises, Color RGB | Ecualizacion Global & CLAHE", Theme::ACCENT_CYAN, cv::Rect() },
        { 3, "MODULO 4", "Binarizacion", "Umbral Fijo, Otsu | Adaptativa Media y Gauss", Theme::ACCENT_BLUE, cv::Rect() },
        { 4, "MODULO 5", "Morfologia", "Erosion, Dilatacion | Apertura, Cierre, Top-Hat", Theme::ACCENT_GOLD, cv::Rect() },
        { 5, "MODULO 6", "Segmentacion", "Color HSV, Watershed | Clustered K-Means", Theme::ACCENT_GREEN, cv::Rect() },
        { 6, "MODULO 7", "Caracterizacion", "Contornos, Momentos Hu | Convex Hull y Esquinas", Theme::ACCENT_CYAN, cv::Rect() },
        { 7, "MODULO 8", "Frecuencia (FFT)", "Transformada Fourier 2D | Espectro de Magnitud", Theme::ACCENT_BLUE, cv::Rect() }
    };

    for (int i = 0; i < 8; i++) {
        int col = i % 4;
        int row = i / 4;
        items[i].bounds = cv::Rect(startX + col * (w + gapX), startY + row * (h + gapY), w, h);
        g_app.tarjetas.push_back(items[i]);
    }
}

void actualizarTabsModulo() {
    g_app.tabs.clear();

    std::vector<std::string> titulos;
    switch (g_app.moduloActual) {
        case 0: titulos = { "1. Calibracion Pinhole", "2. Homografia 3x3", "3. Camara en Vivo" }; break;
        case 1: titulos = { "1. Suavizado", "2. Bordes & Gradientes", "3. Sharpening", "4. Camara en Vivo" }; break;
        case 2: titulos = { "1. Histograma Grises", "2. Histograma RGB", "3. Ecualizacion vs CLAHE", "4. Camara en Vivo" }; break;
        case 3: titulos = { "1. Comparativa 2x2", "2. Histograma Otsu", "3. Camara en Vivo" }; break;
        case 4: titulos = { "1. Erosion / Dilatacion", "2. Apertura & Cierre", "3. Formas de Kernel", "4. Camara en Vivo" }; break;
        case 5: titulos = { "1. Color HSV", "2. Watershed Paso a Paso", "3. K-Means (K=3)", "4. Camara en Vivo" }; break;
        case 6: titulos = { "1. Analisis Geometrico", "2. Momentos de Hu", "3. Convex Hull", "4. Camara en Vivo" }; break;
        case 7: titulos = { "1. Espectro FFT 2D", "2. Camara en Vivo" }; break;
    }

    int tabX = 220;
    int tabY = 16;
    int tabH = 38;

    for (size_t i = 0; i < titulos.size(); i++) {
        int tabW = 180;
        TabInfo t;
        t.id = static_cast<int>(i);
        t.label = titulos[i];
        t.bounds = cv::Rect(tabX, tabY, tabW, tabH);
        tabX += tabW + 12;
        g_app.tabs.push_back(t);
    }
}

// Ejecución de cámara web en vivo para cualquier módulo
void ejecutarCamaraEnModulo(int moduloId) {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "[!] No se detecto camara web conectada.\n";
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    const std::string camWin = "Camara en Vivo - Presiona [ESC] o [Q] para volver";
    cv::namedWindow(camWin, cv::WINDOW_AUTOSIZE);

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat proc;
        std::string tag = "";

        if (moduloId == 0) { // Calibración
            proc = frame.clone();
            cv::Size sz(9, 6);
            std::vector<cv::Point2f> corners;
            cv::Mat gray;
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            if (cv::findChessboardCorners(gray, sz, corners, cv::CALIB_CB_FAST_CHECK)) {
                cv::drawChessboardCorners(proc, sz, corners, true);
                tag = "Tablero Detectado (9x6)";
            } else tag = "Buscando Tablero...";
        } else if (moduloId == 1) { // Filtros (Canny)
            cv::Mat g;
            cv::cvtColor(frame, g, cv::COLOR_BGR2GRAY);
            cv::Canny(g, proc, 50, 150);
            cv::cvtColor(proc, proc, cv::COLOR_GRAY2BGR);
            tag = "Filtro Canny en Vivo";
        } else if (moduloId == 2) { // Histogramas (CLAHE)
            cv::Mat g;
            cv::cvtColor(frame, g, cv::COLOR_BGR2GRAY);
            cv::Ptr<cv::CLAHE> cl = cv::createCLAHE(3.0, cv::Size(8, 8));
            cl->apply(g, proc);
            cv::cvtColor(proc, proc, cv::COLOR_GRAY2BGR);
            tag = "CLAHE en Vivo";
        } else if (moduloId == 3) { // Binarización (Otsu)
            cv::Mat g;
            cv::cvtColor(frame, g, cv::COLOR_BGR2GRAY);
            cv::threshold(g, proc, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
            cv::cvtColor(proc, proc, cv::COLOR_GRAY2BGR);
            tag = "Otsu en Vivo";
        } else if (moduloId == 4) { // Morfología (Gradiente)
            cv::Mat g, b;
            cv::cvtColor(frame, g, cv::COLOR_BGR2GRAY);
            cv::threshold(g, b, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
            cv::Mat el = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
            cv::morphologyEx(b, proc, cv::MORPH_GRADIENT, el);
            cv::cvtColor(proc, proc, cv::COLOR_GRAY2BGR);
            tag = "Gradiente Morfologico en Vivo";
        } else if (moduloId == 5) { // Segmentación (Azul)
            cv::Mat hsv, m;
            cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
            cv::inRange(hsv, cv::Scalar(95, 80, 50), cv::Scalar(135, 255, 255), m);
            cv::bitwise_and(frame, frame, proc, m);
            tag = "Tracking Color Azul en Vivo";
        } else if (moduloId == 6) { // Caracterización (Contornos y Bounding Boxes)
            proc = frame.clone();
            cv::Mat g, b;
            cv::cvtColor(frame, g, cv::COLOR_BGR2GRAY);
            cv::threshold(g, b, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
            std::vector<std::vector<cv::Point>> c;
            cv::findContours(b, c, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            for (const auto& ci : c) {
                if (cv::contourArea(ci) > 1200) {
                    cv::Rect r = cv::boundingRect(ci);
                    cv::rectangle(proc, r, cv::Scalar(0, 255, 0), 2);
                }
            }
            tag = "Caracterizacion en Vivo";
        } else { // Frecuencia (FFT)
            cv::Mat g;
            cv::cvtColor(frame, g, cv::COLOR_BGR2GRAY);
            cv::resize(g, g, cv::Size(320, 240));
            cv::Mat mag = calcularFFTMagnitudMat(g);
            mag.convertTo(proc, CV_8U, 255.0);
            cv::cvtColor(proc, proc, cv::COLOR_GRAY2BGR);
            cv::resize(proc, proc, frame.size());
            tag = "Espectro FFT en Vivo";
        }

        cv::putText(frame, "Camara Original", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.70, cv::Scalar(0, 255, 0), 2);
        cv::putText(proc, tag, cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.70, cv::Scalar(0, 220, 255), 2);

        cv::Mat comparativa;
        cv::hconcat(frame, proc, comparativa);
        cv::imshow(camWin, comparativa);

        char k = static_cast<char>(cv::waitKey(15));
        if (k == 27 || k == 'q' || k == 'Q') break;
    }

    cap.release();
    cv::destroyWindow(camWin);
}

// Renderizado completo del lienzo
void renderizarInterfaz() {
    g_app.canvas = cv::Mat(760, 1200, CV_8UC3, Theme::BG_DARK);

    if (g_app.estado == STATE_MAIN_MENU) {
        // --- HEADER PRINCIPAL ---
        cv::rectangle(g_app.canvas, cv::Rect(0, 0, 1200, 90), Theme::HEADER_BG, -1);
        cv::line(g_app.canvas, cv::Point(0, 90), cv::Point(1200, 90), Theme::ACCENT_GOLD, 2);

        cv::putText(g_app.canvas, "VISION ARTIFICIAL - PLAYGROUND DE PROYECTOS",
                    cv::Point(35, 42), cv::FONT_HERSHEY_DUPLEX, 0.85, Theme::TEXT_WHITE, 2, cv::LINE_AA);
        cv::putText(g_app.canvas, "Entrega Final: Suite Grafica Interactiva | Procesamiento Digital de Imagenes",
                    cv::Point(35, 70), cv::FONT_HERSHEY_SIMPLEX, 0.48, Theme::TEXT_GRAY, 1, cv::LINE_AA);

        cv::Rect badge(980, 26, 185, 36);
        cv::rectangle(g_app.canvas, badge, cv::Scalar(42, 35, 30), -1);
        cv::rectangle(g_app.canvas, badge, Theme::ACCENT_GREEN, 1);
        cv::circle(g_app.canvas, cv::Point(998, 44), 5, Theme::ACCENT_GREEN, -1);
        cv::putText(g_app.canvas, "OpenCV 4.14 | C++17", cv::Point(1012, 49),
                    cv::FONT_HERSHEY_SIMPLEX, 0.45, Theme::TEXT_WHITE, 1, cv::LINE_AA);

        // Tarjetas
        for (auto& card : g_app.tarjetas) {
            dibujarTarjetaMenu(g_app.canvas, card);
        }

        // Footer
        cv::rectangle(g_app.canvas, cv::Rect(0, 715, 1200, 45), cv::Scalar(18, 15, 13), -1);
        cv::putText(g_app.canvas, "💡 Selecciona cualquier tarjeta con un clic. En cualquier momento presiona [ESC] para salir o volver.",
                    cv::Point(35, 742), cv::FONT_HERSHEY_SIMPLEX, 0.46, Theme::TEXT_GRAY, 1, cv::LINE_AA);

    } else if (g_app.estado == STATE_MODULE_VIEW) {
        // --- HEADER DEL MÓDULO ---
        cv::rectangle(g_app.canvas, cv::Rect(0, 0, 1200, 70), Theme::HEADER_BG, -1);
        cv::line(g_app.canvas, cv::Point(0, 70), cv::Point(1200, 70), Theme::CARD_BORDER, 1);

        // Botón Volver al Menú
        dibujarBoton(g_app.canvas, g_app.btnVolver, "< VOLVER AL MENU", g_app.hoverVolver, false,
                     cv::Scalar(50, 42, 36), Theme::TAB_ACTIVE);

        // Tabs de Sub-Acciones
        for (auto& tab : g_app.tabs) {
            bool active = (tab.id == g_app.subAccionActual);
            dibujarBoton(g_app.canvas, tab.bounds, tab.label, tab.hover, active,
                         Theme::TAB_INACTIVE, Theme::TAB_ACTIVE);
        }

        // Título del módulo
        std::string modTag = g_app.tarjetas[g_app.moduloActual].titulo;
        cv::putText(g_app.canvas, modTag, cv::Point(1200 - 320, 42),
                    cv::FONT_HERSHEY_DUPLEX, 0.62, Theme::ACCENT_GOLD, 1, cv::LINE_AA);

        // --- ÁREA DE CONTENIDO (VIEWPORT) ---
        cv::Mat viewMat;
        switch (g_app.moduloActual) {
            case 0: viewMat = procesarVistaCalibracion(g_app.subAccionActual); break;
            case 1: viewMat = procesarVistaFiltros(g_app.subAccionActual); break;
            case 2: viewMat = procesarVistaHistogramas(g_app.subAccionActual); break;
            case 3: viewMat = procesarVistaBinarizacion(g_app.subAccionActual); break;
            case 4: viewMat = procesarVistaMorfologia(g_app.subAccionActual); break;
            case 5: viewMat = procesarVistaSegmentacion(g_app.subAccionActual); break;
            case 6: viewMat = procesarVistaCaracterizacion(g_app.subAccionActual); break;
            case 7: viewMat = procesarVistaFrecuencia(g_app.subAccionActual); break;
        }

        if (!viewMat.empty()) {
            int maxW = 1140, maxH = 640;
            double scale = std::min((double)maxW / viewMat.cols, (double)maxH / viewMat.rows);
            cv::Mat scaled;
            cv::resize(viewMat, scaled, cv::Size(), scale, scale);

            int startX = (1200 - scaled.cols) / 2;
            int startY = 85 + (650 - scaled.rows) / 2;
            cv::Rect roi(startX, startY, scaled.cols, scaled.rows);
            scaled.copyTo(g_app.canvas(roi));

            cv::rectangle(g_app.canvas, roi, Theme::CARD_BORDER, 1);
        }
    }
}

// ============================================================================
// 6. GESTOR DE EVENTOS DEL RATÓN
// ============================================================================
void onMouseCallback(int event, int x, int y, int, void*) {
    g_app.mouseX = x;
    g_app.mouseY = y;

    if (g_app.estado == STATE_MAIN_MENU) {
        bool cambioHover = false;
        for (auto& card : g_app.tarjetas) {
            bool prev = card.hover;
            card.hover = card.bounds.contains(cv::Point(x, y));
            if (prev != card.hover) cambioHover = true;

            if (event == cv::EVENT_LBUTTONDOWN && card.hover) {
                g_app.moduloActual = card.id;
                g_app.subAccionActual = 0;
                g_app.estado = STATE_MODULE_VIEW;
                actualizarTabsModulo();
                g_app.necesitaRedibujar = true;
                return;
            }
        }
        if (cambioHover) g_app.necesitaRedibujar = true;

    } else if (g_app.estado == STATE_MODULE_VIEW) {
        bool cambioHover = false;

        // Botón Volver
        bool prevVolver = g_app.hoverVolver;
        g_app.hoverVolver = g_app.btnVolver.contains(cv::Point(x, y));
        if (prevVolver != g_app.hoverVolver) cambioHover = true;

        if (event == cv::EVENT_LBUTTONDOWN && g_app.hoverVolver) {
            g_app.estado = STATE_MAIN_MENU;
            g_app.necesitaRedibujar = true;
            return;
        }

        // Tabs
        for (auto& tab : g_app.tabs) {
            bool prevTab = tab.hover;
            tab.hover = tab.bounds.contains(cv::Point(x, y));
            if (prevTab != tab.hover) cambioHover = true;

            if (event == cv::EVENT_LBUTTONDOWN && tab.hover) {
                // Si seleccionó la última pestaña (Cámara en Vivo)
                if (tab.id == static_cast<int>(g_app.tabs.size()) - 1) {
                    ejecutarCamaraEnModulo(g_app.moduloActual);
                    g_app.necesitaRedibujar = true;
                    return;
                } else {
                    g_app.subAccionActual = tab.id;
                    cambioHover = true;
                }
            }
        }

        if (cambioHover) g_app.necesitaRedibujar = true;
    }
}

// ============================================================================
// 7. FUNCIÓN PRINCIPAL (MAIN)
// ============================================================================
int main() {
    std::cout << "==============================================================\n";
    std::cout << "  INICIANDO SUITE INTERACTIVA - VISION ARTIFICIAL (GUI)\n";
    std::cout << "==============================================================\n";
    std::cout << "[+] Cargando interfaz grafica moderna...\n";
    std::cout << "[+] Resolviendo recursos e imagenes de prueba...\n";
    std::cout << "[+] Ventana interactiva lista.\n\n";

    inicializarTarjetasMenu();

    cv::namedWindow(g_app.windowName, cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback(g_app.windowName, onMouseCallback, nullptr);

    renderizarInterfaz();
    cv::imshow(g_app.windowName, g_app.canvas);

    while (!g_app.salirApp) {
        if (g_app.necesitaRedibujar) {
            renderizarInterfaz();
            cv::imshow(g_app.windowName, g_app.canvas);
            g_app.necesitaRedibujar = false;
        }

        int key = cv::waitKey(25);
        if (key == 27) { // Tecla ESC
            if (g_app.estado == STATE_MODULE_VIEW) {
                g_app.estado = STATE_MAIN_MENU;
                g_app.necesitaRedibujar = true;
            } else {
                g_app.salirApp = true;
            }
        } else if (key == 'q' || key == 'Q') {
            if (g_app.estado == STATE_MODULE_VIEW) {
                g_app.estado = STATE_MAIN_MENU;
                g_app.necesitaRedibujar = true;
            } else {
                g_app.salirApp = true;
            }
        }
    }

    cv::destroyAllWindows();
    std::cout << "[+] Aplicacion cerrada correctamente.\n";
    return 0;
}
