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
// 2. OPCIÓN 1: OPERACIONES BÁSICAS (EROSIÓN Y DILATACIÓN)
// ============================================================================
void mostrarErosionDilatacion(const cv::Mat& src) {
    cv::Mat bin;
    if (src.channels() == 3) {
        cv::cvtColor(src, bin, cv::COLOR_BGR2GRAY);
    } else {
        bin = src.clone();
    }
    // Asegurar imagen binaria bien definida
    cv::threshold(bin, bin, 127, 255, cv::THRESH_BINARY);

    // Elemento estructurante rectangular 5x5
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));

    cv::Mat erosionada, dilatada, bordeGradiente;
    cv::erode(bin, erosionada, element);
    cv::dilate(bin, dilatada, element);
    cv::subtract(dilatada, erosionada, bordeGradiente);

    int w = 380, h = 280;
    cv::Mat rOrig, rEro, rDil, rBorde;
    cv::resize(bin, rOrig, cv::Size(w, h));
    cv::resize(erosionada, rEro, cv::Size(w, h));
    cv::resize(dilatada, rDil, cv::Size(w, h));
    cv::resize(bordeGradiente, rBorde, cv::Size(w, h));

    cv::cvtColor(rOrig, rOrig, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rEro, rEro, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rDil, rDil, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rBorde, rBorde, cv::COLOR_GRAY2BGR);

    cv::putText(rOrig, "1. Binaria Original", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
    cv::putText(rEro, "2. Erosion (Adelgazar)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 150, 255), 2);
    cv::putText(rDil, "3. Dilatacion (Engrosar)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 120, 0), 2);
    cv::putText(rBorde, "4. Borde (Dilat - Erod)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(255, 0, 255), 2);

    cv::Mat fila1, fila2, mosaico;
    cv::hconcat(rOrig, rEro, fila1);
    cv::hconcat(rDil, rBorde, fila2);
    cv::vconcat(fila1, fila2, mosaico);

    cv::imshow("Operaciones Morfologicas Basicas: Erosion y Dilatacion", mosaico);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para volver al menu.\n";
    cv::waitKey(0);
    cv::destroyWindow("Operaciones Morfologicas Basicas: Erosion y Dilatacion");
}

// ============================================================================
// 3. OPCIÓN 2: OPERACIONES COMPUESTAS (APERTURA, CIERRE, GRADIENTE, TOP-HAT)
// ============================================================================
void mostrarOperacionesCompuestas(const cv::Mat& src) {
    cv::Mat bin;
    if (src.channels() == 3) {
        cv::cvtColor(src, bin, cv::COLOR_BGR2GRAY);
    } else {
        bin = src.clone();
    }
    cv::threshold(bin, bin, 127, 255, cv::THRESH_BINARY);

    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));

    cv::Mat apertura, cierre, gradiente, tophat, blackhat;
    cv::morphologyEx(bin, apertura, cv::MORPH_OPEN, element);
    cv::morphologyEx(bin, cierre, cv::MORPH_CLOSE, element);
    cv::morphologyEx(bin, gradiente, cv::MORPH_GRADIENT, element);
    cv::morphologyEx(bin, tophat, cv::MORPH_TOPHAT, element);
    cv::morphologyEx(bin, blackhat, cv::MORPH_BLACKHAT, element);

    int w = 380, h = 260;
    cv::Mat rOrig, rApertura, rCierre, rGrad, rTop, rBlack;
    cv::resize(bin, rOrig, cv::Size(w, h));
    cv::resize(apertura, rApertura, cv::Size(w, h));
    cv::resize(cierre, rCierre, cv::Size(w, h));
    cv::resize(gradiente, rGrad, cv::Size(w, h));
    cv::resize(tophat, rTop, cv::Size(w, h));
    cv::resize(blackhat, rBlack, cv::Size(w, h));

    cv::cvtColor(rOrig, rOrig, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rApertura, rApertura, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rCierre, rCierre, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rGrad, rGrad, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rTop, rTop, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rBlack, rBlack, cv::COLOR_GRAY2BGR);

    cv::putText(rOrig, "Original (Con Ruido)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 0), 2);
    cv::putText(rApertura, "Apertura (Quita ruido externo)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 220, 255), 2);
    cv::putText(rCierre, "Cierre (Tapa huecos internos)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 120, 0), 2);
    cv::putText(rGrad, "Gradiente Morfologico", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 0, 255), 2);
    cv::putText(rTop, "Top-Hat (Elementos claros)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 255, 0), 2);
    cv::putText(rBlack, "Black-Hat (Elementos oscuros)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(180, 180, 255), 2);

    cv::Mat fila1, fila2, mosaico;
    cv::hconcat(std::vector<cv::Mat>{rOrig, rApertura, rCierre}, fila1);
    cv::hconcat(std::vector<cv::Mat>{rGrad, rTop, rBlack}, fila2);
    cv::vconcat(fila1, fila2, mosaico);

    cv::imshow("Operaciones Morfologicas Compuestas", mosaico);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para volver al menu.\n";
    cv::waitKey(0);
    cv::destroyWindow("Operaciones Morfologicas Compuestas");
}

// ============================================================================
// 4. OPCIÓN 3: COMPARATIVA DE ELEMENTOS ESTRUCTURANTES (SHAPES)
// ============================================================================
void mostrarComparativaElementos(const cv::Mat& src) {
    cv::Mat bin;
    if (src.channels() == 3) {
        cv::cvtColor(src, bin, cv::COLOR_BGR2GRAY);
    } else {
        bin = src.clone();
    }
    cv::threshold(bin, bin, 127, 255, cv::THRESH_BINARY);

    int sz = 11;
    cv::Mat elemRect = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(sz, sz));
    cv::Mat elemCross = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(sz, sz));
    cv::Mat elemEllipse = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(sz, sz));

    cv::Mat dilRect, dilCross, dilEllipse;
    cv::dilate(bin, dilRect, elemRect);
    cv::dilate(bin, dilCross, elemCross);
    cv::dilate(bin, dilEllipse, elemEllipse);

    int w = 380, h = 280;
    cv::Mat rOrig, rR, rC, rE;
    cv::resize(bin, rOrig, cv::Size(w, h));
    cv::resize(dilRect, rR, cv::Size(w, h));
    cv::resize(dilCross, rC, cv::Size(w, h));
    cv::resize(dilEllipse, rE, cv::Size(w, h));

    cv::cvtColor(rOrig, rOrig, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rR, rR, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rC, rC, cv::COLOR_GRAY2BGR);
    cv::cvtColor(rE, rE, cv::COLOR_GRAY2BGR);

    cv::putText(rOrig, "Original", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 255, 0), 2);
    cv::putText(rR, "Dilatacion (Kernel Rectangulo)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 200, 255), 2);
    cv::putText(rC, "Dilatacion (Kernel Cruz)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(255, 100, 0), 2);
    cv::putText(rE, "Dilatacion (Kernel Elipse)", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(255, 0, 255), 2);

    cv::Mat fila1, fila2, mosaico;
    cv::hconcat(rOrig, rR, fila1);
    cv::hconcat(rC, rE, fila2);
    cv::vconcat(fila1, fila2, mosaico);

    cv::imshow("Impacto de la Forma del Elemento Estructurante (11x11)", mosaico);
    std::cout << "[+] Ventana abierta. Presiona cualquier tecla para continuar.\n";
    cv::waitKey(0);
    cv::destroyWindow("Impacto de la Forma del Elemento Estructurante (11x11)");
}

// ============================================================================
// 5. OPCIÓN 4: LABORATORIO INTERACTIVO CON TRACKBARS
// ============================================================================
struct MorfLabData {
    cv::Mat binSrc;
    int operacion = 0;   // 0: Erode, 1: Dilate, 2: Open, 3: Close, 4: Gradient, 5: TopHat, 6: BlackHat
    int elemento = 0;    // 0: Rect, 1: Cross, 2: Ellipse
    int ksize = 5;       // 1 a 25 px
    int iteraciones = 1; // 1 a 10
};

static void onMorfTrackbar(int, void* userdata) {
    MorfLabData* data = static_cast<MorfLabData*>(userdata);

    int k = data->ksize;
    if (k % 2 == 0) k += 1;
    if (k < 1) k = 1;

    int iter = std::max(1, data->iteraciones);

    int morphShape = cv::MORPH_RECT;
    std::string nombreShape = "Rectangulo";
    if (data->elemento == 1) {
        morphShape = cv::MORPH_CROSS;
        nombreShape = "Cruz";
    } else if (data->elemento == 2) {
        morphShape = cv::MORPH_ELLIPSE;
        nombreShape = "Elipse";
    }

    cv::Mat kernel = cv::getStructuringElement(morphShape, cv::Size(k, k));
    cv::Mat dst;
    std::string nombreOp = "";

    switch (data->operacion) {
        case 0:
            cv::erode(data->binSrc, dst, kernel, cv::Point(-1, -1), iter);
            nombreOp = "Erosion";
            break;
        case 1:
            cv::dilate(data->binSrc, dst, kernel, cv::Point(-1, -1), iter);
            nombreOp = "Dilatacion";
            break;
        case 2:
            cv::morphologyEx(data->binSrc, dst, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), iter);
            nombreOp = "Apertura (Open)";
            break;
        case 3:
            cv::morphologyEx(data->binSrc, dst, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), iter);
            nombreOp = "Cierre (Close)";
            break;
        case 4:
            cv::morphologyEx(data->binSrc, dst, cv::MORPH_GRADIENT, kernel, cv::Point(-1, -1), iter);
            nombreOp = "Gradiente Morfologico";
            break;
        case 5:
            cv::morphologyEx(data->binSrc, dst, cv::MORPH_TOPHAT, kernel, cv::Point(-1, -1), iter);
            nombreOp = "Top-Hat";
            break;
        case 6:
            cv::morphologyEx(data->binSrc, dst, cv::MORPH_BLACKHAT, kernel, cv::Point(-1, -1), iter);
            nombreOp = "Black-Hat";
            break;
        default:
            dst = data->binSrc.clone();
            nombreOp = "Original";
            break;
    }

    cv::Mat visual;
    cv::cvtColor(dst, visual, cv::COLOR_GRAY2BGR);

    // Banner informativo superior
    cv::rectangle(visual, cv::Rect(0, 0, visual.cols, 42), cv::Scalar(20, 20, 20), -1);
    std::string infoStr = nombreOp + " | Elemento: " + nombreShape + " (" + std::to_string(k) + "x" +
                          std::to_string(k) + ") | Iters: " + std::to_string(iter);
    cv::putText(visual, infoStr, cv::Point(15, 28), cv::FONT_HERSHEY_SIMPLEX, 0.58, cv::Scalar(0, 255, 200), 2);

    cv::imshow("Laboratorio Interactivo de Morfologia", visual);
}

void ejecutarLaboratorioMorfologia(const cv::Mat& src) {
    MorfLabData data;
    cv::Mat bin;
    if (src.channels() == 3) {
        cv::cvtColor(src, bin, cv::COLOR_BGR2GRAY);
    } else {
        bin = src.clone();
    }
    cv::threshold(bin, data.binSrc, 127, 255, cv::THRESH_BINARY);

    double escala = 640.0 / data.binSrc.cols;
    cv::resize(data.binSrc, data.binSrc, cv::Size(), escala, escala);

    const std::string winName = "Laboratorio Interactivo de Morfologia";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    cv::createTrackbar("Op [0:Ero,1:Dil,2:Op,3:Cl,4:Gr,5:TH,6:BH]", winName, &data.operacion, 6, onMorfTrackbar, &data);
    cv::createTrackbar("Shape [0:Rect, 1:Cruz, 2:Elipse]", winName, &data.elemento, 2, onMorfTrackbar, &data);
    cv::createTrackbar("Kernel (1-25)", winName, &data.ksize, 25, onMorfTrackbar, &data);
    cv::createTrackbar("Iteraciones (1-10)", winName, &data.iteraciones, 10, onMorfTrackbar, &data);

    std::cout << "\n--- LABORATORIO DE MORFOLOGIA ACTIVADO ---\n";
    std::cout << "Ajusta la operacion, kernel y repeticiones interactivamente.\n";
    std::cout << "Presiona 'ESC' o 'q' para salir al menu.\n";

    onMorfTrackbar(0, &data);

    while (true) {
        char key = static_cast<char>(cv::waitKey(20));
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }

    cv::destroyWindow(winName);
}

// ============================================================================
// 6. OPCIÓN 5: MORFOLOGÍA CON CÁMARA EN TIEMPO REAL
// ============================================================================
void procesarCamaraMorfologia() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "\n[ERROR] No se pudo abrir la camara web.\n";
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    std::cout << "\n[+] Camara iniciada.\n";
    std::cout << "Controles en vivo (teclas):\n";
    std::cout << " [1] Binaria Base   [2] Erosion   [3] Dilatacion\n";
    std::cout << " [4] Apertura       [5] Cierre    [6] Gradiente\n";
    std::cout << " Presiona 'ESC' o 'q' para volver al menu.\n";

    const std::string winName = "Camara Morfologia: Original | Binaria | Morfologia";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    int modo = 2; // Iniciar en Erosión
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat gray, bin, morf;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        // Umbralización de Otsu en tiempo real
        cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        std::string tag = "";
        switch (modo) {
            case 1:
                morf = bin.clone();
                tag = "Sin Morfologia";
                break;
            case 2:
                cv::erode(bin, morf, kernel);
                tag = "Erosion (7x7)";
                break;
            case 3:
                cv::dilate(bin, morf, kernel);
                tag = "Dilatacion (7x7)";
                break;
            case 4:
                cv::morphologyEx(bin, morf, cv::MORPH_OPEN, kernel);
                tag = "Apertura (Limpia ruido)";
                break;
            case 5:
                cv::morphologyEx(bin, morf, cv::MORPH_CLOSE, kernel);
                tag = "Cierre (Tapa agujeros)";
                break;
            case 6:
                cv::morphologyEx(bin, morf, cv::MORPH_GRADIENT, kernel);
                tag = "Gradiente (Bordes)";
                break;
            default:
                morf = bin.clone();
                tag = "Binaria";
                break;
        }

        int w = 360, h = 270;
        cv::Mat rOrig, rBin, rMorf;
        cv::resize(frame, rOrig, cv::Size(w, h));
        cv::resize(bin, rBin, cv::Size(w, h));
        cv::resize(morf, rMorf, cv::Size(w, h));

        cv::cvtColor(rBin, rBin, cv::COLOR_GRAY2BGR);
        cv::cvtColor(rMorf, rMorf, cv::COLOR_GRAY2BGR);

        cv::putText(rOrig, "Original Color", cv::Point(12, 25), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 0), 2);
        cv::putText(rBin, "Binaria (Otsu)", cv::Point(12, 25), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 180, 255), 2);
        cv::putText(rMorf, tag, cv::Point(12, 25), cv::FONT_HERSHEY_SIMPLEX, 0.60, cv::Scalar(0, 255, 255), 2);

        cv::Mat visor;
        cv::hconcat(std::vector<cv::Mat>{rOrig, rBin, rMorf}, visor);

        cv::imshow(winName, visor);

        int key = cv::waitKey(15);
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        } else if (key >= '1' && key <= '6') {
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
        std::cout << "\n======================================================\n";
        std::cout << "  VISION ARTIFICIAL: MORFOLOGIA MATEMATICA            \n";
        std::cout << "======================================================\n";
        std::cout << "Imagen detectada: " << rutaImg << "\n\n";
        std::cout << "1. Operaciones Basicas (Erosion y Dilatacion)\n";
        std::cout << "2. Operaciones Compuestas (Apertura, Cierre, Gradiente, Top-Hat)\n";
        std::cout << "3. Comparativa de Elementos Estructurantes (Rect, Cruz, Elipse)\n";
        std::cout << "4. Laboratorio Interactivo con Trackbars (Sliders en vivo)\n";
        std::cout << "5. Morfologia Matematica con Camara en Vivo\n";
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
                    mostrarErosionDilatacion(src);
                }
                break;
            case 2:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarOperacionesCompuestas(src);
                }
                break;
            case 3:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    mostrarComparativaElementos(src);
                }
                break;
            case 4:
                if (src.empty()) {
                    std::cerr << "[!] Error cargando imagen: " << rutaImg << "\n";
                } else {
                    ejecutarLaboratorioMorfologia(src);
                }
                break;
            case 5:
                procesarCamaraMorfologia();
                break;
            case 0:
                std::cout << "\nSaliendo del taller de Morfologia...\n";
                break;
            default:
                std::cout << "\n[!] Opcion no valida.\n";
                break;
        }
    }

    return 0;
}
