#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "main.h"
#include "st7796_lcd.h"
#include "peripherals.h"
#include "sprites.h"

// --- Definiciones de Juego ---
#define FILAS_MARCIANOS 5
#define COLS_MARCIANOS 6
#define TOTAL_MARCIANOS (FILAS_MARCIANOS * COLS_MARCIANOS)
#define MAX_BALAS 30
#define MAX_BALAS_ENEMIGAS 5

// --- Estructuras ---
typedef enum {
    ESTADO_MENU,
    ESTADO_SELECCION,
    ESTADO_JUEGO,
    ESTADO_VICTORIA,
    ESTADO_DERROTA
} EstadoJuego;

typedef struct {
    int x, y;
    int tipo;
    int ancho, alto;
    uint8_t vivo;
} Marciano;

typedef struct {
    int x, y;
    uint8_t activa;
} Bala;

// --- Funciones Principales ---
void Game_Init(void);
void Game_Update(void); // Esta es la única función que llamamos en el while(1) del main
void Game_TriggerFire(void);
#endif
