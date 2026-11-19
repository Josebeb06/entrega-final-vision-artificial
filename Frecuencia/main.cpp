#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::string resolverRutaImagen(const std::string& preferida = "lenna.jpg") {
    std::vector<std::string> candidatos = {
        preferida,
        "../" + preferida,
        "../../" + preferida,
        "../../../" + preferida,
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
// 1. CÁLCULO DEL ESPECTRO DE FRECUENCIA (DFT 2D)
// ============================================================================
cv::Mat calcularEspectroFrecuencia(const cv::Mat& entrada) {
    // 1. Asegurar que la imagen esté en escala de grises
    cv::Mat gray;
    if (entrada.channels() == 3) {
        cv::cvtColor(entrada, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = entrada.clone();
    }

    // 2. Expandir la imagen a un tamaño óptimo para acelerar la FFT (potencias de 2, 3 o 5)
    int m = cv::getOptimalDFTSize(gray.rows);
    int n = cv::getOptimalDFTSize(gray.cols);
    cv::Mat padded;
    cv::copyMakeBorder(gray, padded, 0, m - gray.rows, 0, n - gray.cols, 
                       cv::BORDER_CONSTANT, cv::Scalar::all(0));

    // 3. Crear matriz compleja con parte real (imagen padded) e imaginaria (ceros)
    cv::Mat planos[] = { cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F) };
    cv::Mat imagenCompleja;
    cv::merge(planos, 2, imagenCompleja);

    // 4. Aplicar la Transformada Discreta de Fourier (DFT)
    cv::dft(imagenCompleja, imagenCompleja);

    // 5. Calcular la magnitud: sqrt(Re^2 + Im^2)
    cv::split(imagenCompleja, planos);
    cv::Mat magnitud;
    cv::magnitude(planos[0], planos[1], magnitud);

    // 6. Escala logarítmica: M = log(1 + M) para visualizar el rango dinámico
    magnitud += cv::Scalar::all(1);
    cv::log(magnitud, magnitud);

    // 7. Recortar el espectro si el padding añadió filas o columnas impares
    magnitud = magnitud(cv::Rect(0, 0, magnitud.cols & -2, magnitud.rows & -2));

    // 8. Reordenar cuadrantes (fftshift): trasladar la frecuencia cero (DC) al centro
    int cx = magnitud.cols / 2;
    int cy = magnitud.rows / 2;

    cv::Mat q0(magnitud, cv::Rect(0, 0, cx, cy));   // Superior izquierdo
    cv::Mat q1(magnitud, cv::Rect(cx, 0, cx, cy));  // Superior derecho
    cv::Mat q2(magnitud, cv::Rect(0, cy, cx, cy));  // Inferior izquierdo
    cv::Mat q3(magnitud, cv::Rect(cx, cy, cx, cy)); // Inferior derecho

    cv::Mat tmp;
    // Intercambiar cuadrante 0 con 3
    q0.copyTo(tmp);
    q3.copyTo(q0);
    tmp.copyTo(q3);

    // Intercambiar cuadrante 1 con 2
    q1.copyTo(tmp);
    q2.copyTo(q1);
    tmp.copyTo(q2);

    // 9. Normalizar el espectro al rango visual [0, 1] en punto flotante
    cv::normalize(magnitud, magnitud, 0, 1, cv::NORM_MINMAX);

    return magnitud;
}

// ============================================================================
// 2. OPCIÓN 1: ANÁLISIS DE LA IMAGEN DE LENNA
// ============================================================================
void procesarImagenLenna(const std::string& rutaImagen) {
    cv::Mat img = cv::imread(rutaImagen, cv::IMREAD_GRAYSCALE);
    if (img.empty()) {
        std::cerr << "\n[ERROR] No se pudo encontrar o abrir '" << rutaImagen 
                  << "'. Asegurate de tenerla en la carpeta raiz.\n" << std::endl;
        return;
    }

    std::cout << "\n[+] Calculando Transformada de Fourier para '" << rutaImagen << "'..." << std::endl;
    cv::Mat espectro = calcularEspectroFrecuencia(img);

    // Convertir para visualización conjunta
    cv::Mat imgNorm, espectroU8;
    img.convertTo(imgNorm, CV_32F, 1.0 / 255.0);

    // Redimensionar el espectro para que coincida exactamente con la imagen original
    cv::resize(espectro, espectro, imgNorm.size());

    cv::Mat comparativa;
    cv::hconcat(imgNorm, espectro, comparativa);

    cv::namedWindow("Espectro de Frecuencia - Lenna (Dominio Espacial vs Frecuencia)", cv::WINDOW_AUTOSIZE);
    cv::imshow("Espectro de Frecuencia - Lenna (Dominio Espacial vs Frecuencia)", comparativa);

    std::cout << "[+] Presiona cualquier tecla en la ventana de la imagen para volver al menu.\n";
    cv::waitKey(0);
    cv::destroyAllWindows();
}

// ============================================================================
// 3. OPCIÓN 2: ANÁLISIS EN TIEMPO REAL CON LA CÁMARA
// ============================================================================
void procesarCamaraTiempoReal() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "\n[ERROR] No se pudo acceder a la camara web.\n" << std::endl;
        return;
    }

    // Configuración de resolución para procesamiento ágil de la FFT
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    std::cout << "\n[+] Camara iniciada." << std::endl;
    std::cout << "[+] Presiona 'ESC' o 'q' para detener la captura y volver al menu.\n" << std::endl;

    std::string nombreVentana = "Frecuencia en Tiempo Real (Original | Espectro FFT)";
    cv::namedWindow(nombreVentana, cv::WINDOW_AUTOSIZE);

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Calcular espectro de frecuencias del cuadro actual
        cv::Mat espectro = calcularEspectroFrecuencia(gray);

        // Convertir frame original a float [0, 1] y ajustar tamaño para concatenar
        cv::Mat grayFloat;
        gray.convertTo(grayFloat, CV_32F, 1.0 / 255.0);
        cv::resize(espectro, espectro, grayFloat.size());

        cv::Mat visorLadoALado;
        cv::hconcat(grayFloat, espectro, visorLadoALado);

        cv::imshow(nombreVentana, visorLadoALado);

        char key = static_cast<char>(cv::waitKey(15));
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
}

// ============================================================================
// 4. MENÚ PRINCIPAL
// ============================================================================
int main() {
    const std::string rutaLenna = resolverRutaImagen("lenna.jpg");
    int opcion = -1;

    while (opcion != 0) {
        std::cout << "\n============================================\n";
        std::cout << "  VISION ARTIFICIAL: ANALISIS DE FRECUENCIA \n";
        std::cout << "============================================\n";
        std::cout << "1. Ver frecuencia de la imagen de Lena (" << rutaLenna << ")\n";
        std::cout << "2. Ver frecuencia con la camara en tiempo real\n";
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
                procesarImagenLenna(rutaLenna);
                break;
            case 2:
                procesarCamaraTiempoReal();
                break;
            case 0:
                std::cout << "\nCerrando aplicacion...\n";
                break;
            default:
                std::cout << "\n[!] Opcion no valida. Intenta de nuevo.\n";
                break;
        }
    }

    return 0;
}