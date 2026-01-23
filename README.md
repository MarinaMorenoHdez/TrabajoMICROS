# 🚀 STM32 Space Invaders - Arcade Edition

Bienvenido a nuestra versión del clásico **Space Invaders**, desarrollada desde cero en **C** para microcontroladores **STM32**. Este proyecto combina programación de bajo nivel, gestión de hardware y lógica de videojuegos en tiempo real.

## 🎮 Descripción del Juego

El objetivo es defender la Tierra de una invasión alienígena progresiva. El jugador controla una nave espacial seleccionable que puede moverse lateralmente y disparar proyectiles, gestionando el calentamiento del arma y esquivando el fuego enemigo.

### Características Principales:
* **Sistema de Menú Interactivo:** Selección de nave con efectos visuales de zoom.
* **3 Tipos de Naves:** Cada una con características visuales y de juego únicas (Normal, Delta, Interceptor).
* **Mecánica de Calentamiento:** El arma se bloquea si disparas demasiado rápido ("Overheat").
* **Sistema de Oleadas (Waves):**
    * **Oleada 1:** Velocidad normal. Fuego normal.
    * **Oleada 2:** Aumento de velocidad. Fuego alto.
    * **Oleada 3:** Velocidad alta. Fuego muy alto.
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

### 👩‍💻 Sergio Llana - Optimización de código y Gameplay
* **Refactorización y Modularidad:** Migración del código monolítico (main.c) a una arquitectura modular profesional (game_engine.c, sprites.c, peripherals.c, etc), eliminando "superfunciones" y mejorando la mantenibilidad.
* **Game Engine:** Implementación de lógica limpia, organizada en funciones y ordenada.
* **Oleadas enemigas** con dificultad aumentada progresiva. Mayor velocidad y mayor fuego enemigo.
* **Propiedades In-Game** diferenciadas según el tipo de nave.
* **Interfaz (HUD) Avanzada:** Desarrollo de la barra de **sobrecalentamiento dinámica** y visualización del **SCORE** en tiempo real y en pantallas finales.
* **Mecánicas Nuevas:** Implementación del sistema de **Escudo de Energía** (Power-up aleatorio con visualización ovalada).
* **Audio Dinámico:** Composición e implementación de melodías de **Victoria** y **Derrota** sincronizadas con el estado del juego.

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
![WhatsApp Image 2026-01-22 at 16 10 01 (5)](https://github.com/user-attachments/assets/fac5b5e3-c3a7-44fa-beb1-0407d2e9de61)
![WhatsApp Image 2026-01-22 at 16 10 01 (4)](https://github.com/user-attachments/assets/3d5b7205-336a-441b-b6cc-060544432e74)
![WhatsApp Image 2026-01-22 at 16 10 00 (5)](https://github.com/user-attachments/assets/c404a35f-6d01-40a7-9b82-b1a3fe8d4722)
![WhatsApp Image 2026-01-22 at 16 10 00 (4)](https://github.com/user-attachments/assets/5916328b-2c3c-4a5b-9053-2da374d3b14f)

*Proyecto realizado para la asignatura de Sistemas Electrónicos Digitales.*

