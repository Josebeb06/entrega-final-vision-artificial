#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Fruit Ninja CV - Vision Artificial (Optimizacion de Alto Rendimiento 60 FPS)
===========================================================================
Implementacion avanzada de Fruit Ninja en Realidad Aumentada (AR) utilizando
OpenCV y MediaPipe HandLandmarker (Deep Learning con aceleracion LiteRT/XNNPACK).

Optimizaciones de rendimiento:
- Captura de camara asincrona multihilo (ThreadedCamera con backend DirectShow en Windows).
- Eliminacion de bucles de punto flotante sobre toda la pantalla (operaciones vectoriales SIMD).
- Sistema de manchas de jugo ligero basado en objetos vectoriales.
- Alpha blending rapido con aritmetica entera de 16 bits en lugar de floats.
- Sin llamadas bloqueantes de sonido del sistema (winsound.Beep desactivado).
"""

import os
import sys
import math
import time
import random
import threading
from collections import deque
import numpy as np
import cv2

# Importar MediaPipe Tasks Vision API (MediaPipe 1.x / LiteRT)
try:
    import mediapipe as mp
    from mediapipe.tasks import python as mp_python
    from mediapipe.tasks.python import vision as mp_vision
    MEDIAPIPE_AVAILABLE = True
except Exception as e:
    print(f"[!] Error al importar MediaPipe: {e}")
    MEDIAPIPE_AVAILABLE = False


# ==============================================================================
# CONFIGURACION GLOBAL
# ==============================================================================
CANVAS_W = 1920
CANVAS_H = 1080
GRAVITY = 0.58
MIN_CUT_SPEED = 14.0       # Velocidad minima en px/frame para activar corte
COMBO_TIME_WINDOW = 0.35    # Segundos para agrupar cortes en combo

# Estados de Juego
STATE_MENU = 0
STATE_PLAYING = 1
STATE_GAME_OVER = 2


# ==============================================================================
# GESTOR DE AUDIO (SILENCIOSO - SIN PITIDOS MOLESTOS NI BLOQUEOS DE GIL)
# ==============================================================================
def play_sound_async(sound_type):
    """Desactivado a solicitud del usuario para eliminar pitidos molestos y optimizar FPS."""
    pass


# ==============================================================================
# CAPTURA DE CAMARA MULTIHILO DE ALTO RENDIMIENTO
# ==============================================================================
class ThreadedCamera:
    """Captura de camara en segundo plano para no bloquear el bucle principal de renderizado."""
    def __init__(self, src=0, width=1280, height=720):
        if sys.platform.startswith("win"):
            # cv2.CAP_DSHOW en Windows inicializa de inmediato y permite 30-60 FPS fluidos
            self.cap = cv2.VideoCapture(src, cv2.CAP_DSHOW)
            if not self.cap.isOpened():
                self.cap = cv2.VideoCapture(src)
        else:
            self.cap = cv2.VideoCapture(src)

        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
        self.cap.set(cv2.CAP_PROP_FPS, 60)

        self.grabbed, self.frame = self.cap.read()
        self.running = True
        self.lock = threading.Lock()
        self.thread = threading.Thread(target=self._reader, daemon=True)
        self.thread.start()

    def _reader(self):
        while self.running:
            grabbed, frame = self.cap.read()
            if grabbed and frame is not None:
                with self.lock:
                    self.grabbed = grabbed
                    self.frame = frame
            else:
                time.sleep(0.005)

    def read(self):
        with self.lock:
            if self.frame is not None:
                return self.grabbed, self.frame.copy()
            return False, None

    def release(self):
        self.running = False
        if self.cap.isOpened():
            self.cap.release()


# ==============================================================================
# GENERACION PROCEDURAL DE SPRITES DE FRUTAS HD (RGBA)
# ==============================================================================
def create_watermelon_sprites(radius=68):
    size = radius * 2 + 32
    c = size // 2
    whole = np.zeros((size, size, 4), dtype=np.uint8)
    
    cv2.circle(whole, (c, c), radius, (35, 135, 45, 255), -1, cv2.LINE_AA)
    for angle in range(0, 360, 30):
        rad = math.radians(angle)
        x2 = int(c + (radius - 2) * math.cos(rad))
        y2 = int(c + (radius - 2) * math.sin(rad))
        cv2.line(whole, (c, c), (x2, y2), (20, 80, 25, 255), 5, cv2.LINE_AA)
        
    cv2.circle(whole, (c, c), radius - 8, (215, 245, 215, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c, c), radius - 14, (45, 40, 235, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c, c), radius - 24, (65, 60, 245, 255), -1, cv2.LINE_AA)
    
    seed_offsets = [(-20, -12), (20, -12), (0, -28), (-14, 18), (14, 18), (0, 8), (-32, 0), (32, 0)]
    for sx, sy in seed_offsets:
        cv2.circle(whole, (c + sx, c + sy), 4, (15, 15, 15, 255), -1, cv2.LINE_AA)
        cv2.circle(whole, (c + sx - 1, c + sy - 1), 1, (180, 180, 180, 255), -1, cv2.LINE_AA)

    half_left = whole.copy()
    half_left[:, c:] = 0
    cv2.line(half_left, (c - 1, c - radius + 14), (c - 1, c + radius - 14), (55, 50, 250, 255), 3, cv2.LINE_AA)

    half_right = whole.copy()
    half_right[:, :c] = 0
    cv2.line(half_right, (c, c - radius + 14), (c, c + radius - 14), (55, 50, 250, 255), 3, cv2.LINE_AA)

    return whole, half_left, half_right, radius, (45, 40, 235)


def create_orange_sprites(radius=64):
    size = radius * 2 + 32
    c = size // 2
    whole = np.zeros((size, size, 4), dtype=np.uint8)
    
    cv2.circle(whole, (c, c), radius, (0, 135, 250, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c, c), radius - 7, (215, 240, 255, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c, c), radius - 12, (10, 165, 255, 255), -1, cv2.LINE_AA)
    
    r_gajo = radius - 16
    for i in range(8):
        a1 = math.radians(i * 45 + 5)
        a2 = math.radians(i * 45 + 40)
        pts = np.array([
            [c, c],
            [int(c + r_gajo * math.cos(a1)), int(c + r_gajo * math.sin(a1))],
            [int(c + r_gajo * math.cos(a2)), int(c + r_gajo * math.sin(a2))]
        ], dtype=np.int32)
        cv2.fillConvexPoly(whole, pts, (0, 195, 255, 255), cv2.LINE_AA)
        
    cv2.circle(whole, (c, c), 7, (245, 250, 255, 255), -1, cv2.LINE_AA)

    half_left = whole.copy()
    half_left[:, c:] = 0
    cv2.line(half_left, (c - 1, c - radius + 7), (c - 1, c + radius - 7), (230, 245, 255, 255), 3, cv2.LINE_AA)

    half_right = whole.copy()
    half_right[:, :c] = 0
    cv2.line(half_right, (c, c - radius + 7), (c, c + radius - 7), (230, 245, 255, 255), 3, cv2.LINE_AA)

    return whole, half_left, half_right, radius, (0, 165, 255)


def create_apple_sprites(radius=64):
    size = radius * 2 + 36
    c = size // 2
    whole = np.zeros((size, size, 4), dtype=np.uint8)
    
    cv2.circle(whole, (c - 14, c + 2), radius - 4, (25, 25, 220, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c + 14, c + 2), radius - 4, (25, 25, 220, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c, c + 10), radius - 6, (30, 30, 240, 255), -1, cv2.LINE_AA)
    
    cv2.ellipse(whole, (c - 20, c - 16), (15, 8), -30, 0, 360, (120, 120, 255, 255), -1, cv2.LINE_AA)
    cv2.line(whole, (c, c - radius + 8), (c + 6, c - radius - 12), (30, 60, 115, 255), 4, cv2.LINE_AA)
    cv2.ellipse(whole, (c + 14, c - radius - 6), (13, 6), 35, 0, 360, (40, 200, 60, 255), -1, cv2.LINE_AA)

    half_left = whole.copy()
    half_left[:, c:] = 0
    cv2.ellipse(half_left, (c - 2, c + 4), (16, 26), 0, 0, 360, (210, 245, 245, 255), -1, cv2.LINE_AA)
    cv2.circle(half_left, (c - 6, c + 2), 3, (20, 40, 70, 255), -1, cv2.LINE_AA)

    half_right = whole.copy()
    half_right[:, :c] = 0
    cv2.ellipse(half_right, (c + 2, c + 4), (16, 26), 0, 0, 360, (210, 245, 245, 255), -1, cv2.LINE_AA)
    cv2.circle(half_right, (c + 6, c + 2), 3, (20, 40, 70, 255), -1, cv2.LINE_AA)

    return whole, half_left, half_right, radius, (40, 40, 235)


def create_banana_sprites(radius=62):
    size = radius * 2 + 36
    c = size // 2
    whole = np.zeros((size, size, 4), dtype=np.uint8)
    
    cv2.ellipse(whole, (c, c + 8), (54, 26), 25, 0, 360, (25, 220, 245, 255), -1, cv2.LINE_AA)
    cv2.ellipse(whole, (c + 6, c + 2), (48, 20), 25, 0, 360, (45, 235, 255, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c - 40, c - 8), 6, (25, 80, 50, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c + 42, c + 26), 5, (25, 50, 80, 255), -1, cv2.LINE_AA)

    half_left = whole.copy()
    half_left[:, c:] = 0
    cv2.circle(half_left, (c - 2, c + 8), 10, (220, 245, 245, 255), -1, cv2.LINE_AA)

    half_right = whole.copy()
    half_right[:, :c] = 0
    cv2.circle(half_right, (c + 2, c + 8), 10, (220, 245, 245, 255), -1, cv2.LINE_AA)

    return whole, half_left, half_right, radius, (30, 225, 255)


def create_lemon_sprites(radius=58):
    size = radius * 2 + 32
    c = size // 2
    whole = np.zeros((size, size, 4), dtype=np.uint8)
    
    cv2.ellipse(whole, (c, c), (radius + 4, radius - 8), -20, 0, 360, (20, 225, 250, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c - radius + 2, c + 14), 6, (20, 180, 210, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c + radius - 2, c - 14), 6, (20, 180, 210, 255), -1, cv2.LINE_AA)
    cv2.ellipse(whole, (c - 10, c - 6), (radius // 2, radius // 3), -20, 0, 360, (90, 240, 255, 255), -1, cv2.LINE_AA)

    half_left = whole.copy()
    half_left[:, c:] = 0
    half_right = whole.copy()
    half_right[:, :c] = 0

    return whole, half_left, half_right, radius, (20, 235, 255)


def create_bomb_sprites(radius=62):
    size = radius * 2 + 36
    c = size // 2
    whole = np.zeros((size, size, 4), dtype=np.uint8)
    
    cv2.circle(whole, (c, c + 6), radius, (36, 36, 40, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c, c + 6), radius - 4, (50, 50, 56, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c - 18, c - 12), radius // 3, (110, 110, 125, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c - 22, c - 16), 6, (230, 230, 245, 255), -1, cv2.LINE_AA)
    
    cv2.rectangle(whole, (c - 10, c - radius - 2), (c + 10, c - radius + 6), (40, 160, 215, 255), -1, cv2.LINE_AA)
    cv2.line(whole, (c, c - radius - 2), (c + 12, c - radius - 16), (90, 145, 190, 255), 4, cv2.LINE_AA)
    cv2.circle(whole, (c, c + 6), 14, (200, 200, 220, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c - 5, c + 4), 3, (30, 30, 35, 255), -1, cv2.LINE_AA)
    cv2.circle(whole, (c + 5, c + 4), 3, (30, 30, 35, 255), -1, cv2.LINE_AA)
    cv2.line(whole, (c - 4, c + 14), (c + 4, c + 14), (30, 30, 35, 255), 2, cv2.LINE_AA)

    return whole, whole, whole, radius, (50, 120, 255)


# ==============================================================================
# CLASE MANCHA DE JUGO LIGERA (SIN MATRIZ FLOAT32 GIGANTE)
# ==============================================================================
class Splatter:
    """Mancha de jugo ligera en el lente que desaparece gradualmente."""
    def __init__(self, cx, cy, color):
        self.blobs = []
        self.life = 70
        self.max_life = 70
        self.color = color
        # 6 a 10 gotas alrededor del centro
        for _ in range(random.randint(6, 10)):
            bx = cx + random.randint(-65, 65)
            by = cy + random.randint(-65, 65)
            r = random.randint(7, 22)
            self.blobs.append((bx, by, r))

    def update(self):
        self.life -= 1
        return self.life > 0

    def draw(self, canvas):
        if self.life <= 0:
            return
        # Dibujar gotas con radio atenuado
        factor = self.life / self.max_life
        for bx, by, r in self.blobs:
            cur_r = max(2, int(r * factor))
            if 0 <= bx < CANVAS_W and 0 <= by < CANVAS_H:
                cv2.circle(canvas, (bx, by), cur_r, self.color, -1, cv2.LINE_AA)


# ==============================================================================
# CLASE PARTICULA Y TEXTO FLOTANTE
# ==============================================================================
class Particle:
    def __init__(self, x, y, vx, vy, color, radius, life=28, is_spark=False):
        self.x = float(x)
        self.y = float(y)
        self.vx = float(vx)
        self.vy = float(vy)
        self.color = color
        self.radius = float(radius)
        self.max_life = life
        self.life = life
        self.is_spark = is_spark

    def update(self):
        self.x += self.vx
        self.y += self.vy
        if not self.is_spark:
            self.vy += GRAVITY * 0.5
        self.life -= 1
        return self.life > 0

    def draw(self, canvas):
        if self.life <= 0:
            return
        r = max(1, int(self.radius * (self.life / self.max_life)))
        ix, iy = int(self.x), int(self.y)
        if 0 <= ix < CANVAS_W and 0 <= iy < CANVAS_H:
            if self.is_spark:
                cv2.circle(canvas, (ix, iy), r + 2, self.color, -1, cv2.LINE_AA)
                cv2.circle(canvas, (ix, iy), max(1, r // 2), (255, 255, 255), -1, cv2.LINE_AA)
            else:
                cv2.circle(canvas, (ix, iy), r, self.color, -1, cv2.LINE_AA)


class FloatingText:
    def __init__(self, x, y, text, color=(0, 255, 255), scale=1.3, life=40):
        self.x = float(x)
        self.y = float(y)
        self.text = text
        self.color = color
        self.scale = scale
        self.life = life

    def update(self):
        self.y -= 2.0
        self.life -= 1
        return self.life > 0

    def draw(self, canvas):
        if self.life <= 0:
            return
        ix, iy = int(self.x), int(self.y)
        cv2.putText(canvas, self.text, (ix, iy), cv2.FONT_HERSHEY_DUPLEX, self.scale, (0, 0, 0), 5, cv2.LINE_AA)
        cv2.putText(canvas, self.text, (ix, iy), cv2.FONT_HERSHEY_DUPLEX, self.scale, self.color, 2, cv2.LINE_AA)


# ==============================================================================
# CLASE FRUTA
# ==============================================================================
class Fruit:
    def __init__(self, fruit_type, x, y, vx, vy, sprites_dict):
        self.fruit_type = fruit_type
        self.x = float(x)
        self.y = float(y)
        self.vx = float(vx)
        self.vy = float(vy)
        self.angle = random.uniform(0, 360)
        self.v_rot = random.uniform(-4.5, 4.5)
        self.sliced = False
        self.halves_life = 55
        
        self.whole_sprite, self.half_left_sprite, self.half_right_sprite, self.radius, self.juice_color = sprites_dict[fruit_type]
        self.is_bomb = (fruit_type == "bomba")
        
        self.hl_x = 0.0
        self.hl_y = 0.0
        self.hl_vx = 0.0
        self.hl_vy = 0.0
        self.hl_angle = 0.0
        self.hl_vrot = 0.0

        self.hr_x = 0.0
        self.hr_y = 0.0
        self.hr_vx = 0.0
        self.hr_vy = 0.0
        self.hr_angle = 0.0
        self.hr_vrot = 0.0

    def slice_fruit(self, cut_angle=0.0):
        if self.sliced:
            return
        self.sliced = True
        self.hl_x, self.hl_y = self.x, self.y
        self.hr_x, self.hr_y = self.x, self.y
        self.hl_angle = self.angle
        self.hr_angle = self.angle
        
        sep_force = random.uniform(6.5, 9.5)
        self.hl_vx = self.vx - sep_force
        self.hl_vy = self.vy - random.uniform(2.0, 4.0)
        self.hl_vrot = -random.uniform(6.0, 10.0)

        self.hr_vx = self.vx + sep_force
        self.hr_vy = self.vy - random.uniform(2.0, 4.0)
        self.hr_vrot = random.uniform(6.0, 10.0)

    def update(self):
        if not self.sliced:
            self.x += self.vx
            self.y += self.vy
            self.vy += GRAVITY
            self.angle = (self.angle + self.v_rot) % 360
            if self.y > CANVAS_H + 120 and self.vy > 0:
                return False
            return True
        else:
            self.hl_x += self.hl_vx
            self.hl_y += self.hl_vy
            self.hl_vy += GRAVITY * 1.1
            self.hl_angle = (self.hl_angle + self.hl_vrot) % 360

            self.hr_x += self.hr_vx
            self.hr_y += self.hr_vy
            self.hr_vy += GRAVITY * 1.1
            self.hr_angle = (self.hr_angle + self.hr_vrot) % 360

            self.halves_life -= 1
            return self.halves_life > 0

    def draw(self, canvas):
        if not self.sliced:
            _draw_rotated_sprite(canvas, self.whole_sprite, int(self.x), int(self.y), self.angle)
            if self.is_bomb:
                rad = math.radians(self.angle - 90)
                fuse_x = int(self.x + (self.radius + 12) * math.cos(rad))
                fuse_y = int(self.y + (self.radius + 12) * math.sin(rad))
                cv2.circle(canvas, (fuse_x, fuse_y), random.randint(4, 7), (0, 220, 255), -1, cv2.LINE_AA)
                cv2.circle(canvas, (fuse_x, fuse_y), 2, (255, 255, 255), -1, cv2.LINE_AA)
        else:
            alpha_fade = max(0.0, min(1.0, self.halves_life / 55.0))
            _draw_rotated_sprite(canvas, self.half_left_sprite, int(self.hl_x), int(self.hl_y), self.hl_angle, alpha_fade)
            _draw_rotated_sprite(canvas, self.half_right_sprite, int(self.hr_x), int(self.hr_y), self.hr_angle, alpha_fade)


# ==============================================================================
# BLENDING RAPIDO CON ARITMETICA ENTERA DE 16 BITS (SIN FLOATS EN MATRIZ GIGANTE)
# ==============================================================================
def _draw_rotated_sprite(canvas, sprite, cx, cy, angle, alpha_mul=1.0):
    sh, sw = sprite.shape[:2]
    M = cv2.getRotationMatrix2D((sw // 2, sh // 2), angle, 1.0)
    rotated = cv2.warpAffine(sprite, M, (sw, sh), flags=cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT, borderValue=(0,0,0,0))
    
    x1 = cx - sw // 2
    y1 = cy - sh // 2
    x2 = x1 + sw
    y2 = y1 + sh

    cx1 = max(0, x1)
    cy1 = max(0, y1)
    cx2 = min(CANVAS_W, x2)
    cy2 = min(CANVAS_H, y2)

    if cx1 >= cx2 or cy1 >= cy2:
        return

    sx1 = cx1 - x1
    sy1 = cy1 - y1
    sx2 = sx1 + (cx2 - cx1)
    sy2 = sy1 + (cy2 - cy1)

    roi = canvas[cy1:cy2, cx1:cx2]
    sprite_crop = rotated[sy1:sy2, sx1:sx2]

    # Alpha blending en uint16 con bit-shift: 3 veces mas rapido que float32
    alpha = sprite_crop[:, :, 3:4].astype(np.uint16)
    if alpha_mul < 1.0:
        alpha = (alpha * int(alpha_mul * 255)) >> 8
    inv_alpha = 255 - alpha
    roi[:] = ((roi.astype(np.uint16) * inv_alpha + sprite_crop[:, :, :3] * alpha) >> 8).astype(np.uint8)


# ==============================================================================
# DETECCION DE CORTE EXACTO RAYO - CIRCULO
# ==============================================================================
def check_line_circle_cut(p1, p2, circle_center, radius):
    x1, y1 = float(p1[0]), float(p1[1])
    x2, y2 = float(p2[0]), float(p2[1])
    cx, cy = float(circle_center[0]), float(circle_center[1])

    dx = x2 - x1
    dy = y2 - y1
    seg_len_sq = dx * dx + dy * dy
    if seg_len_sq < 1e-4:
        return math.hypot(x1 - cx, y1 - cy) <= radius

    t = ((cx - x1) * dx + (cy - y1) * dy) / seg_len_sq
    t = max(0.0, min(1.0, t))
    closest_x = x1 + t * dx
    closest_y = y1 + t * dy

    return math.hypot(closest_x - cx, closest_y - cy) <= radius


# ==============================================================================
# CLASE PRINCIPAL: FRUIT NINJA CV
# ==============================================================================
class FruitNinjaCV:
    def __init__(self):
        self.state = STATE_MENU
        self.score = 0
        self.high_score = 0
        self.lives = 3
        
        self.trail_left = deque(maxlen=14)
        self.trail_right = deque(maxlen=14)
        
        self.fruits = []
        self.particles = []
        self.floating_texts = []
        self.splatters = []  # Lista ligera de manchas activas
        
        self.last_wave_time = time.time()
        self.wave_interval = 2.3
        self.slice_history = []
        
        self.menu_watermelon = None
        
        print("[*] Generando sprites procedurales HD...")
        self.sprites = {
            "sandia": create_watermelon_sprites(),
            "naranja": create_orange_sprites(),
            "manzana": create_apple_sprites(),
            "platano": create_banana_sprites(),
            "limon": create_lemon_sprites(),
            "bomba": create_bomb_sprites()
        }
        self.fruit_types_pool = ["sandia", "naranja", "manzana", "platano", "limon"]
        
        self.detector = self._init_mediapipe_landmarker()
        self._setup_menu_watermelon()

    def _init_mediapipe_landmarker(self):
        if not MEDIAPIPE_AVAILABLE:
            print("[!] MediaPipe no esta disponible. Modo simulacion.")
            return None
            
        possible_paths = [
            os.path.join(os.path.dirname(__file__), "hand_landmarker.task"),
            os.path.join(os.getcwd(), "FruitNinjaCV", "hand_landmarker.task"),
            os.path.join(getattr(sys, '_MEIPASS', ''), "FruitNinjaCV", "hand_landmarker.task"),
            os.path.join(getattr(sys, '_MEIPASS', ''), "hand_landmarker.task"),
            os.path.join(os.getcwd(), "hand_landmarker.task"),
        ]
        model_path = None
        for p in possible_paths:
            if p and os.path.exists(p):
                model_path = p
                break
                
        if not model_path:
            print("[!] hand_landmarker.task no encontrado. Deteccion de mano deshabilitada.")
            return None
            
        print(f"[*] Cargando modelo MediaPipe HandLandmarker desde: {model_path}")
        try:
            with open(model_path, "rb") as mf:
                model_buffer = mf.read()

            base_options = mp_python.BaseOptions(model_asset_buffer=model_buffer)
            options = mp_vision.HandLandmarkerOptions(
                base_options=base_options,
                num_hands=2,
                min_hand_detection_confidence=0.5,
                min_hand_presence_confidence=0.5,
                min_tracking_confidence=0.5
            )
            detector = mp_vision.HandLandmarker.create_from_options(options)
            print("[+] MediaPipe HandLandmarker listo para rastreo en tiempo real.")
            return detector
        except Exception as e:
            print(f"[!] Error al crear HandLandmarker: {e}")
            return None

    def _setup_menu_watermelon(self):
        self.menu_watermelon = Fruit("sandia", CANVAS_W // 2, CANVAS_H // 2 + 50, 0, 0, self.sprites)
        self.menu_watermelon.v_rot = 1.2

    def spawn_wave(self):
        num_fruits = random.randint(1, 3)
        if self.score > 50:
            num_fruits = random.randint(2, 4)
        if self.score > 120:
            num_fruits = random.randint(3, 5)

        bomb_prob = 0.0
        if self.score >= 20:
            bomb_prob = 0.15
        if self.score >= 80:
            bomb_prob = 0.25

        for _ in range(num_fruits):
            x = random.randint(250, CANVAS_W - 250)
            y = CANVAS_H + 50
            vy = -random.uniform(23.5, 28.5)
            if x < CANVAS_W // 2:
                vx = random.uniform(1.5, 6.0)
            else:
                vx = -random.uniform(1.5, 6.0)

            if random.random() < bomb_prob:
                f_type = "bomba"
            else:
                f_type = random.choice(self.fruit_types_pool)

            self.fruits.append(Fruit(f_type, x, y, vx, vy, self.sprites))

    def add_splatter(self, cx, cy, color):
        self.splatters.append(Splatter(cx, cy, color))
        if len(self.splatters) > 8:
            self.splatters.pop(0)

    def spawn_fruit_particles(self, cx, cy, color, count=28):
        for _ in range(count):
            angle = random.uniform(0, 2 * math.pi)
            speed = random.uniform(4.0, 14.0)
            vx = math.cos(angle) * speed
            vy = math.sin(angle) * speed - random.uniform(2.0, 5.0)
            r = random.uniform(4, 8)
            self.particles.append(Particle(cx, cy, vx, vy, color, r, life=random.randint(22, 38)))

    def spawn_bomb_explosion(self, cx, cy):
        for _ in range(60):
            angle = random.uniform(0, 2 * math.pi)
            speed = random.uniform(6.0, 20.0)
            vx = math.cos(angle) * speed
            vy = math.sin(angle) * speed
            color = random.choice([(0, 140, 255), (0, 215, 255), (20, 20, 20), (255, 255, 255)])
            self.particles.append(Particle(cx, cy, vx, vy, color, random.uniform(5, 12), life=random.randint(25, 45), is_spark=True))

    def trigger_slice(self, p1, p2):
        now = time.time()
        fruits_sliced_this_swing = 0

        if self.state == STATE_MENU and self.menu_watermelon:
            if not self.menu_watermelon.sliced:
                if check_line_circle_cut(p1, p2, (self.menu_watermelon.x, self.menu_watermelon.y), self.menu_watermelon.radius):
                    self.menu_watermelon.slice_fruit()
                    self.spawn_fruit_particles(self.menu_watermelon.x, self.menu_watermelon.y, self.menu_watermelon.juice_color, count=35)
                    self.add_splatter(int(self.menu_watermelon.x), int(self.menu_watermelon.y), self.menu_watermelon.juice_color)
                    self.floating_texts.append(FloatingText(self.menu_watermelon.x, self.menu_watermelon.y - 40, "COMIENZA!", (0, 255, 255), 1.6))
                    self.state = STATE_PLAYING
                    self.score = 0
                    self.lives = 3
                    self.fruits.clear()
                    self.last_wave_time = now + 0.8
            return

        if self.state == STATE_GAME_OVER and self.menu_watermelon:
            if not self.menu_watermelon.sliced:
                if check_line_circle_cut(p1, p2, (self.menu_watermelon.x, self.menu_watermelon.y), self.menu_watermelon.radius):
                    self.menu_watermelon.slice_fruit()
                    self.spawn_fruit_particles(self.menu_watermelon.x, self.menu_watermelon.y, self.menu_watermelon.juice_color, count=35)
                    self.floating_texts.append(FloatingText(self.menu_watermelon.x, self.menu_watermelon.y - 40, "REINICIANDO!", (0, 255, 255), 1.6))
                    self.state = STATE_PLAYING
                    self.score = 0
                    self.lives = 3
                    self.fruits.clear()
                    self.last_wave_time = now + 0.8
            return

        if self.state == STATE_PLAYING:
            for fruit in self.fruits:
                if not fruit.sliced:
                    if check_line_circle_cut(p1, p2, (fruit.x, fruit.y), fruit.radius):
                        fruit.slice_fruit()
                        if fruit.is_bomb:
                            self.spawn_bomb_explosion(fruit.x, fruit.y)
                            self.lives = 0
                            self.state = STATE_GAME_OVER
                            self._setup_menu_watermelon()
                            if self.score > self.high_score:
                                self.high_score = self.score
                            self.floating_texts.append(FloatingText(fruit.x, fruit.y - 40, "BOOM!", (0, 0, 255), 2.0))
                            return
                        else:
                            fruits_sliced_this_swing += 1
                            self.score += 1
                            if self.score > self.high_score:
                                self.high_score = self.score
                            self.spawn_fruit_particles(fruit.x, fruit.y, fruit.juice_color)
                            self.add_splatter(int(fruit.x), int(fruit.y), fruit.juice_color)
                            self.floating_texts.append(FloatingText(fruit.x, fruit.y - 20, "+1", (0, 255, 255), 1.1))

            if fruits_sliced_this_swing > 0:
                self.slice_history.append((now, fruits_sliced_this_swing))
                recent_slices = [count for t, count in self.slice_history if now - t <= COMBO_TIME_WINDOW]
                total_in_combo = sum(recent_slices)
                if total_in_combo >= 2:
                    bonus = total_in_combo * 10
                    self.score += bonus
                    combo_text = f"COMBO x{total_in_combo}! +{bonus}"
                    if total_in_combo >= 4:
                        combo_text = f"MEGA COMBO x{total_in_combo}! +{bonus}"
                    self.floating_texts.append(FloatingText(CANVAS_W // 2 - 120, CANVAS_H // 2 - 100, combo_text, (0, 215, 255), 1.8))

    def process_hand_landmarks(self, frame_bgr):
        if self.detector is None:
            return

        # Para maximo rendimiento (60+ FPS), reescalar frame de deteccion
        det_w, det_h = 480, 270
        det_frame = cv2.resize(frame_bgr, (det_w, det_h))
        rgb_frame = cv2.cvtColor(det_frame, cv2.COLOR_BGR2RGB)
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)

        result = self.detector.detect(mp_image)
        now = time.time()

        left_found = False
        right_found = False

        if result.hand_landmarks:
            for h_idx, hand in enumerate(result.hand_landmarks):
                index_tip = hand[8]
                screen_x = int(index_tip.x * CANVAS_W)
                screen_y = int(index_tip.y * CANVAS_H)

                if h_idx == 0:
                    left_found = True
                    self._update_trail(self.trail_left, screen_x, screen_y, now)
                elif h_idx == 1:
                    right_found = True
                    self._update_trail(self.trail_right, screen_x, screen_y, now)

        if not left_found and len(self.trail_left) > 0:
            self.trail_left.popleft()
        if not right_found and len(self.trail_right) > 0:
            self.trail_right.popleft()

    def _update_trail(self, trail_deque, sx, sy, now):
        if len(trail_deque) > 0:
            prev_x, prev_y, _ = trail_deque[-1]
            speed = math.hypot(sx - prev_x, sy - prev_y)
            if speed >= MIN_CUT_SPEED:
                self.trigger_slice((prev_x, prev_y), (sx, sy))
                for _ in range(2):
                    self.particles.append(Particle(sx, sy, random.uniform(-3, 3), random.uniform(-3, 3), (255, 255, 255), random.uniform(2, 5), life=10, is_spark=True))
        trail_deque.append((sx, sy, now))

    def draw_katana_blade(self, canvas, trail_deque, blade_color=(255, 220, 50)):
        pts = list(trail_deque)
        n = len(pts)
        if n < 2:
            return

        for i in range(n - 1):
            p1 = (pts[i][0], pts[i][1])
            p2 = (pts[i + 1][0], pts[i + 1][1])
            progress = (i + 1) / n
            thickness = max(2, int(15 * progress))
            cv2.line(canvas, p1, p2, blade_color, thickness, cv2.LINE_AA)

        for i in range(n - 1):
            p1 = (pts[i][0], pts[i][1])
            p2 = (pts[i + 1][0], pts[i + 1][1])
            progress = (i + 1) / n
            thickness = max(1, int(4 * progress))
            cv2.line(canvas, p1, p2, (255, 255, 255), thickness, cv2.LINE_AA)

        tip_x, tip_y = pts[-1][0], pts[-1][1]
        cv2.circle(canvas, (tip_x, tip_y), 10, blade_color, -1, cv2.LINE_AA)
        cv2.circle(canvas, (tip_x, tip_y), 4, (255, 255, 255), -1, cv2.LINE_AA)

    def draw_hud(self, canvas, fps):
        # Barra superior translucida rapida (solo procesa ROI de 85px)
        top_roi = canvas[0:85, :].copy()
        cv2.rectangle(top_roi, (0, 0), (CANVAS_W, 85), (20, 20, 24), -1)
        cv2.line(top_roi, (0, 84), (CANVAS_W, 84), (45, 180, 255), 2)
        cv2.addWeighted(top_roi, 0.75, canvas[0:85, :], 0.25, 0, canvas[0:85, :])

        # Marcador de Puntos y Record
        score_text = f"PUNTOS: {self.score}"
        cv2.putText(canvas, score_text, (40, 58), cv2.FONT_HERSHEY_DUPLEX, 1.4, (0, 0, 0), 5, cv2.LINE_AA)
        cv2.putText(canvas, score_text, (40, 58), cv2.FONT_HERSHEY_DUPLEX, 1.4, (0, 215, 255), 2, cv2.LINE_AA)

        record_text = f"RECORD: {self.high_score}"
        cv2.putText(canvas, record_text, (380, 56), cv2.FONT_HERSHEY_DUPLEX, 1.0, (180, 180, 180), 2, cv2.LINE_AA)

        # 3 Vidas
        lives_x = CANVAS_W - 200
        for i in range(3):
            lx = lives_x + i * 55
            ly = 45
            if i < self.lives:
                cv2.circle(canvas, (lx, ly), 18, (40, 190, 60), -1, cv2.LINE_AA)
                cv2.circle(canvas, (lx, ly), 18, (255, 255, 255), 2, cv2.LINE_AA)
                cv2.line(canvas, (lx - 7, ly), (lx - 2, ly + 6), (255, 255, 255), 3, cv2.LINE_AA)
                cv2.line(canvas, (lx - 2, ly + 6), (lx + 8, ly - 6), (255, 255, 255), 3, cv2.LINE_AA)
            else:
                cv2.circle(canvas, (lx, ly), 18, (30, 30, 180), -1, cv2.LINE_AA)
                cv2.circle(canvas, (lx, ly), 18, (255, 255, 255), 2, cv2.LINE_AA)
                cv2.line(canvas, (lx - 8, ly - 8), (lx + 8, ly + 8), (255, 255, 255), 3, cv2.LINE_AA)
                cv2.line(canvas, (lx + 8, ly - 8), (lx - 8, ly + 8), (255, 255, 255), 3, cv2.LINE_AA)

        # Barra inferior rapida (solo ROI de 45px)
        info_roi = canvas[CANVAS_H - 45:CANVAS_H, :].copy()
        cv2.rectangle(info_roi, (0, 0), (CANVAS_W, 45), (15, 15, 18), -1)
        cv2.addWeighted(info_roi, 0.70, canvas[CANVAS_H - 45:CANVAS_H, :], 0.30, 0, canvas[CANVAS_H - 45:CANVAS_H, :])
        
        info_text = "Modo: MediaPipe AI HandLandmarker (Inmune a fondos, cuadros y rostros) | Espada: Dedo Indice"
        cv2.putText(canvas, info_text, (30, CANVAS_H - 16), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (210, 220, 230), 1, cv2.LINE_AA)

        controls_text = f"FPS: {int(fps)} | 'R' Reiniciar | 'F' Pantalla Completa | 'ESC' Salir"
        cv2.putText(canvas, controls_text, (CANVAS_W - 620, CANVAS_H - 16), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 215, 255), 1, cv2.LINE_AA)

    def draw_menu(self, canvas):
        title = "FRUIT NINJA CV"
        sub = "Playground de Vision Artificial - Corte con Mano AI (60 FPS)"
        cv2.putText(canvas, title, (CANVAS_W // 2 - 320, 260), cv2.FONT_HERSHEY_DUPLEX, 2.5, (0, 0, 0), 8, cv2.LINE_AA)
        cv2.putText(canvas, title, (CANVAS_W // 2 - 320, 260), cv2.FONT_HERSHEY_DUPLEX, 2.5, (0, 215, 255), 4, cv2.LINE_AA)
        cv2.putText(canvas, sub, (CANVAS_W // 2 - 420, 320), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (230, 230, 240), 2, cv2.LINE_AA)

        if self.menu_watermelon:
            self.menu_watermelon.angle = (self.menu_watermelon.angle + 1.2) % 360
            pulse = 12 + int(6 * math.sin(time.time() * 5))
            cv2.circle(canvas, (int(self.menu_watermelon.x), int(self.menu_watermelon.y)), self.menu_watermelon.radius + pulse, (0, 220, 255), 3, cv2.LINE_AA)
            self.menu_watermelon.draw(canvas)

        call_action = "CORTA LA SANDIA CON TU DEDO PARA COMENZAR"
        cv2.putText(canvas, call_action, (CANVAS_W // 2 - 390, CANVAS_H // 2 + 180), cv2.FONT_HERSHEY_DUPLEX, 1.1, (0, 0, 0), 5, cv2.LINE_AA)
        cv2.putText(canvas, call_action, (CANVAS_W // 2 - 390, CANVAS_H // 2 + 180), cv2.FONT_HERSHEY_DUPLEX, 1.1, (255, 255, 255), 2, cv2.LINE_AA)

    def draw_game_over(self, canvas):
        banner_roi = canvas[180:420, :].copy()
        cv2.rectangle(banner_roi, (0, 0), (CANVAS_W, 240), (20, 20, 45), -1)
        cv2.addWeighted(banner_roi, 0.85, canvas[180:420, :], 0.15, 0, canvas[180:420, :])

        go_text = "FIN DEL JUEGO"
        cv2.putText(canvas, go_text, (CANVAS_W // 2 - 280, 290), cv2.FONT_HERSHEY_DUPLEX, 2.6, (0, 0, 0), 8, cv2.LINE_AA)
        cv2.putText(canvas, go_text, (CANVAS_W // 2 - 280, 290), cv2.FONT_HERSHEY_DUPLEX, 2.6, (40, 50, 240), 4, cv2.LINE_AA)

        score_res = f"Puntaje Final: {self.score}   |   Mejor Puntaje: {self.high_score}"
        cv2.putText(canvas, score_res, (CANVAS_W // 2 - 350, 360), cv2.FONT_HERSHEY_DUPLEX, 1.2, (255, 255, 255), 2, cv2.LINE_AA)

        if self.menu_watermelon:
            self.menu_watermelon.angle = (self.menu_watermelon.angle + 1.2) % 360
            pulse = 12 + int(6 * math.sin(time.time() * 5))
            cv2.circle(canvas, (int(self.menu_watermelon.x), int(self.menu_watermelon.y)), self.menu_watermelon.radius + pulse, (0, 220, 255), 3, cv2.LINE_AA)
            self.menu_watermelon.draw(canvas)

        call_action = "CORTA LA SANDIA PARA REINICIAR (O PULSA 'R')"
        cv2.putText(canvas, call_action, (CANVAS_W // 2 - 390, CANVAS_H // 2 + 180), cv2.FONT_HERSHEY_DUPLEX, 1.1, (0, 0, 0), 5, cv2.LINE_AA)
        cv2.putText(canvas, call_action, (CANVAS_W // 2 - 390, CANVAS_H // 2 + 180), cv2.FONT_HERSHEY_DUPLEX, 1.1, (0, 215, 255), 2, cv2.LINE_AA)

    def run(self):
        print("[*] Inicializando camara multihilo...")
        camera = ThreadedCamera(0, width=1280, height=720)

        window_name = "Fruit Ninja CV - Vision Artificial (Full HD)"
        cv2.namedWindow(window_name, cv2.WINDOW_NORMAL)
        cv2.resizeWindow(window_name, 1600, 900)

        mouse_down = False
        def _on_mouse(event, mx, my, flags, param):
            nonlocal mouse_down
            if event == cv2.EVENT_LBUTTONDOWN:
                mouse_down = True
                self._update_trail(self.trail_left, mx, my, time.time())
            elif event == cv2.EVENT_MOUSEMOVE and mouse_down:
                self._update_trail(self.trail_left, mx, my, time.time())
            elif event == cv2.EVENT_LBUTTONUP:
                mouse_down = False

        cv2.setMouseCallback(window_name, _on_mouse)

        prev_time = time.time()
        fps = 60.0
        is_fullscreen = False

        print("[+] Fruit Ninja CV listo. Rendimiento optimizado a 60 FPS!")
        print("    Controles: 'F': Pantalla completa | 'R': Reiniciar | 'Q' / 'ESC': Salir")

        while True:
            ret, frame = camera.read()
            now = time.time()
            dt = now - prev_time
            if dt > 0:
                fps = 0.92 * fps + 0.08 * (1.0 / dt)
            prev_time = now

            if not ret or frame is None:
                frame = np.zeros((720, 1280, 3), dtype=np.uint8)
                cv2.putText(frame, "CAMARA NO DETECTADA - MODO SIMULACION", (300, 360), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 0, 255), 2)

            # Efecto espejo
            frame = cv2.flip(frame, 1)

            # Escalar camara al lienzo Full HD 1920x1080
            canvas = cv2.resize(frame, (CANVAS_W, CANVAS_H), interpolation=cv2.INTER_LINEAR)

            # Atenuar camara rapido con SIMD
            canvas = cv2.convertScaleAbs(canvas, alpha=0.88)

            # 1. Procesar deteccion de mano con MediaPipe (Inmune a fondos y cuadros)
            self.process_hand_landmarks(frame)

            # 2. Actualizar y dibujar manchas ligeras
            self.splatters = [sp for sp in self.splatters if sp.update()]
            for sp in self.splatters:
                sp.draw(canvas)

            # 3. Logica segun Estado
            if self.state == STATE_MENU:
                self.draw_menu(canvas)
            elif self.state == STATE_PLAYING:
                if now - self.last_wave_time >= self.wave_interval:
                    self.spawn_wave()
                    self.last_wave_time = now
                    self.wave_interval = max(1.4, 2.3 - (self.score * 0.01))

                surviving_fruits = []
                for fruit in self.fruits:
                    alive = fruit.update()
                    if alive:
                        fruit.draw(canvas)
                        surviving_fruits.append(fruit)
                    else:
                        if not fruit.sliced and not fruit.is_bomb:
                            self.lives -= 1
                            self.floating_texts.append(FloatingText(fruit.x, CANVAS_H - 80, "X", (0, 0, 255), 2.2))
                            if self.lives <= 0:
                                self.state = STATE_GAME_OVER
                                self._setup_menu_watermelon()
                                if self.score > self.high_score:
                                    self.high_score = self.score
                self.fruits = surviving_fruits

            elif self.state == STATE_GAME_OVER:
                self.draw_game_over(canvas)

            # 4. Actualizar y dibujar particulas
            self.particles = [p for p in self.particles if p.update()]
            for p in self.particles:
                p.draw(canvas)

            # 5. Actualizar y dibujar textos flotantes
            self.floating_texts = [ft for ft in self.floating_texts if ft.update()]
            for ft in self.floating_texts:
                ft.draw(canvas)

            # 6. Dibujar espadas katana en las manos detectadas
            self.draw_katana_blade(canvas, self.trail_left, blade_color=(255, 215, 40))
            self.draw_katana_blade(canvas, self.trail_right, blade_color=(40, 220, 255))

            # 7. Renderizar HUD
            self.draw_hud(canvas, fps)

            # Mostrar ventana
            cv2.imshow(window_name, canvas)

            # Gestion de teclas
            key = cv2.waitKey(1) & 0xFF
            if key == 27 or key == ord('q') or key == ord('Q'):
                break
            elif key == ord('f') or key == ord('F'):
                is_fullscreen = not is_fullscreen
                if is_fullscreen:
                    cv2.setWindowProperty(window_name, cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)
                else:
                    cv2.setWindowProperty(window_name, cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_NORMAL)
            elif key == ord('r') or key == ord('R'):
                self.score = 0
                self.lives = 3
                self.fruits.clear()
                self.particles.clear()
                self.floating_texts.clear()
                self.splatters.clear()
                self.state = STATE_PLAYING
                self.last_wave_time = now

        camera.release()
        cv2.destroyAllWindows()
        print("[+] Fruit Ninja CV cerrado correctamente.")


if __name__ == "__main__":
    game = FruitNinjaCV()
    game.run()
