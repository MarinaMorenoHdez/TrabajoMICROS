#include "game_engine.h"
#include "st7796_lcd.h"
#include "peripherals.h"
#include "sprites.h"
#include <stdio.h>
#include <stdlib.h>



// --- CONSTANTES ---
#define MAX_CALOR 1200
#define CALOR_POR_DISPARO 300
#define ENFRIAMIENTO_FRAME 33
#define COOLDOWN_DISPARO 150
const uint32_t TIEMPO_ENTRE_FRAMES = 33;

// --- VARIABLES DE ESTADO (Globales al archivo) ---
static EstadoJuego estadoActual = ESTADO_MENU;
static uint32_t ultimoTiempoRefresco = 0;
static uint32_t ultimoMovMarcianos = 0;
static uint32_t ultimoDisparo = 0;
static uint32_t ultimoDisparoEnemigo = 0;
static int puntuacion = 0;
static uint32_t cooldownEnemigo = 1200;
static volatile uint8_t flagDisparoInterrupcion = 0;

// --- VARIABLES JUGADOR ---
static int naveX = 152;
static int naveY = 420;
static int oldNaveX = 152;
static int vidas = 3;
static int naveSeleccionada = 0;
static int anchoNave = 16;
static int altoNave = 8;
static int calorArma = 0;
static uint8_t armaSobrecalentada = 0;
static int velocidadNave = 8;
static int cooldownDisparoNave = 150;

// --- VARIABLES ESCUDO ---
static struct {
    int x, y;
    int activa; // 1 = cayendo, 0 = no existe
} itemEscudo;
static uint8_t tieneEscudo = 0;       // 1 = jugador protegido
static uint32_t tiempoFinEscudo = 0;  // Momento en que se acaba el escudo

// --- VARIABLES ENEMIGOS Y OBJETOS ---
static Marciano marcianos[TOTAL_MARCIANOS];
static Bala balas[MAX_BALAS];
static Bala balasEnemigas[MAX_BALAS_ENEMIGAS];
static int marcianosVivos = 0;
static int oleada = 1;
static int velocidadMarcianos = 4;
static int direccionMarcianos = 1;

// =========================================================
// PROTOTIPOS DE FUNCIONES PRIVADAS
// =========================================================
static void InicializarMarcianos(void);
static void ResetJuego(void);

// Funciones de Lógica de Estados
static void Update_Estado_Menu(void);
static void Update_Estado_Seleccion(void);
static void Update_Estado_Juego(void);
static void Update_Estado_Victoria(void);
static void Update_Estado_Derrota(void);

// Sub-funciones del Juego
static void Juego_GestionarCalor(void);
static void Juego_MoverNave(void);
static void Juego_DispararJugador(void);
static void Juego_ActualizarBalasJugador(void);
static uint8_t Juego_ActualizarBalasEnemigas(void); // Retorna 1 si jugador muere
static uint8_t Juego_MoverMarcianos(void);          // Retorna 1 si jugador muere (invasión)
static void Juego_DibujarHUD(void);
static void Juego_GestionarPowerUp(void);
static void Juego_GestionarNivelYVidas(uint8_t muerteJugador);
static void DibujarElipse(int x0, int y0, int rx, int ry, uint16_t color);

// Auxiliar de interfaz
static void DibujarNaveIcono(int id, int x, int y, int zoom);

// =========================================================
// IMPLEMENTACIÓN
// =========================================================

static void InicializarMarcianos() {
    int idx = 0;
    direccionMarcianos = 1;
    marcianosVivos = TOTAL_MARCIANOS;
    for(int fila=0; fila < FILAS_MARCIANOS; fila++) {
        for(int col=0; col < COLS_MARCIANOS; col++) {
            marcianos[idx].vivo = 1;
            marcianos[idx].x = 30 + (col * 45);
            marcianos[idx].y = 60 + (fila * 40);
            marcianos[idx].tipo = fila;
            if(fila <= 2) {
                marcianos[idx].ancho = 16; marcianos[idx].alto = 11;
            } else if(fila == 3) {
                marcianos[idx].ancho = 18; marcianos[idx].alto = 12;
            } else {
                marcianos[idx].ancho = 24; marcianos[idx].alto = 16;
            }
            idx++;
        }
    }
}

static void ResetJuego() {
    vidas = 3;
    oleada = 1;
    calorArma = 0;
    puntuacion = 0;
    cooldownEnemigo = 1200;
    armaSobrecalentada = 0;
    naveX = 152;
    oldNaveX = -1;

    // Reset Escudo
    itemEscudo.activa = 0;
    tieneEscudo = 0;

    if (naveSeleccionada == 1) {      // DELTA
        anchoNave = 20; altoNave = 10;
        velocidadNave = 14; cooldownDisparoNave = 200;
    } else if (naveSeleccionada == 2) { // INTERCEPTOR
        anchoNave = 16; altoNave = 10;
        velocidadNave = 4; cooldownDisparoNave = 80;
    } else {                         // NORMAL
        anchoNave = 16; altoNave = 8;
        velocidadNave = 9; cooldownDisparoNave = 180;
    }
    InicializarMarcianos();
    for(int i=0; i<MAX_BALAS; i++) balas[i].activa = 0;
    for(int i=0; i<MAX_BALAS_ENEMIGAS; i++) balasEnemigas[i].activa = 0;
    Peripherals_UpdateLivesLEDs(vidas);
}

static void LimpiarTodasLasBalas(void) {
    // Desactivar balas del jugador
    for(int i=0; i<MAX_BALAS; i++) {
        balas[i].activa = 0;
    }
    // Desactivar balas enemigas
    for(int i=0; i<MAX_BALAS_ENEMIGAS; i++) {
        balasEnemigas[i].activa = 0;
    }
}

// --- FUNCIONES PÚBLICAS ---

void Game_Init(void) {
    ST7796_Init();
    ST7796_Fill(0x0000);
    ResetJuego();
    Peripherals_UpdateLivesLEDs(vidas);
    Peripherals_Beep(1500, 100);
}

void Game_TriggerFire(void) {
    flagDisparoInterrupcion = 1;
}

void Game_Update(void) {
    uint32_t tiempoActual = HAL_GetTick();
    if (tiempoActual - ultimoTiempoRefresco < TIEMPO_ENTRE_FRAMES) return;
    ultimoTiempoRefresco = tiempoActual;

    Peripherals_CheckBuzzerTimeout();

    switch (estadoActual) {
        case ESTADO_MENU:      Update_Estado_Menu();      break;
        case ESTADO_SELECCION: Update_Estado_Seleccion(); break;
        case ESTADO_JUEGO:     Update_Estado_Juego();     break;
        case ESTADO_VICTORIA:  Update_Estado_Victoria();  break;
        case ESTADO_DERROTA:   Update_Estado_Derrota();   break;
    }
}

// =========================================================
// SUB-FUNCIONES DE ESTADOS
// =========================================================

static void Update_Estado_Menu(void) {
    static uint8_t pintado = 0;
    if (!pintado) {
        ST7796_Fill(0x0000);
        ST7796_WriteString(40, 100, "SPACE", 0x07E0, 0x0000, 5);
        ST7796_WriteString(20, 160, "INVADERS", 0xF800, 0x0000, 5);
        ST7796_WriteString(60, 350, "PULSA START", 0xFFE0, 0x0000, 2);
        pintado = 1;
    }
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
        Peripherals_Beep(1000, 100);
        ST7796_Fill(0x0000);
        pintado = 0;
        estadoActual = ESTADO_SELECCION;
    }
}

static void DibujarNaveIcono(int id, int x, int y, int zoom) {
    const uint16_t* sprite;
    int w, h;

    if (id == 0) {
        sprite = nave_16x8; w = 16; h = 8;
    } else if (id == 1) {
        sprite = nave_delta_20x10; w = 20; h = 10;
    } else { // id == 2
        sprite = nave_interceptor_16x10; w = 16; h = 10;
    }
    ST7796_DrawBitmapZoom(x, y, w, h, sprite, zoom);
}

static void Update_Estado_Seleccion(void) {
    static int seleccionAnterior = -1;
    uint32_t joyValSel = Peripherals_ReadJoystick();

    if (joyValSel > 3000) {
        naveSeleccionada++;
        HAL_Delay(150);
    }
    else if (joyValSel < 1000) {
        naveSeleccionada--;
        HAL_Delay(150);
    }

    if (naveSeleccionada > 2) naveSeleccionada = 0;
    if (naveSeleccionada < 0) naveSeleccionada = 2;

    if (naveSeleccionada != seleccionAnterior) {
        ST7796_DrawRect(0, 150, 320, 250, 0x0000);

        ST7796_WriteString(80, 40, "SELECCIONA", 0xFFFF, 0x0000, 2);
        ST7796_WriteString(100, 70, "TU NAVE", 0xFFFF, 0x0000, 2);

        int idxIzquierda = (naveSeleccionada == 0) ? 2 : naveSeleccionada - 1;
        int idxDerecha   = (naveSeleccionada == 2) ? 0 : naveSeleccionada + 1;

        DibujarNaveIcono(idxIzquierda, 40, 200, 2);
        DibujarNaveIcono(idxDerecha, 250, 200, 2);

        ST7796_WriteString(10, 200, "<", 0x7BEF, 0x0000, 2);
        ST7796_WriteString(290, 200, ">", 0x7BEF, 0x0000, 2);

        if (naveSeleccionada == 0) {
            ST7796_DrawBitmapZoom(128, 180, 16, 8, nave_16x8, 4);
            ST7796_WriteString(100, 260, "NORMAL", 0x07E0, 0x0000, 2);
            ST7796_WriteString(60, 290, "CALOR:     ***", 0xFFFF, 0x0000, 1);
            ST7796_WriteString(60, 310, "VELOCIDAD: ***", 0xFFFF, 0x0000, 1);
            ST7796_WriteString(60, 330, "FUEGO:     ***", 0xFFFF, 0x0000, 1);
        } else if (naveSeleccionada == 1) {
            ST7796_DrawBitmapZoom(120, 180, 20, 10, nave_delta_20x10, 4);
            ST7796_WriteString(110, 260, "DELTA", 0x07E0, 0x0000, 2);
            ST7796_WriteString(60, 290, "CALOR:     ***", 0xFFFF, 0x0000, 1);
            ST7796_WriteString(60, 310, "VELOCIDAD: *****", 0xFFFF, 0x0000, 1);
            ST7796_WriteString(60, 330, "FUEGO:     ***", 0xFFFF, 0x0000, 1);
        } else {
            ST7796_DrawBitmapZoom(128, 180, 16, 10, nave_interceptor_16x10, 4);
            ST7796_WriteString(80, 260, "INTERCEPTOR", 0x07E0, 0x0000, 2);
            ST7796_WriteString(60, 290, "CALOR:     *****", 0xFFFF, 0x0000, 1);
            ST7796_WriteString(60, 310, "VELOCIDAD: **", 0xFFFF, 0x0000, 1);
            ST7796_WriteString(60, 330, "FUEGO:     *****", 0xFFFF, 0x0000, 1);
        }
        seleccionAnterior = naveSeleccionada;
    }

    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
        Peripherals_Beep(1200, 100);
        ST7796_Fill(0x0000);
        seleccionAnterior = -1;
        ResetJuego();

        const uint16_t* s = (naveSeleccionada==0)?nave_16x8:(naveSeleccionada==1)?nave_delta_20x10:nave_interceptor_16x10;
        ST7796_DrawBitmap(naveX, naveY, anchoNave, altoNave, s);
        oldNaveX = naveX;

        estadoActual = ESTADO_JUEGO;
    }
    else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
        Peripherals_Beep(800, 100);
        ST7796_Fill(0x0000);
        seleccionAnterior = -1;
        estadoActual = ESTADO_MENU;
        HAL_Delay(200);
    }
}

static void Update_Estado_Juego(void) {
    uint8_t muerteJugador = 0;

    Juego_GestionarCalor();
    Juego_MoverNave();
    Juego_DispararJugador();
    Juego_ActualizarBalasJugador();
    Juego_DibujarHUD();
    Juego_GestionarPowerUp();

    if (Juego_ActualizarBalasEnemigas()) muerteJugador = 1;
    if (Juego_MoverMarcianos()) muerteJugador = 1;

    Juego_GestionarNivelYVidas(muerteJugador);

    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
        Peripherals_Beep(800, 100);
        ST7796_Fill(0x0000);
        ResetJuego();
        estadoActual = ESTADO_MENU;
        HAL_Delay(200);
    }
}

// =========================================================
// FUNCIONES AUXILIARES DE JUEGO
// =========================================================

static void Juego_GestionarCalor(void) {
    if (calorArma > 0) {
        calorArma -= ENFRIAMIENTO_FRAME;
        if (calorArma < 0) calorArma = 0;
    }
    if (armaSobrecalentada && calorArma == 0) armaSobrecalentada = 0;
}

// --- GESTIÓN DEL ESCUDO ---
static void Juego_GestionarPowerUp(void) {
    // 1. Gestión del tiempo del escudo activo
    if (tieneEscudo) {
        if (HAL_GetTick() > tiempoFinEscudo) {
            tieneEscudo = 0; // Se acabó el tiempo

           // Limpia el contorno Cian
            ST7796_DrawRect(naveX - 8, naveY - 6, anchoNave + 16, altoNave + 12, 0x0000);
            const uint16_t* s = (naveSeleccionada == 0) ? nave_16x8 : (naveSeleccionada == 1) ? nave_delta_20x10 : nave_interceptor_16x10;
            ST7796_DrawBitmap(naveX, naveY, anchoNave, altoNave, s);
            Peripherals_PlayTone(400, 100); // Sonido "Escudo Apagado"
        }
    }

    // 2. Spawn aleatorio
    if (!itemEscudo.activa && !tieneEscudo) {
        if ((rand() % 500) == 0) { // Probabilidad baja
            itemEscudo.activa = 1;
            itemEscudo.x = 20 + (rand() % 280);
            itemEscudo.y = 40;
        }
    }

    // 3. Movimiento y Dibujado del Ítem
    if (itemEscudo.activa) {
        // BORRAMOS ESTELA
        ST7796_DrawRect(itemEscudo.x, itemEscudo.y, 12, 12, 0x0000);

        itemEscudo.y += 3; // Caída

        if (itemEscudo.y > 480) {
            itemEscudo.activa = 0;
        } else {
            // Colisión con Nave
            if (itemEscudo.x + 8 >= naveX && itemEscudo.x <= naveX + anchoNave &&
                itemEscudo.y + 8 >= naveY && itemEscudo.y <= naveY + altoNave) {

                itemEscudo.activa = 0;
                tieneEscudo = 1;
                tiempoFinEscudo = HAL_GetTick() + 5000; // 5 Segundos
                Peripherals_PlayTone(2000, 100); // Sonido PowerUp
            } else {
                // Dibujar ítem (Cuadrado Azul Cian)
                ST7796_DrawRect(itemEscudo.x, itemEscudo.y, 8, 8, 0x07FF); // Relleno
                ST7796_WriteString(itemEscudo.x+2, itemEscudo.y, "S", 0x0000, 0x07FF, 1);
            }
        }
    }
}
// Algoritmo dibujar una elipse hueca para escudo

static void DibujarElipse(int x0, int y0, int rx, int ry, uint16_t color) {
    long x, y;
    long rx2 = rx * rx;
    long ry2 = ry * ry;
    long twoRx2 = 2 * rx2;
    long twoRy2 = 2 * ry2;
    long p;
    long px = 0;
    long py = twoRx2 * ry;

    // Región 1
    p = (long)(ry2 - (rx2 * ry) + (0.25 * rx2));
    x = 0;
    y = ry;
    while (px < py) {
        ST7796_DrawRect(x0 + x, y0 + y, 1, 1, color);
        ST7796_DrawRect(x0 - x, y0 + y, 1, 1, color);
        ST7796_DrawRect(x0 + x, y0 - y, 1, 1, color);
        ST7796_DrawRect(x0 - x, y0 - y, 1, 1, color);
        x++;
        px += twoRy2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }
    }

    // Región 2
    p = (long)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y > 0) {
        ST7796_DrawRect(x0 + x, y0 + y, 1, 1, color);
        ST7796_DrawRect(x0 - x, y0 + y, 1, 1, color);
        ST7796_DrawRect(x0 + x, y0 - y, 1, 1, color);
        ST7796_DrawRect(x0 - x, y0 - y, 1, 1, color);
        y--;
        py -= twoRx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }
    }
}


static void Juego_MoverNave(void) {
    uint32_t joy = Peripherals_ReadJoystick();
    if (joy < 1000)      naveX -= velocidadNave;
    else if (joy > 3000) naveX += velocidadNave;

    if (naveX < 0) naveX = 0;
    if (naveX > (320 - anchoNave)) naveX = 320 - anchoNave;


    if (naveX != oldNaveX || tieneEscudo) {

        ST7796_DrawRect(oldNaveX - 8, naveY - 6, anchoNave + 16, altoNave + 12, 0x0000);

        // 2. Dibujar Nave Blanca
        const uint16_t* s = (naveSeleccionada == 0) ? nave_16x8 : (naveSeleccionada == 1) ? nave_delta_20x10 : nave_interceptor_16x10;
        ST7796_DrawBitmap(naveX, naveY, anchoNave, altoNave, s);

        // 3. INDICADOR DE PROTECCIÓN: CÚPULA OVALADA
        if (tieneEscudo) {
            // Calculamos el centro de la nave
            int cx = naveX + (anchoNave / 2);
            int cy = naveY + (altoNave / 2);

            // Radios del óvalo
            int radioX = (anchoNave / 2) + 5; // Un poco más ancho que la nave
            int radioY = (altoNave / 2) + 4;  // Un poco más alto

            // Dibujamos el óvalo Cian
            DibujarElipse(cx, cy, radioX, radioY, 0x07FF);
        }

        oldNaveX = naveX;
    }
}

static void Juego_DispararJugador(void) {
    uint8_t botonPulsado = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET);

    if (flagDisparoInterrupcion == 1 || (naveSeleccionada == 2 && botonPulsado)) {
        if (!armaSobrecalentada && (HAL_GetTick() - ultimoDisparo > cooldownDisparoNave)) {
            int balasACrear = (naveSeleccionada == 2) ? 3 : 1;
            int creadas = 0;

            for (int i = 0; i < MAX_BALAS && creadas < balasACrear; i++) {
                if (!balas[i].activa) {
                    balas[i].activa = 1;
                    int offset = 0;
                    if (balasACrear == 3) {
                        if (creadas == 1) offset = -6;
                        if (creadas == 2) offset = 6;
                    }
                    balas[i].x = naveX + (anchoNave / 2) + offset;
                    balas[i].y = naveY - 5;
                    creadas++;
                }
            }

            if (creadas > 0) {
                int incremento = (naveSeleccionada == 2) ? 350 : CALOR_POR_DISPARO;
                calorArma += incremento;
                if (calorArma >= MAX_CALOR) armaSobrecalentada = 1;
                ultimoDisparo = HAL_GetTick();
                Peripherals_PlayTone(6000, 20);
            }
            flagDisparoInterrupcion = 0;
        }
    }
}


static void Juego_ActualizarBalasJugador(void) {
    for (int i = 0; i < MAX_BALAS; i++) {
        if (balas[i].activa) {
            ST7796_DrawRect(balas[i].x, balas[i].y, 2, 5, 0x0000);
            balas[i].y -= 12;
            if (balas[i].y < 0) {
                balas[i].activa = 0;
            } else {
                for(int m=0; m < TOTAL_MARCIANOS; m++) {
                    if(marcianos[m].vivo &&
                       balas[i].x >= marcianos[m].x && balas[i].x <= marcianos[m].x + marcianos[m].ancho &&
                       balas[i].y >= marcianos[m].y && balas[i].y <= marcianos[m].y + marcianos[m].alto) {
                        marcianos[m].vivo = 0;
                        marcianosVivos--;
                        balas[i].activa = 0;
                        ST7796_DrawRect(marcianos[m].x, marcianos[m].y, marcianos[m].ancho, marcianos[m].alto, 0x0000);
                        puntuacion += (5 - marcianos[m].tipo) * 10;
                        break;
                    }
                }
                if(balas[i].activa) ST7796_DrawRect(balas[i].x, balas[i].y, 2, 5, 0xFFFF);
            }
        }
    }
}

static void Juego_DibujarHUD(void) {
    char buf[16];
    sprintf(buf, "SCORE:%04d", puntuacion);
    ST7796_WriteString(10, 10, buf, 0xFFFF, 0x0000, 2);

    int anchoBarra = (calorArma * 100) / MAX_CALOR;
    uint16_t colorBarra = armaSobrecalentada ? 0xF800 : 0xFFE0;

    ST7796_DrawRect(200, 15, 100, 10, 0x3186); // Fondo
    ST7796_DrawRect(200, 15, anchoBarra, 10, colorBarra);
    ST7796_WriteString(185, 15, "HEAT", 0xFFFF, 0x0000, 1);
}

static uint8_t Juego_ActualizarBalasEnemigas(void) {
    uint8_t jugadorGolpeado = 0;

    if (HAL_GetTick() - ultimoDisparoEnemigo > cooldownEnemigo) {
        int idx_random = rand() % TOTAL_MARCIANOS;
        if (marcianos[idx_random].vivo) {
            for (int i = 0; i < MAX_BALAS_ENEMIGAS; i++) {
                if (!balasEnemigas[i].activa) {
                    balasEnemigas[i].activa = 1;
                    balasEnemigas[i].x = marcianos[idx_random].x + (marcianos[idx_random].ancho / 2);
                    balasEnemigas[i].y = marcianos[idx_random].y + marcianos[idx_random].alto;
                    break;
                }
            }
        }
        ultimoDisparoEnemigo = HAL_GetTick();
    }

    for (int i = 0; i < MAX_BALAS_ENEMIGAS; i++) {
        if (balasEnemigas[i].activa) {
            ST7796_DrawRect(balasEnemigas[i].x, balasEnemigas[i].y, 2, 5, 0x0000);
            int velocidadBala = (oleada == 3) ? 12 : 8;
            balasEnemigas[i].y += velocidadBala;

            if (balasEnemigas[i].y > 480) {
                balasEnemigas[i].activa = 0;
            } else {
                if (balasEnemigas[i].x >= naveX && balasEnemigas[i].x <= naveX + anchoNave &&
                    balasEnemigas[i].y >= naveY && balasEnemigas[i].y <= naveY + altoNave) {

                    balasEnemigas[i].activa = 0;

                    if (tieneEscudo) {
                        // ESCUDO ROTO
                        tieneEscudo = 0;
                        Peripherals_PlayTone(500, 200);

                        // BORRADO CORRECTO: Área amplia
                        ST7796_DrawRect(naveX - 8, naveY - 6, anchoNave + 16, altoNave + 12, 0x0000);
                        const uint16_t* s = (naveSeleccionada == 0) ? nave_16x8 : (naveSeleccionada == 1) ? nave_delta_20x10 : nave_interceptor_16x10;
                        ST7796_DrawBitmap(naveX, naveY, anchoNave, altoNave, s);
                    } else {
                        // NO TIENE ESCUDO
                        jugadorGolpeado = 1;
                    }

                } else {
                    ST7796_DrawRect(balasEnemigas[i].x, balasEnemigas[i].y, 2, 5, 0xF800);
                }
            }
        }
    }
    return jugadorGolpeado;
}

static uint8_t Juego_MoverMarcianos(void) {
    if (HAL_GetTick() - ultimoMovMarcianos <= 100) return 0;

    uint8_t rebote = 0;
    uint8_t invasion = 0;

    for(int i=0; i<TOTAL_MARCIANOS; i++) {
        if(marcianos[i].vivo) {
            if ((direccionMarcianos == 1 && marcianos[i].x > 290) ||
                (direccionMarcianos == -1 && marcianos[i].x < 10)) {
                rebote = 1;
            }
            if (marcianos[i].y + marcianos[i].alto >= naveY) {
                invasion = 1;
            }
        }
    }

    if (invasion) return 1;

    if (rebote) {
        direccionMarcianos *= -1;
        for(int i=0; i<TOTAL_MARCIANOS; i++) {
            if(marcianos[i].vivo) {
                ST7796_DrawRect(marcianos[i].x, marcianos[i].y, marcianos[i].ancho, marcianos[i].alto, 0x0000);
                marcianos[i].y += 10;
            }
        }
    } else {
        for(int i=0; i<TOTAL_MARCIANOS; i++) {
            if(marcianos[i].vivo) {
                ST7796_DrawRect(marcianos[i].x, marcianos[i].y, marcianos[i].ancho, marcianos[i].alto, 0x0000);
                marcianos[i].x += (velocidadMarcianos * direccionMarcianos);

                const uint16_t* spriteActual;
                if (marcianos[i].tipo == 0)      spriteActual = marciano1_Verde_16x11;
                else if (marcianos[i].tipo == 1) spriteActual = marciano1_Amarillo_16x11;
                else if (marcianos[i].tipo == 2) spriteActual = marciano1_Azul_16x11;
                else if (marcianos[i].tipo == 3) spriteActual = marciano2_Rojo_18x12;
                else                            spriteActual = marciano3_Morado_24x16;

                ST7796_DrawBitmap(marcianos[i].x, marcianos[i].y, marcianos[i].ancho, marcianos[i].alto, spriteActual);
            }
        }
    }
    ultimoMovMarcianos = HAL_GetTick();
    return 0;
}

static void Juego_GestionarNivelYVidas(uint8_t muerteJugador) {
    if (muerteJugador) {
        vidas--;
        Peripherals_UpdateLivesLEDs(vidas);
        if (vidas <= 0) {
            ST7796_Fill(0x0000);
            estadoActual = ESTADO_DERROTA;
        } else {
            ST7796_Fill(0x0000);
            LimpiarTodasLasBalas();
            InicializarMarcianos();
            naveX = 152;
            oldNaveX = -1;
            tieneEscudo = 0;
            HAL_Delay(1000);
        }
    }
    else if (marcianosVivos <= 0 && oleada > 0) {
        oleada++;
        if (oleada > 3) {
            ST7796_Fill(0x0000);
            estadoActual = ESTADO_VICTORIA;
        } else {
            if (oleada == 2) cooldownEnemigo = 700;
            if (oleada == 3) cooldownEnemigo = 400;

            ST7796_Fill(0x0000);
            char txt[20];
            sprintf(txt, "OLEADA %d", oleada);
            ST7796_WriteString(80, 200, txt, 0xFFFF, 0x0000, 3);
            HAL_Delay(1500);
            ST7796_Fill(0x0000);
            LimpiarTodasLasBalas();

            InicializarMarcianos();
            oldNaveX = -1;
           // for (int i = 0; i < MAX_BALAS_ENEMIGAS; i++) balasEnemigas[i].activa = 0;
        }
    }
}
static void ReproducirMelodiaVictoria(void) {
    Peripherals_Beep(1000, 100); HAL_Delay(120);
    Peripherals_Beep(1500, 100); HAL_Delay(120);
    Peripherals_Beep(2000, 400); HAL_Delay(400);
}

static void ReproducirMelodiaDerrota(void) {
    Peripherals_Beep(1500, 200); HAL_Delay(220);
    Peripherals_Beep(1000, 200); HAL_Delay(220);
    Peripherals_Beep(500, 400); HAL_Delay(400);
}

// =========================================================
// PANTALLAS FINALES
// =========================================================

static void Update_Estado_Victoria(void) {
    static uint8_t winPintado = 0;
    if (!winPintado) {
        ST7796_WriteString(60, 200, "VICTORIA!", 0x07E0, 0x0000, 3);

        char buf[32];
        sprintf(buf, "SCORE: %d", puntuacion);
        ST7796_WriteString(90, 245, buf, 0xFFE0, 0x0000, 2);

        ST7796_WriteString(90, 290, "PULSA RESET", 0xFFFF, 0x0000, 2);
        ReproducirMelodiaVictoria();
        winPintado = 1;
    }
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
        winPintado = 0;
        ResetJuego();
        estadoActual = ESTADO_MENU;
    }
}

static void Update_Estado_Derrota(void) {
    static uint8_t lossPintado = 0;
    if (!lossPintado) {
        ST7796_WriteString(80, 200, "GAME OVER", 0xF800, 0x0000, 3);

        char buf[32];
        sprintf(buf, "SCORE: %d", puntuacion);
        ST7796_WriteString(90, 245, buf, 0xFFE0, 0x0000, 2);

        ST7796_WriteString(90, 290, "PULSA RESET", 0xFFFF, 0x0000, 2);
        ReproducirMelodiaDerrota();
        lossPintado = 1;
    }
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
        lossPintado = 0;
        ResetJuego();
        estadoActual = ESTADO_MENU;
    }
}
