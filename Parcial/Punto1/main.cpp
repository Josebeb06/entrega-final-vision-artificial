#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "======================================================\n";
    std::cout << "  PARCIAL SEGUNDO CORTE - PUNTO 1: CONVOLUCION (CANAL R)\n";
    std::cout << "======================================================\n\n";

    // 1. Matriz de entrada 6x6 dada en el enunciado (Canal R)
    cv::Mat_<float> imgEntrada(6, 6);
    imgEntrada << 10, 25, 45, 60, 32, 20,
                  55, 95, 35, 66, 98, 10,
                  18, 22, 37, 48, 21, 87,
                  26, 87, 38, 49, 76, 26,
                  36, 28, 29, 44, 25, 12,
                  45, 37, 28, 99, 31, 72;

    // 2. Filtro de convolución (Kernel 3x3) para detección de bordes
    cv::Mat_<float> kernel(3, 3);
    kernel <<  0, -1,  0,
              -1,  4, -1,
               0, -1,  0;

    // 3. Aplicar la convolución
    cv::Mat resultado;
    cv::filter2D(imgEntrada, resultado, CV_32F, kernel);

    std::cout << "--- MATRIZ DE ENTRADA (6x6) ---\n";
    std::cout << imgEntrada << "\n\n";

    std::cout << "--- KERNEL DE CONVOLUCION (3x3) ---\n";
    std::cout << kernel << "\n\n";

    std::cout << "--- MATRIZ RESULTADO DE LA CONVOLUCION (Bordes) ---\n";
    for(int r = 0; r < resultado.rows; r++) {
        for(int c = 0; c < resultado.cols; c++) {
            std::cout << std::setw(6) << resultado.at<float>(r, c) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
    
    return 0;
}