#include <opencv2/opencv.hpp>
#include <vector>
#include <deque>
#include <iostream>
#include <cmath>
#include <string>
#include <algorithm>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

// ============================================================================
// 1. LOCALIZADOR AUTOMÁTICO DE RECURSOS (PORTABLE)
// ============================================================================
std::string resolverArchivoCascade(const std::string& nombre = "haarcascade_frontalface_default.xml") {
    std::vector<std::string> candidatos = {
        nombre,
        "../" + nombre,
        "../../" + nombre,
        "assets/" + nombre,
        "../assets/" + nombre,
        "../../assets/" + nombre,
        "C:/opencv/build/etc/haarcascades/" + nombre,
        "C:/opencv/sources/data/haarcascades/" + nombre
    };

    for (const auto& ruta : candidatos) {
        if (fs::exists(ruta)) {
            return ruta;
        }
    }
    return nombre;
}

// ============================================================================
// 2. ESTRUCTURAS DEL JUEGO: FRUTAS, MITADES Y PARTÍCULAS
// ============================================================================
enum TipoFruta {
    FRUTA_SANDIA = 0,
    FRUTA_NARANJA = 1,
    FRUTA_MANZANA = 2,
    FRUTA_PLATANO = 3,
    OBJETO_BOMBA = 4
};

enum EstadoJuego {
    ESTADO_MENU = 0,
    ESTADO_JUGANDO = 1,
    ESTADO_GAMEOVER = 2
};

struct ParticulaJugo {
    cv::Point2f pos;
    cv::Point2f vel;
    cv::Scalar color;
    float radio;
    int vida;
    int vidaMaxima;
};

struct TextoFlotante {
    std::string texto;
    cv::Point2f pos;
    cv::Scalar color;
    float escala;
    int vida;
};

struct MitadFruta {
    cv::Mat sprite;
    cv::Mat mascara;
    cv::Point2f pos;
    cv::Point2f vel;
    float angulo = 0.0f;
    float velGiro = 0.0f;
};

struct Fruta {
    TipoFruta tipo;
    cv::Mat sprite;
    cv::Mat mascara;
    cv::Point2f pos;
    cv::Point2f vel;
    float angulo = 0.0f;
    float velGiro = 3.0f;
    int radio = 40;
    bool cortada = false;
    cv::Scalar colorJugo;
};

// ============================================================================
// 3. GENERADOR DE SPRITES VECTORIALES DE ALTA CALIDAD
// ============================================================================
void generarSpriteSandia(cv::Mat& sprite, cv::Mat& mask, cv::Mat& mitadIzq, cv::Mat& mitadDer, 
                         cv::Mat& maskIzq, cv::Mat& maskDer, int radio = 42) {
    int tam = radio * 2 + 16;
    sprite = cv::Mat::zeros(tam, tam, CV_8UC3);
    mask   = cv::Mat::zeros(tam, tam, CV_8UC1);
    cv::Point c(tam / 2, tam / 2);

    // Corteza verde exterior con franjas oscuras
    cv::circle(sprite, c, radio, cv::Scalar(30, 140, 40), -1, cv::LINE_AA);
    for (int a = 0; a < 360; a += 45) {
        double rad = a * CV_PI / 180.0;
        cv::Point pFin(c.x + cvRound((radio - 3) * std::cos(rad)), c.y + cvRound((radio - 3) * std::sin(rad)));
        cv::line(sprite, c, pFin, cv::Scalar(20, 80, 25), 4, cv::LINE_AA);
    }
    // Capa blanca de la corteza
    cv::circle(sprite, c, radio - 5, cv::Scalar(210, 240, 210), -1, cv::LINE_AA);
    // Pulpa roja jugosa
    cv::circle(sprite, c, radio - 8, cv::Scalar(40, 40, 235), -1, cv::LINE_AA);
    cv::circle(sprite, c, radio - 16, cv::Scalar(60, 60, 245), -1, cv::LINE_AA);

    // Semillas negras
    std::vector<cv::Point> semillas = {
        cv::Point(-12, -8), cv::Point(12, -8), cv::Point(0, -18),
        cv::Point(-8, 12), cv::Point(8, 12), cv::Point(0, 4)
    };
    for (const auto& s : semillas) {
        cv::circle(sprite, c + s, 3, cv::Scalar(15, 15, 15), -1, cv::LINE_AA);
        cv::circle(sprite, c + s, 1, cv::Scalar(60, 60, 60), -1, cv::LINE_AA);
    }
    cv::circle(mask, c, radio, cv::Scalar(255), -1, cv::LINE_AA);

    // Cortar en mitades
    int mitad = tam / 2;
    mitadIzq = cv::Mat::zeros(tam, tam, CV_8UC3);
    mitadDer = cv::Mat::zeros(tam, tam, CV_8UC3);
    maskIzq  = cv::Mat::zeros(tam, tam, CV_8UC1);
    maskDer  = cv::Mat::zeros(tam, tam, CV_8UC1);

    sprite(cv::Rect(0, 0, mitad, tam)).copyTo(mitadIzq(cv::Rect(0, 0, mitad, tam)));
    mask(cv::Rect(0, 0, mitad, tam)).copyTo(maskIzq(cv::Rect(0, 0, mitad, tam)));
    sprite(cv::Rect(mitad, 0, mitad, tam)).copyTo(mitadDer(cv::Rect(mitad, 0, mitad, tam)));
    mask(cv::Rect(mitad, 0, mitad, tam)).copyTo(maskDer(cv::Rect(mitad, 0, mitad, tam)));
}

void generarSpriteNaranja(cv::Mat& sprite, cv::Mat& mask, cv::Mat& mitadIzq, cv::Mat& mitadDer, 
                          cv::Mat& maskIzq, cv::Mat& maskDer, int radio = 38) {
    int tam = radio * 2 + 16;
    sprite = cv::Mat::zeros(tam, tam, CV_8UC3);
    mask   = cv::Mat::zeros(tam, tam, CV_8UC1);
    cv::Point c(tam / 2, tam / 2);

    // Cáscara naranja intensa
    cv::circle(sprite, c, radio, cv::Scalar(0, 130, 245), -1, cv::LINE_AA);
    cv::circle(sprite, c, radio - 4, cv::Scalar(210, 240, 255), -1, cv::LINE_AA); // Albedo blanco
    cv::circle(sprite, c, radio - 7, cv::Scalar(10, 160, 255), -1, cv::LINE_AA); // Pulpa

    // 8 Gajos triangulares de naranja
    for (int i = 0; i < 8; i++) {
        double a1 = (i * 45 + 5) * CV_PI / 180.0;
        double a2 = (i * 45 + 40) * CV_PI / 180.0;
        double rGajo = radio - 10;
        std::vector<cv::Point> ptsGajo = {
            c,
            cv::Point(c.x + cvRound(rGajo * std::cos(a1)), c.y + cvRound(rGajo * std::sin(a1))),
            cv::Point(c.x + cvRound(rGajo * std::cos(a2)), c.y + cvRound(rGajo * std::sin(a2)))
        };
        cv::fillConvexPoly(sprite, ptsGajo, cv::Scalar(0, 185, 255), cv::LINE_AA);
    }
    cv::circle(sprite, c, 5, cv::Scalar(240, 250, 255), -1, cv::LINE_AA);
    cv::circle(mask, c, radio, cv::Scalar(255), -1, cv::LINE_AA);

    int mitad = tam / 2;
    mitadIzq = cv::Mat::zeros(tam, tam, CV_8UC3);
    mitadDer = cv::Mat::zeros(tam, tam, CV_8UC3);
    maskIzq  = cv::Mat::zeros(tam, tam, CV_8UC1);
    maskDer  = cv::Mat::zeros(tam, tam, CV_8UC1);

    sprite(cv::Rect(0, 0, mitad, tam)).copyTo(mitadIzq(cv::Rect(0, 0, mitad, tam)));
    mask(cv::Rect(0, 0, mitad, tam)).copyTo(maskIzq(cv::Rect(0, 0, mitad, tam)));
    sprite(cv::Rect(mitad, 0, mitad, tam)).copyTo(mitadDer(cv::Rect(mitad, 0, mitad, tam)));
    mask(cv::Rect(mitad, 0, mitad, tam)).copyTo(maskDer(cv::Rect(mitad, 0, mitad, tam)));
}

void generarSpriteManzana(cv::Mat& sprite, cv::Mat& mask, cv::Mat& mitadIzq, cv::Mat& mitadDer, 
                          cv::Mat& maskIzq, cv::Mat& maskDer, int radio = 38) {
    int tam = radio * 2 + 16;
    sprite = cv::Mat::zeros(tam, tam, CV_8UC3);
    mask   = cv::Mat::zeros(tam, tam, CV_8UC1);
    cv::Point c(tam / 2, tam / 2 + 2);

    // Forma manzana roja con ligera hendidura
    cv::circle(sprite, c + cv::Point(-10, 0), radio - 3, cv::Scalar(20, 20, 210), -1, cv::LINE_AA);
    cv::circle(sprite, c + cv::Point(10, 0), radio - 3, cv::Scalar(20, 20, 210), -1, cv::LINE_AA);
    cv::circle(sprite, c + cv::Point(0, 6), radio - 4, cv::Scalar(25, 25, 230), -1, cv::LINE_AA);

    // Reflejo brillante de luz
    cv::ellipse(sprite, c + cv::Point(-14, -12), cv::Size(9, 5), -30, 0, 360, cv::Scalar(100, 100, 255), -1, cv::LINE_AA);

    // Tallo de madera café y hoja verde arriba
    cv::line(sprite, c + cv::Point(0, -radio + 4), c + cv::Point(4, -radio - 8), cv::Scalar(30, 60, 110), 3, cv::LINE_AA);
    cv::ellipse(sprite, c + cv::Point(8, -radio - 4), cv::Size(9, 4), 35, 0, 360, cv::Scalar(40, 190, 60), -1, cv::LINE_AA);

    cv::circle(mask, c + cv::Point(-10, 0), radio - 3, cv::Scalar(255), -1, cv::LINE_AA);
    cv::circle(mask, c + cv::Point(10, 0), radio - 3, cv::Scalar(255), -1, cv::LINE_AA);
    cv::circle(mask, c + cv::Point(0, 6), radio - 4, cv::Scalar(255), -1, cv::LINE_AA);
    cv::ellipse(mask, c + cv::Point(8, -radio - 4), cv::Size(9, 4), 35, 0, 360, cv::Scalar(255), -1, cv::LINE_AA);

    int mitad = tam / 2;
    mitadIzq = cv::Mat::zeros(tam, tam, CV_8UC3);
    mitadDer = cv::Mat::zeros(tam, tam, CV_8UC3);
    maskIzq  = cv::Mat::zeros(tam, tam, CV_8UC1);
    maskDer  = cv::Mat::zeros(tam, tam, CV_8UC1);

    sprite(cv::Rect(0, 0, mitad, tam)).copyTo(mitadIzq(cv::Rect(0, 0, mitad, tam)));
    mask(cv::Rect(0, 0, mitad, tam)).copyTo(maskIzq(cv::Rect(0, 0, mitad, tam)));
    sprite(cv::Rect(mitad, 0, mitad, tam)).copyTo(mitadDer(cv::Rect(mitad, 0, mitad, tam)));
    mask(cv::Rect(mitad, 0, mitad, tam)).copyTo(maskDer(cv::Rect(mitad, 0, mitad, tam)));
}

void generarSpritePlatano(cv::Mat& sprite, cv::Mat& mask, cv::Mat& mitadIzq, cv::Mat& mitadDer, 
                          cv::Mat& maskIzq, cv::Mat& maskDer, int radio = 36) {
    int tam = radio * 2 + 16;
    sprite = cv::Mat::zeros(tam, tam, CV_8UC3);
    mask   = cv::Mat::zeros(tam, tam, CV_8UC1);
    cv::Point c(tam / 2, tam / 2);

    // Plátano curvado amarillo
    cv::ellipse(sprite, c + cv::Point(0, 6), cv::Size(34, 16), 25, 0, 360, cv::Scalar(20, 220, 245), -1, cv::LINE_AA);
    cv::ellipse(sprite, c + cv::Point(4, 2), cv::Size(30, 12), 25, 0, 360, cv::Scalar(40, 235, 255), -1, cv::LINE_AA);
    // Extremos oscuros
    cv::circle(sprite, c + cv::Point(-26, -5), 4, cv::Scalar(20, 70, 40), -1, cv::LINE_AA);
    cv::circle(sprite, c + cv::Point(28, 18), 3, cv::Scalar(20, 40, 70), -1, cv::LINE_AA);

    cv::ellipse(mask, c + cv::Point(0, 6), cv::Size(34, 16), 25, 0, 360, cv::Scalar(255), -1, cv::LINE_AA);

    int mitad = tam / 2;
    mitadIzq = cv::Mat::zeros(tam, tam, CV_8UC3);
    mitadDer = cv::Mat::zeros(tam, tam, CV_8UC3);
    maskIzq  = cv::Mat::zeros(tam, tam, CV_8UC1);
    maskDer  = cv::Mat::zeros(tam, tam, CV_8UC1);

    sprite(cv::Rect(0, 0, mitad, tam)).copyTo(mitadIzq(cv::Rect(0, 0, mitad, tam)));
    mask(cv::Rect(0, 0, mitad, tam)).copyTo(maskIzq(cv::Rect(0, 0, mitad, tam)));
    sprite(cv::Rect(mitad, 0, mitad, tam)).copyTo(mitadDer(cv::Rect(mitad, 0, mitad, tam)));
    mask(cv::Rect(mitad, 0, mitad, tam)).copyTo(maskDer(cv::Rect(mitad, 0, mitad, tam)));
}

void generarSpriteBomba(cv::Mat& sprite, cv::Mat& mask, int radio = 36) {
    int tam = radio * 2 + 20;
    sprite = cv::Mat::zeros(tam, tam, CV_8UC3);
    mask   = cv::Mat::zeros(tam, tam, CV_8UC1);
    cv::Point c(tam / 2, tam / 2 + 4);

    // Cuerpo esférico metálico negro
    cv::circle(sprite, c, radio, cv::Scalar(35, 35, 38), -1, cv::LINE_AA);
    cv::circle(sprite, c, radio - 3, cv::Scalar(48, 48, 52), -1, cv::LINE_AA);
    // Reflejo metálico
    cv::circle(sprite, c + cv::Point(-10, -10), radio / 3, cv::Scalar(110, 110, 120), -1, cv::LINE_AA);
    cv::circle(sprite, c + cv::Point(-12, -12), 4, cv::Scalar(230, 230, 240), -1, cv::LINE_AA);

    // Tapa dorada y mecha
    cv::rectangle(sprite, cv::Rect(c.x - 7, c.y - radio - 4, 14, 6), cv::Scalar(40, 160, 210), -1, cv::LINE_AA);
    // Mecha curva
    cv::line(sprite, cv::Point(c.x, c.y - radio - 4), cv::Point(c.x + 8, c.y - radio - 12), cv::Scalar(80, 140, 180), 3, cv::LINE_AA);

    cv::circle(mask, c, radio, cv::Scalar(255), -1, cv::LINE_AA);
    cv::rectangle(mask, cv::Rect(c.x - 7, c.y - radio - 4, 14, 6), cv::Scalar(255), -1, cv::LINE_AA);
    cv::line(mask, cv::Point(c.x, c.y - radio - 4), cv::Point(c.x + 8, c.y - radio - 12), cv::Scalar(255), 4, cv::LINE_AA);
}

// ============================================================================
// 4. TRANSFORMACIONES Y DIBUJO CON INTERPOLACIÓN AFÍN
// ============================================================================
cv::Mat rotarSpriteAfin(const cv::Mat& src, double angulo) {
    cv::Point2f c(src.cols / 2.0f, src.rows / 2.0f);
    cv::Mat rot = cv::getRotationMatrix2D(c, angulo, 1.0);
    cv::Mat dst;
    cv::warpAffine(src, dst, rot, src.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return dst;
}

void dibujarSpriteSeguro(cv::Mat& fondo, const cv::Mat& sprite, const cv::Mat& mask, cv::Point2f pos) {
    int x0 = static_cast<int>(pos.x - sprite.cols / 2.0f);
    int y0 = static_cast<int>(pos.y - sprite.rows / 2.0f);

    cv::Rect roiFondo(x0, y0, sprite.cols, sprite.rows);
    cv::Rect pantalla(0, 0, fondo.cols, fondo.rows);
    cv::Rect inter = roiFondo & pantalla;

    if (inter.width <= 0 || inter.height <= 0) return;

    cv::Rect roiSprite(inter.x - x0, inter.y - y0, inter.width, inter.height);
    cv::Mat subFondo  = fondo(inter);
    cv::Mat subSprite = sprite(roiSprite);
    cv::Mat subMask   = mask(roiSprite);

    subSprite.copyTo(subFondo, subMask);
}

// ============================================================================
// 5. DETECCIÓN ROBUSTA DE LA MANO Y YEMA DEL DEDO ÍNDICE
// ============================================================================
class DetectorManoNinja {
public:
    cv::CascadeClassifier faceCascade;
    bool cascadeCargada = false;
    cv::Point2f dedoSuavizado = cv::Point2f(-1.0f, -1.0f);
    cv::Point centroPalmaSuavizado = cv::Point(-1, -1);
    float radioPalmaSuavizado = 0.0f;
    bool modoColorEspada = false; // Tecla [M] para alternar

    DetectorManoNinja() {
        std::string rutaXml = resolverArchivoCascade("haarcascade_frontalface_default.xml");
        if (faceCascade.load(rutaXml)) {
            cascadeCargada = true;
            std::cout << "[+] Clasificador de rostros cargado: " << rutaXml << "\n";
        } else {
            std::cerr << "[!] Advertencia: No se encontro haarcascade_frontalface_default.xml. Deteccion facial desactivada.\n";
        }
    }

    bool procesarMano(const cv::Mat& frame, cv::Point& puntoCorte, cv::Mat& debugMask) {
        // --- PASO 1: DETECCIÓN Y SUPRESIÓN DEL ROSTRO Y CUELLO ---
        cv::Mat mascaraRostro = cv::Mat::zeros(frame.size(), CV_8UC1);
        if (cascadeCargada) {
            cv::Mat graySmall;
            cv::cvtColor(frame, graySmall, cv::COLOR_BGR2GRAY);
            cv::resize(graySmall, graySmall, cv::Size(), 0.5, 0.5);
            std::vector<cv::Rect> faces;
            faceCascade.detectMultiScale(graySmall, faces, 1.15, 3, 0, cv::Size(30, 30));

            for (const auto& f : faces) {
                int fx = f.x * 2;
                int fy = f.y * 2;
                int fw = f.width * 2;
                int fh = f.height * 2;

                // Expandir óvalo para enmascarar cabeza, barbilla, cuello y hombros
                int cx = fx + fw / 2;
                int cy = fy + fh / 2 + static_cast<int>(fh * 0.20f);
                int rx = static_cast<int>(fw * 0.70f);
                int ry = static_cast<int>(fh * 0.95f);
                cv::ellipse(mascaraRostro, cv::Point(cx, cy), cv::Size(rx, ry), 0, 0, 360, cv::Scalar(255), -1);
            }
        }

        // --- PASO 2: SEGMENTACIÓN HÍBRIDA DE PIEL (YCrCb + HSV) ---
        cv::Mat ycrcb, hsv;
        cv::cvtColor(frame, ycrcb, cv::COLOR_BGR2YCrCb);
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        cv::Mat maskYCrCb, maskHSV, mascaraPiel;
        cv::inRange(ycrcb, cv::Scalar(0, 133, 77), cv::Scalar(255, 175, 127), maskYCrCb);
        cv::inRange(hsv, cv::Scalar(0, 35, 50), cv::Scalar(25, 200, 255), maskHSV);

        cv::bitwise_and(maskYCrCb, maskHSV, mascaraPiel);

        // ¡ELIMINAR ROSTRO Y CUELLO DE LA MÁSCARA DE PIEL!
        mascaraPiel.setTo(0, mascaraRostro);

        // Limpieza morfológica estricta
        cv::Mat k3 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
        cv::Mat k7 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9));
        cv::morphologyEx(mascaraPiel, mascaraPiel, cv::MORPH_OPEN, k3);
        cv::morphologyEx(mascaraPiel, mascaraPiel, cv::MORPH_CLOSE, k7);
        cv::GaussianBlur(mascaraPiel, mascaraPiel, cv::Size(5, 5), 1.2);

        debugMask = mascaraPiel.clone();

        // --- PASO 3: EXTRACCIÓN DE CONTORNOS Y CENTRO DE LA PALMA ---
        std::vector<std::vector<cv::Point>> contornos;
        cv::findContours(mascaraPiel, contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        double maxArea = 0;
        int idxMejor = -1;
        for (size_t i = 0; i < contornos.size(); i++) {
            cv::Rect bb = cv::boundingRect(contornos[i]);
            // Descartar elementos estaticos pegados al borde superior (cuadros en la pared, lamparas)
            if (bb.y <= 15 && bb.br().y < frame.rows * 0.45f) {
                continue;
            }
            double a = cv::contourArea(contornos[i]);
            if (a > 3000 && a < 120000 && a > maxArea) {
                maxArea = a;
                idxMejor = static_cast<int>(i);
            }
        }

        if (idxMejor < 0) {
            dedoSuavizado = cv::Point2f(-1.0f, -1.0f);
            return false;
        }

        const auto& cMano = contornos[idxMejor];
        cv::Rect bBox = cv::boundingRect(cMano);

        // Máscara local para Transformada de Distancia (Centro exacto de la palma)
        cv::Mat subMascara = cv::Mat::zeros(bBox.size(), CV_8UC1);
        std::vector<cv::Point> cLocal;
        cLocal.reserve(cMano.size());
        for (const auto& p : cMano) cLocal.push_back(p - bBox.tl());
        cv::drawContours(subMascara, std::vector<std::vector<cv::Point>>{cLocal}, 0, cv::Scalar(255), -1);

        cv::Mat dist;
        cv::distanceTransform(subMascara, dist, cv::DIST_L2, 5);
        double maxDistVal = 0.0;
        cv::Point maxDistLoc;
        cv::minMaxLoc(dist, nullptr, &maxDistVal, nullptr, &maxDistLoc);

        cv::Point centroPalma = bBox.tl() + maxDistLoc;
        float radioPalma = static_cast<float>(maxDistVal);

        // --- PASO 4: CONVEX HULL Y BÚSQUEDA DE LA YEMA DEL DEDO ÍNDICE ---
        std::vector<cv::Point> hull;
        cv::convexHull(cMano, hull);

        cv::Point mejorDedo(-1, -1);
        double mayorDistancia = 0.0;

        for (const auto& hPt : hull) {
            // El dedo índice debe extenderse hacia arriba o lejos de la palma
            double d = cv::norm(hPt - centroPalma);
            if (d > radioPalma * 1.35 && hPt.y < centroPalma.y + 10) {
                // Priorizar el punto más alto y más proyectado
                double puntuacion = d + (centroPalma.y - hPt.y) * 1.5;
                if (puntuacion > mayorDistancia) {
                    mayorDistancia = puntuacion;
                    mejorDedo = hPt;
                }
            }
        }

        // Si no se encontró un dedo claramente extendido, usar el punto más alto de la mano
        if (mejorDedo.x < 0) {
            cv::Point masAlto = cMano[0];
            for (const auto& p : cMano) if (p.y < masAlto.y) masAlto = p;
            if (cv::norm(masAlto - centroPalma) > radioPalma * 1.2) {
                mejorDedo = masAlto;
            }
        }

        if (mejorDedo.x >= 0) {
            // Filtrado exponencial (EMA) para estabilizar la yema y dar cortes fluidos
            if (dedoSuavizado.x < 0) {
                dedoSuavizado = cv::Point2f(mejorDedo);
            } else {
                dedoSuavizado = 0.70f * cv::Point2f(mejorDedo) + 0.30f * dedoSuavizado;
            }
            puntoCorte = cv::Point(cvRound(dedoSuavizado.x), cvRound(dedoSuavizado.y));
            centroPalmaSuavizado = centroPalma;
            radioPalmaSuavizado = radioPalma;
            return true;
        }

        dedoSuavizado = cv::Point2f(-1.0f, -1.0f);
        return false;
    }
};

// ============================================================================
// 6. PROGRAMA PRINCIPAL: FRUIT NINJA CV
// ============================================================================
int main() {
    std::cout << "==============================================================\n";
    std::cout << "  INICIANDO FRUIT NINJA CV - PROCESAMIENTO DE VISION EN VIVO  \n";
    std::cout << "==============================================================\n";
    std::cout << "[+] Controles:\n";
    std::cout << "    - Corta las frutas deslizando tu dedo indice.\n";
    std::cout << "    - [D] Activar/Desactivar visualizacion de mascara de piel.\n";
    std::cout << "    - [ESC] Salir del juego.\n\n";

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "[ERROR] No se pudo acceder a la camara web.\n";
        return -1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    const std::string winName = "Fruit Ninja CV - Vision Artificial";
    cv::namedWindow(winName, cv::WINDOW_NORMAL);
    cv::resizeWindow(winName, 1600, 900);

    // Inicializar detector de manos y sprites
    DetectorManoNinja detector;
    bool mostrarDebug = false;

    // Sprites de frutas y mitades
    cv::Mat sandiaSprite, sandiaMask, sandiaIzq, sandiaDer, sandiaMaskIzq, sandiaMaskDer;
    cv::Mat naranjaSprite, naranjaMask, naranjaIzq, naranjaDer, naranjaMaskIzq, naranjaMaskDer;
    cv::Mat manzanaSprite, manzanaMask, manzanaIzq, manzanaDer, manzanaMaskIzq, manzanaMaskDer;
    cv::Mat platanoSprite, platanoMask, platanoIzq, platanoDer, platanoMaskIzq, platanoMaskDer;
    cv::Mat bombaSprite, bombaMask;

    generarSpriteSandia(sandiaSprite, sandiaMask, sandiaIzq, sandiaDer, sandiaMaskIzq, sandiaMaskDer, 44);
    generarSpriteNaranja(naranjaSprite, naranjaMask, naranjaIzq, naranjaDer, naranjaMaskIzq, naranjaMaskDer, 38);
    generarSpriteManzana(manzanaSprite, manzanaMask, manzanaIzq, manzanaDer, manzanaMaskIzq, manzanaMaskDer, 38);
    generarSpritePlatano(platanoSprite, platanoMask, platanoIzq, platanoDer, platanoMaskIzq, platanoMaskDer, 36);
    generarSpriteBomba(bombaSprite, bombaMask, 36);

    // Estado del juego
    EstadoJuego estado = ESTADO_MENU;
    int puntaje = 0;
    int record = 0;
    int vidas = 3;
    int framesShake = 0;
    int ticksGeneracion = 0;

    std::deque<cv::Point> rastroKatana;
    std::vector<Fruta> frutas;
    std::vector<MitadFruta> mitades;
    std::vector<ParticulaJugo> particulas;
    std::vector<TextoFlotante> textos;

    // Fruta central del menú para iniciar el juego
    Fruta frutaMenu;
    frutaMenu.tipo = FRUTA_SANDIA;
    frutaMenu.sprite = sandiaSprite.clone();
    frutaMenu.mascara = sandiaMask.clone();
    frutaMenu.pos = cv::Point2f(480, 280);
    frutaMenu.vel = cv::Point2f(0, 0);
    frutaMenu.radio = 50;
    frutaMenu.colorJugo = cv::Scalar(40, 40, 235);

    std::random_device rd;
    std::mt19937 rng(rd());

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;
        cv::flip(frame, frame, 1); // Modo espejo natural

        // Efecto de sacudida de pantalla (Screen Shake al cortar bomba o combo)
        if (framesShake > 0) {
            int dx = (rand() % 16) - 8;
            int dy = (rand() % 16) - 8;
            cv::Mat matTrans = (cv::Mat_<float>(2, 3) << 1, 0, dx, 0, 1, dy);
            cv::warpAffine(frame, frame, matTrans, frame.size(), cv::INTER_LINEAR, cv::BORDER_REFLECT);
            framesShake--;
        }

        // Procesar mano y dedo
        cv::Point puntaDedo(-1, -1);
        cv::Mat debugMask;
        bool manoDetectada = detector.procesarMano(frame, puntaDedo, debugMask);

        // Actualizar rastro de la katana
        if (manoDetectada) {
            rastroKatana.push_back(puntaDedo);
            if (rastroKatana.size() > 14) rastroKatana.pop_front();
        } else {
            if (!rastroKatana.empty()) rastroKatana.pop_front();
        }

        // ====================================================================
        // LÓGICA SEGÚN ESTADO DEL JUEGO
        // ====================================================================
        if (estado == ESTADO_MENU) {
            // Fondo oscuro semitransparente para ambientación
            cv::Mat overlay = frame.clone();
            cv::rectangle(overlay, cv::Rect(0, 0, frame.cols, frame.rows), cv::Scalar(15, 10, 8), -1);
            cv::addWeighted(overlay, 0.45, frame, 0.55, 0, frame);

            // Título arcade
            cv::putText(frame, "FRUIT NINJA CV", cv::Point(frame.cols / 2 - 250, 90),
                        cv::FONT_HERSHEY_DUPLEX, 1.6, cv::Scalar(0, 215, 255), 3, cv::LINE_AA);
            cv::putText(frame, "Corta la sandia con tu dedo para jugar", cv::Point(frame.cols / 2 - 220, 140),
                        cv::FONT_HERSHEY_SIMPLEX, 0.72, cv::Scalar(240, 240, 240), 2, cv::LINE_AA);

            // Instrucciones
            cv::rectangle(frame, cv::Rect(40, frame.rows - 110, frame.cols - 80, 80), cv::Scalar(25, 20, 18), -1);
            cv::rectangle(frame, cv::Rect(40, frame.rows - 110, frame.cols - 80, 80), cv::Scalar(60, 50, 40), 1);
            cv::putText(frame, "1. Muestra tu mano a la camara y extiende tu dedo indice.", cv::Point(60, frame.rows - 75),
                        cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(200, 240, 200), 1, cv::LINE_AA);
            cv::putText(frame, "2. Realiza movimientos rapidos en el aire para rebanar frutas y esquivar bombas.", cv::Point(60, frame.rows - 45),
                        cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(200, 240, 200), 1, cv::LINE_AA);

            // Fruta central de inicio
            frutaMenu.angulo += 2.0f;
            cv::Mat spriteRot = rotarSpriteAfin(frutaMenu.sprite, frutaMenu.angulo);
            cv::Mat maskRot   = rotarSpriteAfin(frutaMenu.mascara, frutaMenu.angulo);
            dibujarSpriteSeguro(frame, spriteRot, maskRot, frutaMenu.pos);

            cv::putText(frame, "CORTA AQUI", cv::Point(frutaMenu.pos.x - 70, frutaMenu.pos.y + 75),
                        cv::FONT_HERSHEY_DUPLEX, 0.75, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

            // Detectar si el usuario corta la sandía de inicio
            if (!rastroKatana.empty()) {
                double dist = cv::norm(frutaMenu.pos - cv::Point2f(rastroKatana.back().x, rastroKatana.back().y));
                if (dist < frutaMenu.radio + 15) {
                    estado = ESTADO_JUGANDO;
                    puntaje = 0;
                    vidas = 3;
                    frutas.clear();
                    mitades.clear();
                    particulas.clear();
                    textos.clear();

                    // Explosión de jugo
                    for (int p = 0; p < 35; p++) {
                        ParticulaJugo pj;
                        pj.pos = frutaMenu.pos;
                        float ang = (rand() % 360) * 3.14159f / 180.0f;
                        float vel = (rand() % 9 + 4.0f);
                        pj.vel = cv::Point2f(std::cos(ang) * vel, std::sin(ang) * vel);
                        pj.color = frutaMenu.colorJugo;
                        pj.radio = (rand() % 5 + 3.0f);
                        pj.vida = 30;
                        pj.vidaMaxima = 30;
                        particulas.push_back(pj);
                    }
                }
            }

        } else if (estado == ESTADO_JUGANDO) {
            // --- GENERACIÓN DE OLEADAS DE FRUTAS ---
            ticksGeneracion++;
            if (ticksGeneracion >= 32) {
                ticksGeneracion = 0;
                int cantidad = (rand() % 2) + 1; // 1 o 2 frutas por oleada
                if (puntaje > 80 && (rand() % 3 == 0)) cantidad = 3;

                for (int c = 0; c < cantidad; c++) {
                    Fruta f;
                    int rTipo = rand() % 10;
                    if (rTipo < 3) {
                        f.tipo = FRUTA_SANDIA;
                        f.sprite = sandiaSprite.clone();
                        f.mascara = sandiaMask.clone();
                        f.radio = 42;
                        f.colorJugo = cv::Scalar(40, 40, 240);
                    } else if (rTipo < 6) {
                        f.tipo = FRUTA_NARANJA;
                        f.sprite = naranjaSprite.clone();
                        f.mascara = naranjaMask.clone();
                        f.radio = 38;
                        f.colorJugo = cv::Scalar(0, 160, 255);
                    } else if (rTipo < 8) {
                        f.tipo = FRUTA_MANZANA;
                        f.sprite = manzanaSprite.clone();
                        f.mascara = manzanaMask.clone();
                        f.radio = 38;
                        f.colorJugo = cv::Scalar(20, 20, 230);
                    } else if (rTipo < 9) {
                        f.tipo = FRUTA_PLATANO;
                        f.sprite = platanoSprite.clone();
                        f.mascara = platanoMask.clone();
                        f.radio = 36;
                        f.colorJugo = cv::Scalar(40, 230, 255);
                    } else { // Bomba Ninja
                        f.tipo = OBJETO_BOMBA;
                        f.sprite = bombaSprite.clone();
                        f.mascara = bombaMask.clone();
                        f.radio = 36;
                        f.colorJugo = cv::Scalar(200, 200, 200);
                    }

                    int posX = (rand() % (frame.cols - 240)) + 120;
                    f.pos = cv::Point2f(posX, frame.rows + 30);
                    float vx = (frame.cols / 2.0f - posX) * 0.012f + ((rand() % 5) - 2.5f);
                    float vy = -(rand() % 4 + 14.0f);
                    f.vel = cv::Point2f(vx, vy);
                    f.velGiro = ((rand() % 12) - 6.0f);
                    if (std::abs(f.velGiro) < 1.0f) f.velGiro = 3.5f;

                    frutas.push_back(f);
                }
            }

            // --- FÍSICA Y DETECCIÓN DE CORTES ---
            int frutasCortadasEnFrame = 0;

            for (size_t i = 0; i < frutas.size();) {
                frutas[i].pos += frutas[i].vel;
                frutas[i].vel.y += 0.42f; // Gravedad
                frutas[i].angulo += frutas[i].velGiro;

                // Colisión con la espada (yema del dedo)
                if (!frutas[i].cortada && !rastroKatana.empty()) {
                    cv::Point2f posEspada(rastroKatana.back().x, rastroKatana.back().y);
                    double dist = cv::norm(frutas[i].pos - posEspada);

                    if (dist < frutas[i].radio + 16) {
                        frutas[i].cortada = true;

                        if (frutas[i].tipo == OBJETO_BOMBA) {
                            // ¡CORTÓ UNA BOMBA!
                            framesShake = 12;
                            vidas -= 3;
                            cv::rectangle(frame, cv::Rect(0, 0, frame.cols, frame.rows), cv::Scalar(255, 255, 255), -1);

                            TextoFlotante tf{"¡¡BOOM!!", frutas[i].pos, cv::Scalar(0, 0, 255), 1.4f, 35};
                            textos.push_back(tf);

                            if (vidas <= 0) {
                                estado = ESTADO_GAMEOVER;
                                if (puntaje > record) record = puntaje;
                            }
                        } else {
                            // Fruta cortada con éxito
                            frutasCortadasEnFrame++;
                            puntaje += 10;

                            // Generar mitades voladoras
                            cv::Mat mIzq, mDer, mkIzq, mkDer;
                            if (frutas[i].tipo == FRUTA_SANDIA) {
                                mIzq = sandiaIzq.clone(); mDer = sandiaDer.clone();
                                mkIzq = sandiaMaskIzq.clone(); mkDer = sandiaMaskDer.clone();
                            } else if (frutas[i].tipo == FRUTA_NARANJA) {
                                mIzq = naranjaIzq.clone(); mDer = naranjaDer.clone();
                                mkIzq = naranjaMaskIzq.clone(); mkDer = naranjaMaskDer.clone();
                            } else if (frutas[i].tipo == FRUTA_MANZANA) {
                                mIzq = manzanaIzq.clone(); mDer = manzanaDer.clone();
                                mkIzq = manzanaMaskIzq.clone(); mkDer = manzanaMaskDer.clone();
                            } else {
                                mIzq = platanoIzq.clone(); mDer = platanoDer.clone();
                                mkIzq = platanoMaskIzq.clone(); mkDer = platanoMaskDer.clone();
                            }

                            MitadFruta m1{ mIzq, mkIzq, frutas[i].pos, cv::Point2f(-5.5f, -3.5f), frutas[i].angulo, -8.0f };
                            MitadFruta m2{ mDer, mkDer, frutas[i].pos, cv::Point2f(5.5f, -3.5f), frutas[i].angulo, 8.0f };
                            mitades.push_back(m1);
                            mitades.push_back(m2);

                            // Partículas de salpicadura de jugo
                            for (int p = 0; p < 25; p++) {
                                ParticulaJugo pj;
                                pj.pos = frutas[i].pos;
                                float ang = (rand() % 360) * 3.14159f / 180.0f;
                                float vel = (rand() % 8 + 3.0f);
                                pj.vel = cv::Point2f(std::cos(ang) * vel, std::sin(ang) * vel);
                                pj.color = frutas[i].colorJugo;
                                pj.radio = (rand() % 5 + 3.0f);
                                pj.vida = 25;
                                pj.vidaMaxima = 25;
                                particulas.push_back(pj);
                            }

                            TextoFlotante tf{"+10", frutas[i].pos, cv::Scalar(0, 255, 120), 0.85f, 25};
                            textos.push_back(tf);
                        }
                    }
                }

                // Dibujar fruta si no ha sido cortada
                if (!frutas[i].cortada) {
                    cv::Mat spRot = rotarSpriteAfin(frutas[i].sprite, frutas[i].angulo);
                    cv::Mat mkRot = rotarSpriteAfin(frutas[i].mascara, frutas[i].angulo);
                    dibujarSpriteSeguro(frame, spRot, mkRot, frutas[i].pos);
                }

                // Si la fruta cae sin cortar por debajo de la pantalla
                if (frutas[i].pos.y > frame.rows + 60) {
                    if (!frutas[i].cortada && frutas[i].tipo != OBJETO_BOMBA) {
                        vidas--;
                        TextoFlotante tf{"X", cv::Point2f(frutas[i].pos.x, frame.rows - 30), cv::Scalar(0, 0, 255), 1.2f, 20};
                        textos.push_back(tf);
                        if (vidas <= 0) {
                            estado = ESTADO_GAMEOVER;
                            if (puntaje > record) record = puntaje;
                        }
                    }
                    frutas.erase(frutas.begin() + i);
                } else if (frutas[i].cortada) {
                    frutas.erase(frutas.begin() + i);
                } else {
                    ++i;
                }
            }

            // Bonificación por combos múltiples
            if (frutasCortadasEnFrame >= 2) {
                int bonus = frutasCortadasEnFrame * 15;
                puntaje += bonus;
                framesShake = 6;
                std::string msgCombo = "COMBO x" + std::to_string(frutasCortadasEnFrame) + " (+" + std::to_string(bonus) + ")";
                TextoFlotante tc{msgCombo, cv::Point2f(frame.cols / 2 - 120, 150), cv::Scalar(0, 215, 255), 1.1f, 35};
                textos.push_back(tc);
            }

            // --- ANIMACIÓN DE MITADES CORTADAS ---
            for (size_t i = 0; i < mitades.size();) {
                mitades[i].pos += mitades[i].vel;
                mitades[i].vel.y += 0.48f;
                mitades[i].angulo += mitades[i].velGiro;

                cv::Mat sRot = rotarSpriteAfin(mitades[i].sprite, mitades[i].angulo);
                cv::Mat mRot = rotarSpriteAfin(mitades[i].mascara, mitades[i].angulo);
                dibujarSpriteSeguro(frame, sRot, mRot, mitades[i].pos);

                if (mitades[i].pos.y > frame.rows + 80) {
                    mitades.erase(mitades.begin() + i);
                } else {
                    ++i;
                }
            }

            // --- RENDERIZADO DEL HUD SUPERIOR ---
            cv::rectangle(frame, cv::Rect(0, 0, frame.cols, 65), cv::Scalar(20, 16, 14), -1);
            cv::line(frame, cv::Point(0, 65), cv::Point(frame.cols, 65), cv::Scalar(0, 200, 255), 2);

            // Puntaje
            cv::putText(frame, "PUNTOS: " + std::to_string(puntaje), cv::Point(25, 42),
                        cv::FONT_HERSHEY_DUPLEX, 0.95, cv::Scalar(0, 230, 255), 2, cv::LINE_AA);

            // Récord
            cv::putText(frame, "RECORD: " + std::to_string(std::max(record, puntaje)), cv::Point(260, 42),
                        cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(180, 180, 180), 1, cv::LINE_AA);

            // Vidas (Cruces / Espadas rojas)
            for (int v = 0; v < 3; v++) {
                cv::Point cV(frame.cols - 120 + v * 35, 34);
                if (v < vidas) {
                    cv::circle(frame, cV, 12, cv::Scalar(40, 200, 50), -1, cv::LINE_AA);
                    cv::putText(frame, "V", cV + cv::Point(-6, 5), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(20, 20, 20), 2);
                } else {
                    cv::circle(frame, cV, 12, cv::Scalar(30, 30, 200), -1, cv::LINE_AA);
                    cv::putText(frame, "X", cV + cv::Point(-6, 5), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 2);
                }
            }

        } else if (estado == ESTADO_GAMEOVER) {
            // Fondo oscuro difuminado
            cv::Mat overlay = frame.clone();
            cv::rectangle(overlay, cv::Rect(0, 0, frame.cols, frame.rows), cv::Scalar(10, 10, 20), -1);
            cv::addWeighted(overlay, 0.65, frame, 0.35, 0, frame);

            cv::putText(frame, "FIN DE LA PARTIDA", cv::Point(frame.cols / 2 - 240, 120),
                        cv::FONT_HERSHEY_DUPLEX, 1.4, cv::Scalar(0, 0, 255), 3, cv::LINE_AA);

            cv::putText(frame, "PUNTAJE FINAL: " + std::to_string(puntaje), cv::Point(frame.cols / 2 - 160, 180),
                        cv::FONT_HERSHEY_DUPLEX, 0.85, cv::Scalar(0, 230, 255), 2, cv::LINE_AA);
            cv::putText(frame, "MEJOR RECORD:  " + std::to_string(record), cv::Point(frame.cols / 2 - 160, 220),
                        cv::FONT_HERSHEY_SIMPLEX, 0.70, cv::Scalar(200, 200, 200), 1, cv::LINE_AA);

            // Sandía para reiniciar
            frutaMenu.pos = cv::Point2f(frame.cols / 2, 340);
            frutaMenu.angulo += 2.0f;
            cv::Mat spRot = rotarSpriteAfin(frutaMenu.sprite, frutaMenu.angulo);
            cv::Mat mkRot = rotarSpriteAfin(frutaMenu.mascara, frutaMenu.angulo);
            dibujarSpriteSeguro(frame, spRot, mkRot, frutaMenu.pos);

            cv::putText(frame, "CORTA PARA VOLVER A JUGAR", cv::Point(frame.cols / 2 - 180, 430),
                        cv::FONT_HERSHEY_DUPLEX, 0.70, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

            if (!rastroKatana.empty()) {
                double dist = cv::norm(frutaMenu.pos - cv::Point2f(rastroKatana.back().x, rastroKatana.back().y));
                if (dist < frutaMenu.radio + 15) {
                    estado = ESTADO_JUGANDO;
                    puntaje = 0;
                    vidas = 3;
                    frutas.clear();
                    mitades.clear();
                    particulas.clear();
                    textos.clear();
                }
            }
        }

        // --- PARTÍCULAS DE JUGO ---
        for (size_t p = 0; p < particulas.size();) {
            particulas[p].pos += particulas[p].vel;
            particulas[p].vel.y += 0.35f;
            particulas[p].vida--;

            float alfa = static_cast<float>(particulas[p].vida) / particulas[p].vidaMaxima;
            int radioP = std::max(1, cvRound(particulas[p].radio * alfa));
            cv::circle(frame, particulas[p].pos, radioP, particulas[p].color, -1, cv::LINE_AA);

            if (particulas[p].vida <= 0 || particulas[p].pos.y > frame.rows) {
                particulas.erase(particulas.begin() + p);
            } else {
                ++p;
            }
        }

        // --- TEXTOS FLOTANTES ---
        for (size_t t = 0; t < textos.size();) {
            textos[t].pos.y -= 1.8f;
            textos[t].vida--;

            cv::putText(frame, textos[t].texto, textos[t].pos,
                        cv::FONT_HERSHEY_DUPLEX, textos[t].escala, textos[t].color, 2, cv::LINE_AA);

            if (textos[t].vida <= 0) {
                textos.erase(textos.begin() + t);
            } else {
                ++t;
            }
        }

        // --- ESTELA BRILLANTE DE LA KATANA (BLADE TRAIL) ---
        if (rastroKatana.size() >= 2) {
            for (size_t i = 1; i < rastroKatana.size(); i++) {
                float factor = static_cast<float>(i) / rastroKatana.size();
                int grosor = std::max(2, cvRound(12 * factor));

                // Borde exterior dorado/cian
                cv::line(frame, rastroKatana[i - 1], rastroKatana[i], cv::Scalar(0, 180, 255), grosor + 4, cv::LINE_AA);
                // Núcleo blanco brillante
                cv::line(frame, rastroKatana[i - 1], rastroKatana[i], cv::Scalar(255, 255, 255), grosor, cv::LINE_AA);
            }

            // Destello en la punta de la espada
            cv::circle(frame, rastroKatana.back(), 9, cv::Scalar(0, 220, 255), 2, cv::LINE_AA);
            cv::circle(frame, rastroKatana.back(), 5, cv::Scalar(255, 255, 255), -1, cv::LINE_AA);
        }

        // Indicador de modo en la esquina inferior izquierda
        cv::putText(frame, "Modo: Mano Inteligente (Filtrado Facial)", cv::Point(20, frame.rows - 15),
                    cv::FONT_HERSHEY_SIMPLEX, 0.48, cv::Scalar(180, 180, 180), 1, cv::LINE_AA);

        cv::imshow(winName, frame);

        if (mostrarDebug && !debugMask.empty()) {
            cv::imshow("Debug: Mascara de Piel (Sin Rostro)", debugMask);
        }

        char k = static_cast<char>(cv::waitKey(12));
        if (k == 27 || k == 'q' || k == 'Q') {
            break;
        } else if (k == 'd' || k == 'D') {
            mostrarDebug = !mostrarDebug;
            if (!mostrarDebug) cv::destroyWindow("Debug: Mascara de Piel (Sin Rostro)");
        }
    }

    cap.release();
    cv::destroyAllWindows();
    std::cout << "[+] Fruit Ninja CV finalizado.\n";
    return 0;
}
