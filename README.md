# 🚀 STM32 Space Invaders - Arcade Edition

Bienvenido a nuestra versión del clásico **Space Invaders**, desarrollada desde cero en **C** para microcontroladores **STM32**. Este proyecto combina programación de bajo nivel, gestión de hardware y lógica de videojuegos en tiempo real.

## 🎮 Descripción del Juego

El objetivo es defender la Tierra de una invasión alienígena progresiva. El jugador controla una nave espacial que puede moverse lateralmente y disparar proyectiles, gestionando el calentamiento del arma y esquivando el fuego enemigo.

### Características Principales:
* **Sistema de Menú Interactivo:** Selección de nave con efectos visuales de zoom.
* **3 Tipos de Naves:** Cada una con características visuales únicas (Normal, Delta, Interceptor).
* **Mecánica de Calentamiento:** El arma se bloquea si disparas demasiado rápido ("Overheat").
* **Sistema de Oleadas (Waves):**
    * **Oleada 1:** Entrenamiento. Velocidad normal.
    * **Oleada 2:** Aumento de velocidad. Los enemigos Morados disparan.
    * **Oleada 3:** Velocidad alta. Los enemigos Verdes y Morados disparan coordinados.
* **Hardware Feedback:** Vidas representadas con LEDs físicos y sonido mediante Buzzer pasivo (PWM).

---

## 🛠️ Hardware Utilizado

* **Microcontrolador:** STM32 (BlackPill F411).
* **Pantalla:** LCD TFT (Driver ST7796/ILI9341) vía SPI.
* **Controles:**
    * Joystick Analógico (Movimiento) vía ADC.
    * Botones Físicos (Disparo, Reset, Start) vía GPIO.
* **Audio:** Buzzer Pasivo (PWM con Timers).
* **Indicadores:** 3 LEDs externos para el contador de vidas.

---

## 👥 Autores y Contribuciones

Este proyecto ha sido desarrollado en equipo, dividiendo las tareas de ingeniería hardware y desarrollo software:

### 👨‍💻 Andrés - Arquitectura y Hardware
* **Diseño Estructural:** Planteamiento y creación del **Diagrama de Estados** (Máquina de Estados Finitos) que gobierna el flujo del programa.
* **Lógica Core:** Implementación de la base lógica del juego.
* **Hardware Interface:** Configuración del **IOC** (CubeMX), conexión física de la pantalla, lectura del Joystick (ADC) y gestión de botones.
* **Controladores:** Puesta en marcha de los drivers de bajo nivel.

### 👩‍💻 Marina - Gameplay y Experiencia de Usuario (UX)
* **Mecánicas de Juego Avanzadas:** Implementación del sistema de **3 Oleadas** con dificultad progresiva (aumento de velocidad).
* **Inteligencia Artificial:** Desarrollo de la lógica de disparo enemigo en las oleadas 2 y 3.
* **Hardware Feedback:** Integración de los **LEDs físicos** para la gestión de vidas en tiempo real.
* **Estados de Juego:** Creación y lógica de las pantallas de **VICTORIA** y **DERROTA (Game Over)**.
* **Mejoras Visuales:** Rediseño del menú de selección de nave (Efecto Zoom dinámico y optimización de renderizado).
* **Documentación:** Elaboración de este README.

---

## 🕹️ Cómo Jugar

1.  **Menú:** Usa el Joystick para elegir tu nave. Pulsa **START (PB0)** para confirmar.
2.  **Juego:**
    * **Joystick:** Mover Izquierda/Derecha.
    * **Botón Disparo (PB1):** Disparar (¡Cuidado con el calentamiento!).
    * **Botón Reset (PB2):** Volver al menú en cualquier momento.
3.  **Objetivo:** Elimina a todos los marcianos para avanzar de oleada. Si superas la Oleada 3, ganas el juego.
4.  **Derrota:** Pierdes si te quedas sin vidas (0 LEDs encendidos) o si un marciano toca tu nave.

---

## 📸 Galería

*Proyecto realizado para la asignatura de Sistemas Electrónicos Digitales.*

