#include <opencv2/opencv.hpp>
#include <vector>
#include <deque>
#include <iostream>
#include <cmath>

struct MitadFruta {
    cv::Mat sprite;
    cv::Point2f pos;
    cv::Point2f vel;
    float angulo = 0.0f;
    float velGiro = 0.0f;
};

struct Fruta {
    cv::Mat sprite;
    cv::Mat mascara;
    cv::Point2f pos;
    cv::Point2f vel;
    float angulo = 0.0f;
    float velGiro = 4.0f;
    int radio = 35;
    bool cortada = false;
};

// Generador de sprite de sandía
void generarSpriteFruta(cv::Mat& sprite, cv::Mat& mascara, int radio) {
    int tam = radio * 2 + 10;
    sprite = cv::Mat::zeros(tam, tam, CV_8UC3);
    mascara = cv::Mat::zeros(tam, tam, CV_8UC1);
    cv::Point centro(tam / 2, tam / 2);

    cv::circle(sprite, centro, radio, cv::Scalar(40, 180, 50), -1);
    cv::circle(sprite, centro, radio - 6, cv::Scalar(50, 50, 220), -1);
    cv::circle(sprite, centro + cv::Point(-8, -6), 2, cv::Scalar(10, 10, 10), -1);
    cv::circle(sprite, centro + cv::Point(8, -6), 2, cv::Scalar(10, 10, 10), -1);
    cv::circle(sprite, centro + cv::Point(0, 8), 2, cv::Scalar(10, 10, 10), -1);
    cv::circle(mascara, centro, radio, cv::Scalar(255), -1);
}

// Transformación afín de 2x3 para rotación
cv::Mat rotarSpriteAfin(const cv::Mat& src, double angulo) {
    cv::Point2f centro(src.cols / 2.0f, src.rows / 2.0f);
    cv::Mat matrizRot = cv::getRotationMatrix2D(centro, angulo, 1.0);
    cv::Mat dst;
    cv::warpAffine(src, dst, matrizRot, src.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return dst;
}

// Dibujo de sprites con ROI seguro
void dibujarSprite(cv::Mat& fondo, const cv::Mat& sprite, const cv::Mat& mask, cv::Point2f pos) {
    int x0 = static_cast<int>(pos.x - sprite.cols / 2.0f);
    int y0 = static_cast<int>(pos.y - sprite.rows / 2.0f);

    cv::Rect roiFondo(x0, y0, sprite.cols, sprite.rows);
    cv::Rect pantalla(0, 0, fondo.cols, fondo.rows);
    cv::Rect interseccion = roiFondo & pantalla;

    if (interseccion.width <= 0 || interseccion.height <= 0) return;

    cv::Rect roiSprite(interseccion.x - x0, interseccion.y - y0, interseccion.width, interseccion.height);
    cv::Mat subFondo = fondo(interseccion);
    cv::Mat subSprite = sprite(roiSprite);
    cv::Mat subMask = mask(roiSprite);

    subSprite.copyTo(subFondo, subMask);
}

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: No se pudo acceder a la camara." << std::endl;
        return -1;
    }

    // Calibración YCrCb para piel
    int minCr = 133, maxCr = 173;
    int minCb = 77,  maxCb = 127;

    cv::namedWindow("Calibracion Piel", cv::WINDOW_AUTOSIZE);
    cv::createTrackbar("Cr Min", "Calibracion Piel", &minCr, 255);
    cv::createTrackbar("Cr Max", "Calibracion Piel", &maxCr, 255);
    cv::createTrackbar("Cb Min", "Calibracion Piel", &minCb, 255);
    cv::createTrackbar("Cb Max", "Calibracion Piel", &maxCb, 255);

    cv::Mat baseFruta, baseMascara;
    int radioFruta = 35;
    generarSpriteFruta(baseFruta, baseMascara, radioFruta);

    int ancho = baseFruta.cols;
    int alto = baseFruta.rows;
    cv::Mat mitadIzq = cv::Mat::zeros(alto, ancho, CV_8UC3);
    cv::Mat mitadDer = cv::Mat::zeros(alto, ancho, CV_8UC3);
    cv::Mat maskIzq = cv::Mat::zeros(alto, ancho, CV_8UC1);
    cv::Mat maskDer = cv::Mat::zeros(alto, ancho, CV_8UC1);

    baseFruta(cv::Rect(0, 0, ancho / 2, alto)).copyTo(mitadIzq(cv::Rect(0, 0, ancho / 2, alto)));
    baseMascara(cv::Rect(0, 0, ancho / 2, alto)).copyTo(maskIzq(cv::Rect(0, 0, ancho / 2, alto)));
    baseFruta(cv::Rect(ancho / 2, 0, ancho / 2, alto)).copyTo(mitadDer(cv::Rect(ancho / 2, 0, ancho / 2, alto)));
    baseMascara(cv::Rect(ancho / 2, 0, ancho / 2, alto)).copyTo(maskDer(cv::Rect(ancho / 2, 0, ancho / 2, alto)));

    std::deque<cv::Point> rastroCorte;
    std::vector<Fruta> frutas;
    std::vector<MitadFruta> mitades;
    int puntaje = 0;

    // Elemento estructurante para operaciones morfológicas
    cv::Mat kernelMorf = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;
        cv::flip(frame, frame, 1); // Modo espejo

        // 1. Segmentación y filtrado morfológico estricto
        cv::Mat ycrcb, mascaraPiel;
        cv::cvtColor(frame, ycrcb, cv::COLOR_BGR2YCrCb);
        cv::inRange(ycrcb, cv::Scalar(0, minCr, minCb), cv::Scalar(255, maxCr, maxCb), mascaraPiel);

        // Operaciones de Apertura (elimina ruido pequeño) y Cierre (rellena huecos)
        cv::morphologyEx(mascaraPiel, mascaraPiel, cv::MORPH_OPEN, kernelMorf);
        cv::morphologyEx(mascaraPiel, mascaraPiel, cv::MORPH_CLOSE, kernelMorf);
        cv::GaussianBlur(mascaraPiel, mascaraPiel, cv::Size(5, 5), 1.5);

        // 2. Extracción y Filtrado Geométrico de la Mano
        std::vector<std::vector<cv::Point>> contornos;
        cv::findContours(mascaraPiel, contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        cv::Point puntaIndice(-1, -1);
        cv::Point centroPalma(-1, -1);

        for (const auto& contorno : contornos) {
            double area = cv::contourArea(contorno);
            if (area < 4500 || area > 85000) continue; // Descarta parches diminutos y fondos gigantes

            cv::Rect bBox = cv::boundingRect(contorno);
            
            // FILTRO DE CARA Y FONDO:
            // Si el objeto está en el tercio superior central de la pantalla y es casi cuadrado, es el rostro
            bool esCentroRostro = (bBox.y < frame.rows * 0.45) && 
                                  (bBox.x > frame.cols * 0.25 && (bBox.x + bBox.width) < frame.cols * 0.75);
            float aspectRatio = static_cast<float>(bBox.width) / bBox.height;

            // La mano levantada suele ser más alta que ancha o provenir de los costados/abajo
            if (esCentroRostro && (aspectRatio > 0.75f && aspectRatio < 1.35f)) {
                continue; // Ignorar el rostro
            }

            // Calcular centroide (Palma)
            cv::Moments m = cv::moments(contorno);
            if (m.m00 == 0) continue;
            centroPalma = cv::Point(static_cast<int>(m.m10 / m.m00), static_cast<int>(m.m01 / m.m00));

            // Obtener el punto más alto del contorno (Punta del dedo índice)
            cv::Point masAlto = contorno[0];
            for (const auto& pt : contorno) {
                if (pt.y < masAlto.y) masAlto = pt;
            }

            // Validar que el dedo esté claramente extendido por encima de la palma
            if ((centroPalma.y - masAlto.y) > 40) {
                puntaIndice = masAlto;

                // 3. RENDERIZADO DEL ESQUELETO DEL DEDO ÍNDICE (Interpolación Vectorial Afín)
                cv::Point2f vecDedo = cv::Point2f(puntaIndice - centroPalma);
                
                cv::Point pNudillo   = centroPalma + cv::Point(vecDedo * 0.45f);
                cv::Point pIntermedia = centroPalma + cv::Point(vecDedo * 0.75f);

                // Hueso metacarpiano y falanges
                cv::line(frame, centroPalma, pNudillo, cv::Scalar(0, 255, 0), 4);
                cv::line(frame, pNudillo, pIntermedia, cv::Scalar(0, 255, 255), 3);
                cv::line(frame, pIntermedia, puntaIndice, cv::Scalar(0, 165, 255), 3);

                // Articulaciones (Nodos óseos)
                cv::circle(frame, centroPalma, 10, cv::Scalar(255, 0, 255), -1);
                cv::circle(frame, pNudillo, 6, cv::Scalar(0, 255, 0), -1);
                cv::circle(frame, pIntermedia, 6, cv::Scalar(0, 255, 255), -1);
                
                // Yema del dedo (Efecto filo de corte)
                cv::circle(frame, puntaIndice, 12, cv::Scalar(0, 0, 255), 2);
                cv::circle(frame, puntaIndice, 6, cv::Scalar(255, 255, 255), -1);

                break; // Mano procesada con éxito
            }
        }

        // 4. Gestión del rastro de corte
        if (puntaIndice.x >= 0) {
            rastroCorte.push_back(puntaIndice);
            if (rastroCorte.size() > 10) rastroCorte.pop_front();
        } else {
            rastroCorte.clear();
        }

        // 5. Generación de frutas
        if (rand() % 35 == 0) {
            Fruta f;
            f.sprite = baseFruta.clone();
            f.mascara = baseMascara.clone();
            f.pos = cv::Point2f(rand() % (frame.cols - 140) + 70, frame.rows);
            f.vel = cv::Point2f((rand() % 6) - 3.0f, -(rand() % 4 + 13));
            f.velGiro = (rand() % 10 - 5.0f);
            if (std::abs(f.velGiro) < 1.0f) f.velGiro = 3.0f;
            frutas.push_back(f);
        }

        // 6. Actualización física y colisiones
        for (size_t i = 0; i < frutas.size();) {
            frutas[i].pos += frutas[i].vel;
            frutas[i].vel.y += 0.38f;
            frutas[i].angulo += frutas[i].velGiro;

            // Detección de colisión con la yema del dedo
            if (!frutas[i].cortada && !rastroCorte.empty()) {
                double dist = cv::norm(frutas[i].pos - cv::Point2f(rastroCorte.back().x, rastroCorte.back().y));
                if (dist < frutas[i].radio + 15) {
                    frutas[i].cortada = true;
                    puntaje += 10;

                    MitadFruta m1{ mitadIzq.clone(), frutas[i].pos, cv::Point2f(-4.5f, -3.0f), frutas[i].angulo, -9.0f };
                    MitadFruta m2{ mitadDer.clone(), frutas[i].pos, cv::Point2f(4.5f, -3.0f), frutas[i].angulo, 9.0f };
                    mitades.push_back(m1);
                    mitades.push_back(m2);
                }
            }

            if (!frutas[i].cortada) {
                cv::Mat spriteRotado = rotarSpriteAfin(frutas[i].sprite, frutas[i].angulo);
                cv::Mat mascaraRotada = rotarSpriteAfin(frutas[i].mascara, frutas[i].angulo);
                dibujarSprite(frame, spriteRotado, mascaraRotada, frutas[i].pos);
            }

            if (frutas[i].cortada || frutas[i].pos.y > frame.rows + 80) {
                frutas.erase(frutas.begin() + i);
            } else {
                ++i;
            }
        }

        // 7. Animación de mitades cortadas
        for (size_t i = 0; i < mitades.size();) {
            mitades[i].pos += mitades[i].vel;
            mitades[i].vel.y += 0.45f;
            mitades[i].angulo += mitades[i].velGiro;

            cv::Mat spriteRotado = rotarSpriteAfin(mitades[i].sprite, mitades[i].angulo);
            cv::Mat mascaraRotada;
            cv::cvtColor(spriteRotado, mascaraRotada, cv::COLOR_BGR2GRAY);
            cv::threshold(mascaraRotada, mascaraRotada, 1, 255, cv::THRESH_BINARY);

            dibujarSprite(frame, spriteRotado, mascaraRotada, mitades[i].pos);

            if (mitades[i].pos.y > frame.rows + 80) {
                mitades.erase(mitades.begin() + i);
            } else {
                ++i;
            }
        }

        // 8. Línea de corte de la espada
        for (size_t i = 1; i < rastroCorte.size(); i++) {
            cv::line(frame, rastroCorte[i - 1], rastroCorte[i], cv::Scalar(0, 255, 255), 4);
        }

        // Marcador
        cv::putText(frame, "Puntos: " + std::to_string(puntaje), cv::Point(25, 45),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

        cv::imshow("Fruit Ninja - Vision Artificial", frame);
        cv::imshow("Calibracion Piel", mascaraPiel);

        if (cv::waitKey(16) == 27) break;
    }
    return 0;
}