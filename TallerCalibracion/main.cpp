#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <iomanip>

namespace fs = std::filesystem;

// ============================================================================
// 1. LOCALIZADOR AUTOMÁTICO DE IMAGEN (PORTABLE Y RELATIVO)
// ============================================================================
std::string resolverRutaImagen(const std::string& preferida = "tablero_calibracion.png") {
    std::vector<std::string> candidatos = {
        preferida,
        "../" + preferida,
        "../../" + preferida,
        "../../../" + preferida,
        "tablero_calibracion.png",
        "../tablero_calibracion.png",
        "input.jpg",
        "../input.jpg"
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
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                    return entry.path().string();
                }
            }
        }
    } catch (...) {}

    return preferida;
}

// ============================================================================
// 2. RECTIFICACIÓN PROYECTIVA / HOMOGRAFÍA (3x3): ENCUADRE ORTOGONAL PLANO
// ============================================================================
cv::Mat encuadrarTableroFrontal(const cv::Mat& src, 
                                const std::vector<cv::Point2f>& corners, 
                                cv::Size pattern_size) {
    const int winW = 600;
    const int winH = 600;

    cv::Mat tableroRecto(winH, winW, CV_8UC3, cv::Scalar(240, 240, 240));

    int numCasillasX = pattern_size.width + 1;
    int numCasillasY = pattern_size.height + 1;

    float margen = 40.0f;
    float maxW = winW - (2.0f * margen);
    float maxH = winH - (2.0f * margen);

    float squarePx = std::min(maxW / numCasillasX, maxH / numCasillasY);
    float boardW = numCasillasX * squarePx;
    float boardH = numCasillasY * squarePx;

    float offsetX = (winW - boardW) / 2.0f;
    float offsetY = (winH - boardH) / 2.0f;

    std::vector<cv::Point2f> ptsDestino;
    ptsDestino.reserve(pattern_size.width * pattern_size.height);

    for (int i = 0; i < pattern_size.height; i++) {
        for (int j = 0; j < pattern_size.width; j++) {
            float x = offsetX + (j + 1) * squarePx;
            float y = offsetY + (i + 1) * squarePx;
            ptsDestino.push_back(cv::Point2f(x, y));
        }
    }

    cv::Mat H = cv::findHomography(corners, ptsDestino, cv::RANSAC);
    if (H.empty()) return cv::Mat();

    cv::warpPerspective(src, tableroRecto, H, cv::Size(winW, winH),
                        cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(240, 240, 240));

    return tableroRecto;
}

// ============================================================================
// 3. DETECTOR ROBUSTO DE TABLERO DE AJEDREZ (SECTOR-BASED Y CLÁSICO)
// ============================================================================
bool detectarTableroRobustoTablet(const cv::Mat& srcBgr, 
                                  cv::Size& patronDetectado, 
                                  std::vector<cv::Point2f>& cornersOut) {
    cv::Mat gray;
    cv::cvtColor(srcBgr, gray, cv::COLOR_BGR2GRAY);

    std::vector<cv::Size> candidatos = {
        cv::Size(9, 6), cv::Size(6, 9),   // Tablero estándar de calibración (10x7 casillas)
        cv::Size(10, 6), cv::Size(6, 10), // Tablet horizontal y vertical
        cv::Size(7, 7),                   // Ajedrez 8x8 casillas
        cv::Size(8, 6),  cv::Size(6, 8)
    };

    int flagsSB = cv::CALIB_CB_EXHAUSTIVE | cv::CALIB_CB_ACCURACY | cv::CALIB_CB_NORMALIZE_IMAGE;
    for (const auto& sz : candidatos) {
        std::vector<cv::Point2f> cornersSB;
        if (cv::findChessboardCornersSB(gray, sz, cornersSB, flagsSB)) {
            patronDetectado = sz;
            cornersOut = cornersSB;
            return true;
        }
    }

    cv::Mat graySmooth;
    cv::GaussianBlur(gray, graySmooth, cv::Size(3, 3), 0.5);

    int flagsClassic = cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;
    for (const auto& sz : candidatos) {
        std::vector<cv::Point2f> tempCorners;
        if (cv::findChessboardCorners(graySmooth, sz, tempCorners, flagsClassic)) {
            patronDetectado = sz;
            cornersOut = tempCorners;
            return true;
        }
    }

    return false;
}

// ============================================================================
// 4. CALIBRACIÓN COMPLETA DE CÁMARA PINHOLE Y CORRECCIÓN DE DISTORSIÓN
// ============================================================================
void ejecutarCalibracionPinhole(const std::string& rutaTablero) {
    std::cout << "\n==============================================================\n";
    std::cout << "  MODELO DE CAMARA PINHOLE: CALIBRACION INTRINSECA Y DISTORSION\n";
    std::cout << "==============================================================\n";

    cv::Mat baseImg = cv::imread(rutaTablero, cv::IMREAD_COLOR);
    if (baseImg.empty()) {
        std::cerr << "[!] No se pudo cargar el tablero de calibracion: " << rutaTablero << "\n";
        return;
    }

    cv::Size patternSize(9, 6);
    float squareSizeMm = 25.0f; // Tamaño de casilla en mm

    // Puntos 3D del mundo real (Z = 0 en el plano del tablero)
    std::vector<cv::Point3f> objP;
    for (int i = 0; i < patternSize.height; i++) {
        for (int j = 0; j < patternSize.width; j++) {
            objP.push_back(cv::Point3f(j * squareSizeMm, i * squareSizeMm, 0.0f));
        }
    }

    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imagePoints;

    // Generar 4 perspectivas sintéticas rotadas/inclinadas del tablero para calibrar
    std::vector<cv::Mat> vistas;
    vistas.push_back(baseImg.clone());

    // Vista con perspectiva leve
    std::vector<cv::Point2f> srcQuad = {
        cv::Point2f(0, 0), cv::Point2f((float)baseImg.cols, 0),
        cv::Point2f((float)baseImg.cols, (float)baseImg.rows), cv::Point2f(0, (float)baseImg.rows)
    };
    std::vector<cv::Point2f> dstQuad1 = {
        cv::Point2f(50, 40), cv::Point2f((float)baseImg.cols - 80, 20),
        cv::Point2f((float)baseImg.cols - 30, (float)baseImg.rows - 50), cv::Point2f(80, (float)baseImg.rows - 20)
    };
    std::vector<cv::Point2f> dstQuad2 = {
        cv::Point2f(70, 20), cv::Point2f((float)baseImg.cols - 40, 50),
        cv::Point2f((float)baseImg.cols - 90, (float)baseImg.rows - 30), cv::Point2f(30, (float)baseImg.rows - 60)
    };

    cv::Mat warp1, warp2;
    cv::warpPerspective(baseImg, warp1, cv::getPerspectiveTransform(srcQuad, dstQuad1), baseImg.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
    cv::warpPerspective(baseImg, warp2, cv::getPerspectiveTransform(srcQuad, dstQuad2), baseImg.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
    vistas.push_back(warp1);
    vistas.push_back(warp2);

    std::cout << "[+] Extrayendo esquinas en " << vistas.size() << " vistas de calibracion...\n";

    for (size_t v = 0; v < vistas.size(); v++) {
        cv::Mat gray;
        cv::cvtColor(vistas[v], gray, cv::COLOR_BGR2GRAY);
        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCornersSB(gray, patternSize, corners, cv::CALIB_CB_EXHAUSTIVE);
        if (found) {
            imagePoints.push_back(corners);
            objectPoints.push_back(objP);
            std::cout << "    -> Vista #" << (v + 1) << ": 54 esquinas detectadas con precision subpixel.\n";
        }
    }

    if (imagePoints.empty()) {
        std::cerr << "[!] No se pudieron detectar esquinas suficientes.\n";
        return;
    }

    // Calcular matriz de cámara K y coeficientes de distorsión
    cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
    std::vector<cv::Mat> rvecs, tvecs;

    double rms = cv::calibrateCamera(objectPoints, imagePoints, baseImg.size(),
                                     cameraMatrix, distCoeffs, rvecs, tvecs);

    std::cout << "\n--------------------------------------------------------------\n";
    std::cout << "                 RESULTADOS DE LA CALIBRACION                 \n";
    std::cout << "--------------------------------------------------------------\n";
    std::cout << "Error de reproyeccion RMS: " << std::fixed << std::setprecision(4) << rms << " px\n\n";

    double fx = cameraMatrix.at<double>(0, 0);
    double fy = cameraMatrix.at<double>(1, 1);
    double cx = cameraMatrix.at<double>(0, 2);
    double cy = cameraMatrix.at<double>(1, 2);

    std::cout << "Matriz Intrinseca de Camara (K):\n";
    std::cout << "  [ fx: " << std::setw(8) << fx << "   0.0000   cx: " << std::setw(8) << cx << " ]\n";
    std::cout << "  [  0.0000   fy: " << std::setw(8) << fy << "   cy: " << std::setw(8) << cy << " ]\n";
    std::cout << "  [  0.0000     0.0000    1.0000 ]\n\n";

    std::cout << "Coeficientes de Distorsion de Lente [k1, k2, p1, p2, k3]:\n  [ ";
    for (int i = 0; i < std::min(5, distCoeffs.rows); i++) {
        std::cout << std::scientific << std::setprecision(3) << distCoeffs.at<double>(i) << (i < 4 ? ", " : " ]\n");
    }
    std::cout << "--------------------------------------------------------------\n";

    // Demostración de corrección con cv::undistort
    cv::Mat undistorted;
    cv::undistort(baseImg, undistorted, cameraMatrix, distCoeffs);

    // Dibujar esquinas sobre la imagen original para visualización
    cv::Mat cornersDisplay = baseImg.clone();
    cv::drawChessboardCorners(cornersDisplay, patternSize, imagePoints[0], true);

    int w = 420, h = 320;
    cv::Mat rOrig, rUndist;
    cv::resize(cornersDisplay, rOrig, cv::Size(w, h));
    cv::resize(undistorted, rUndist, cv::Size(w, h));

    cv::putText(rOrig, "Esquinas Detectadas (K)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
    cv::putText(rUndist, "Imagen Corregida (Undistort)", cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 200, 255), 2);

    cv::Mat comparativa;
    cv::hconcat(rOrig, rUndist, comparativa);

    cv::imshow("Calibracion Intrinseca y Correccion de Distorsion", comparativa);
    std::cout << "\n[+] Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Calibracion Intrinseca y Correccion de Distorsion");
}

// ============================================================================
// 5. MODO EN VIVO: CÁMARA, DETECCIÓN Y HOMOGRAFÍA EN TABLET
// ============================================================================
void procesarCalibracionCamara() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "\n[ERROR] No se pudo acceder a la camara web.\n";
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    const std::string win1 = "1. Camara en Vivo";
    const std::string win2 = "2. Fotograma Capturado";
    const std::string win3 = "3. Tablero Rectificado Frontal";

    cv::namedWindow(win1, cv::WINDOW_AUTOSIZE);
    cv::namedWindow(win2, cv::WINDOW_AUTOSIZE);
    cv::namedWindow(win3, cv::WINDOW_AUTOSIZE);

    std::cout << "\n[+] Camara iniciada.\n";
    std::cout << " [INSTRUCCIONES]:\n";
    std::cout << "  - Muestra un tablero o la imagen de ajedrez a la camara.\n";
    std::cout << "  - Presiona 'ESPACIO' para capturar y rectificar.\n";
    std::cout << "  - Presiona 'ESC' o 'q' para salir al menu.\n";

    cv::TermCriteria criteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.01);

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat liveDisplay = frame.clone();
        cv::rectangle(liveDisplay, cv::Rect(0, 0, liveDisplay.cols, 40), cv::Scalar(18, 18, 18), -1);
        cv::putText(liveDisplay, "Presiona [ESPACIO] para capturar | [ESC/Q] Salir",
                    cv::Point(15, 26), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 0), 2);

        cv::imshow(win1, liveDisplay);

        int key = cv::waitKey(15);
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }

        if (key == 32 || key == ' ') {
            std::cout << "[+] Procesando fotograma capturado...\n";
            cv::Mat frameCongelado = frame.clone();
            cv::Size patronDetectado;
            std::vector<cv::Point2f> esquinas;

            bool detectado = detectarTableroRobustoTablet(frameCongelado, patronDetectado, esquinas);
            cv::Mat displayCaptura = frameCongelado.clone();

            if (detectado) {
                cv::Mat graySubPix;
                cv::cvtColor(frameCongelado, graySubPix, cv::COLOR_BGR2GRAY);
                cv::cornerSubPix(graySubPix, esquinas, cv::Size(5, 5), cv::Size(-1, -1), criteria);

                cv::drawChessboardCorners(displayCaptura, patronDetectado, esquinas, true);
                cv::rectangle(displayCaptura, cv::Rect(0, 0, displayCaptura.cols, 40), cv::Scalar(0, 120, 0), -1);
                std::string okStr = "DETECTADO (" + std::to_string(patronDetectado.width) + "x" +
                                    std::to_string(patronDetectado.height) + " esquinas)";
                cv::putText(displayCaptura, okStr, cv::Point(15, 26), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(255, 255, 255), 2);

                cv::Mat frontal = encuadrarTableroFrontal(frameCongelado, esquinas, patronDetectado);
                if (!frontal.empty()) {
                    cv::rectangle(frontal, cv::Rect(0, 0, frontal.cols, 40), cv::Scalar(20, 20, 20), -1);
                    cv::putText(frontal, "Tablero Rectificado Frontal (Homografia 3x3)", cv::Point(15, 26),
                                cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 0), 2);
                    cv::imshow(win3, frontal);
                }
                std::cout << "    -> [EXITO] Patron detectado: " << patronDetectado.width << "x" << patronDetectado.height << "\n";
            } else {
                cv::rectangle(displayCaptura, cv::Rect(0, 0, displayCaptura.cols, 40), cv::Scalar(0, 0, 140), -1);
                cv::putText(displayCaptura, "NO DETECTADO: Reintenta con [ESPACIO]", cv::Point(15, 26),
                            cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 255, 255), 2);
                std::cout << "    -> [FALLO] No se detecto el patron. Asegura buen contraste e iluminacion.\n";
            }

            cv::imshow(win2, displayCaptura);
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
    std::string rutaTablero = resolverRutaImagen("tablero_calibracion.png");

    while (opcion != 0) {
        std::cout << "\n==============================================================\n";
        std::cout << "  VISION ARTIFICIAL: MODELOS DE CAMARA Y CALIBRACION          \n";
        std::cout << "==============================================================\n";
        std::cout << "Patron detectado: " << rutaTablero << "\n\n";
        std::cout << "1. Calibracion de Camara Pinhole (Matriz K, Distorsion y Undistort)\n";
        std::cout << "2. Rectificacion Proyectiva y Homografia 3x3 (Imagen de Tablero)\n";
        std::cout << "3. Modo en Vivo con Camara Web (Captura y Rectificacion Frontal)\n";
        std::cout << "0. Salir\n";
        std::cout << "Selecciona una opcion: ";

        if (!(std::cin >> opcion)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            opcion = -1;
            continue;
        }

        switch (opcion) {
            case 1:
                ejecutarCalibracionPinhole(rutaTablero);
                break;
            case 2: {
                cv::Mat baseImg = cv::imread(rutaTablero, cv::IMREAD_COLOR);
                if (baseImg.empty()) {
                    std::cerr << "[!] Error cargando " << rutaTablero << "\n";
                } else {
                    cv::Size sz;
                    std::vector<cv::Point2f> corners;
                    if (detectarTableroRobustoTablet(baseImg, sz, corners)) {
                        cv::Mat frontal = encuadrarTableroFrontal(baseImg, corners, sz);
                        cv::imshow("Tablero Rectificado Frontal", frontal);
                        std::cout << "[+] Presiona cualquier tecla para continuar.\n";
                        cv::waitKey(0);
                        cv::destroyWindow("Tablero Rectificado Frontal");
                    } else {
                        std::cerr << "[!] No se detecto el tablero en la imagen.\n";
                    }
                }
                break;
            }
            case 3:
                procesarCalibracionCamara();
                break;
            case 0:
                std::cout << "\nSaliendo del taller de Calibracion...\n";
                break;
            default:
                std::cout << "\n[!] Opcion no valida.\n";
                break;
        }
    }

    return 0;
}
