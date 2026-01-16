#include "game_engine.h"
#include "st7796_lcd.h"
#include "peripherals.h"
#include "sprites.h"
#include <stdio.h>
#include <stdlib.h>
#define MAX_CALOR 1200
#define CALOR_POR_DISPARO 300
#define ENFRIAMIENTO_FRAME 33
#define COOLDOWN_DISPARO 150




// --- Variables de Estado (Máquina de Estados de Moore) ---
static EstadoJuego estadoActual = ESTADO_MENU;
static uint32_t ultimoTiempoRefresco = 0;
static uint32_t ultimoMovMarcianos = 0;
static uint32_t ultimoDisparo = 0;
static uint32_t ultimoDisparoEnemigo = 0;    //disparos enemigo
static int puntuacion = 0;                   // puntuacion del juego
static uint32_t cooldownEnemigo = 1200; // Tiempo entre disparos (se reduce en cada oleada)
static volatile uint8_t flagDisparoInterrupcion = 0;
const uint32_t TIEMPO_ENTRE_FRAMES = 33;

// --- Variables del Jugador ---
static int naveX = 152;
static int naveY = 420;
static int oldNaveX = 152;
static int oldNaveY = 420;
static int vidas = 3;
static int naveSeleccionada = 0;
static int anchoNave = 16;
static int altoNave = 8;
static int calorArma = 0;
static uint8_t armaSobrecalentada = 0;
static int velocidadNave = 8;        // Píxeles por movimiento
static int cooldownDisparoNave = 150; // Ms entre disparos

// --- Variables de Enemigos y Juego ---
static Marciano marcianos[TOTAL_MARCIANOS];
static Bala balas[MAX_BALAS];
static Bala balasEnemigas[MAX_BALAS_ENEMIGAS];
static int marcianosVivos = 0;
static int oleada = 1;
static int velocidadMarcianos = 4;
static int direccionMarcianos = 1;

// --- Funciones Internas de Ayuda ---

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
            // Definimos dimensiones según el sprite
                        if(fila <= 2) {
                            marcianos[idx].ancho = 16;
                            marcianos[idx].alto = 11;
                        } else if(fila == 3) {
                            marcianos[idx].ancho = 18;
                            marcianos[idx].alto = 12;
                        } else {
                            marcianos[idx].ancho = 24;
                            marcianos[idx].alto = 16;
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
    oleada = 1;
    cooldownEnemigo = 1200;
    armaSobrecalentada = 0;
    naveX = 152;
    oldNaveX = -1; // Forzamos redibujado inicial
    // PROPIEDADES ÚNICAS POR NAVE
        if (naveSeleccionada == 1) {      // DELTA: Veloz, enfriamiento estándar
            anchoNave = 20; altoNave = 10;
            velocidadNave = 14;          // Mucho más rápida
            cooldownDisparoNave = 200;   // Cadencia normal
        } else if (naveSeleccionada == 2) { // INTERCEPTOR: Ráfaga, lenta, se calienta rápido
            anchoNave = 16; altoNave = 10;
            velocidadNave = 4;           // Más lenta maniobrando
            cooldownDisparoNave = 80;    // Rafaga
        } else {                         // NORMAL: Equilibrada
            anchoNave = 16; altoNave = 8;
            velocidadNave = 9;
            cooldownDisparoNave = 180;
        }
    InicializarMarcianos();
    for(int i=0; i<MAX_BALAS; i++) balas[i].activa = 0;
    for(int i=0; i<MAX_BALAS_ENEMIGAS; i++) balasEnemigas[i].activa = 0;
    Peripherals_UpdateLivesLEDs(vidas);

}

// --- Motor del Juego ---

void Game_Init(void) {
    ST7796_Init();
    ST7796_Fill(0x0000);
    ResetJuego();
    Peripherals_UpdateLivesLEDs(vidas);
    Peripherals_Beep(1500, 100);
}
void Game_TriggerFire(void) {
    flagDisparoInterrupcion = 1; // Marcamos que el jugador ha pulsado
}

void Game_Update(void) {
    uint32_t tiempoActual = HAL_GetTick();
    if (tiempoActual - ultimoTiempoRefresco < TIEMPO_ENTRE_FRAMES) return;
    ultimoTiempoRefresco = tiempoActual;

    Peripherals_CheckBuzzerTimeout(); // Sonido no bloqueante

    switch (estadoActual) {
        case ESTADO_MENU: {
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
            break;
        }
        case ESTADO_SELECCION: {
                    static int seleccionAnterior = -1;
                    uint32_t joyValSel = Peripherals_ReadJoystick();

                    if (joyValSel > 3000) { naveSeleccionada++; HAL_Delay(150); }
                    else if (joyValSel < 1000) { naveSeleccionada--; HAL_Delay(150); }

                    if (naveSeleccionada > 2) naveSeleccionada = 0;
                    if (naveSeleccionada < 0) naveSeleccionada = 2;

                    // Solo redibujamos si cambia la selección para evitar parpadeos
                    if (naveSeleccionada != seleccionAnterior) {
                        ST7796_DrawRect(40, 150, 240, 250, 0x0000); // Limpia zona central
                        ST7796_WriteString(80, 40, "SELECCIONA", 0xFFFF, 0x0000, 2);
                        ST7796_WriteString(100, 70, "TU NAVE", 0xFFFF, 0x0000, 2);

                        if (naveSeleccionada == 0) {
                            ST7796_DrawBitmapZoom(128, 180, 16, 8, nave_16x8, 4);
                            ST7796_WriteString(100, 260, "NORMAL", 0x07E0, 0x0000, 2);
                            ST7796_WriteString(60, 290, "CALOR:     ***", 0xFFFF, 0x0000, 1);
                            ST7796_WriteString(60, 310, "VELOCIDAD: ***",   0xFFFF, 0x0000, 1);
                            ST7796_WriteString(60, 330, "FUEGO:     ***",   0xFFFF, 0x0000, 1);
                        } else if (naveSeleccionada == 1) {
                            ST7796_DrawBitmapZoom(120, 180, 20, 10, nave_delta_20x10, 4);
                            ST7796_WriteString(110, 260, "DELTA", 0x07E0, 0x0000, 2);
                            ST7796_WriteString(60, 290, "CALOR:     ***",   0xFFFF, 0x0000, 1);
                            ST7796_WriteString(60, 310, "VELOCIDAD: *****", 0xFFFF, 0x0000, 1);
                            ST7796_WriteString(60, 330, "FUEGO:     ***",   0xFFFF, 0x0000, 1);
                        } else {
                            ST7796_DrawBitmapZoom(128, 180, 16, 10, nave_interceptor_16x10, 4);
                            ST7796_WriteString(80, 260, "INTERCEPTOR", 0x07E0, 0x0000, 2);
                            ST7796_WriteString(60, 290, "CALOR:     *****",   0xFFFF, 0x0000, 1);
                            ST7796_WriteString(60, 310, "VELOCIDAD: **",   0xFFFF, 0x0000, 1);
                            ST7796_WriteString(60, 330, "FUEGO:     *****", 0xFFFF, 0x0000, 1);
                        }
                        seleccionAnterior = seleccionAnterior; // Error corregido aquí:
                        seleccionAnterior = naveSeleccionada;
                    }

                    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
                        Peripherals_Beep(1200, 100);
                        ST7796_Fill(0x0000); // Limpieza total antes de empezar
                        seleccionAnterior = -1;
                        ResetJuego(); // Inicializa marcianos y vidas
                        // --- FORZAR DIBUJO INICIAL DE LA NAVE ---
                        const uint16_t* s = (naveSeleccionada==0)?nave_16x8:(naveSeleccionada==1)?nave_delta_20x10:nave_interceptor_16x10;
                        ST7796_DrawBitmap(naveX, naveY, anchoNave, altoNave, s);
                        oldNaveX = naveX; // Sincronizar para que no se borre inmediatamente
                        estadoActual = ESTADO_JUEGO;
                        return;
                    }
                    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
                          Peripherals_Beep(800, 100);
                          ST7796_Fill(0x0000);
                          seleccionAnterior = -1;
                          estadoActual = ESTADO_MENU;
                         HAL_Delay(200);
                      }
                    break;
                }

                case ESTADO_JUEGO: {
                    uint8_t muerteJugador = 0;

                    // 1. Enfriamiento Arma
                    if (calorArma > 0) {
                        calorArma -= ENFRIAMIENTO_FRAME;
                        if (calorArma < 0) calorArma = 0;
                    }
                    if (armaSobrecalentada && calorArma == 0) armaSobrecalentada = 0;

                    // 2. Movimiento Nave
                    uint32_t joy = Peripherals_ReadJoystick();
                    if (joy < 1000)      naveX -= velocidadNave;
                    else if (joy > 3000) naveX += velocidadNave;

                    if (naveX < 0) naveX = 0;
                    if (naveX > (320 - anchoNave)) naveX = 320 - anchoNave;

                    // Dibujamos si hay movimiento O si es el primer frame después de un reset (usando un flag o forzando oldNaveX)
                    if (naveX != oldNaveX) {
                        ST7796_DrawRect(oldNaveX, naveY, anchoNave, altoNave, 0x0000);
                        const uint16_t* s = (naveSeleccionada == 0) ? nave_16x8 : (naveSeleccionada == 1) ? nave_delta_20x10 : nave_interceptor_16x10;
                        ST7796_DrawBitmap(naveX, naveY, anchoNave, altoNave, s);
                        oldNaveX = naveX;
                    }
                    // 3. Disparos Jugador (Ráfaga Triple para Interceptor)
                    uint8_t botonPulsado = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET);

                    if (flagDisparoInterrupcion == 1 || (naveSeleccionada == 2 && botonPulsado)) {
                        if (!armaSobrecalentada && (HAL_GetTick() - ultimoDisparo > cooldownDisparoNave)) {

                            int balasACrear = (naveSeleccionada == 2) ? 3 : 1; // Triple para Interceptor
                            int creadas = 0;

                            for (int i = 0; i < MAX_BALAS && creadas < balasACrear; i++) {
                                if (!balas[i].activa) {
                                    balas[i].activa = 1;

                                    // Calculamos desplazamiento X para el disparo triple
                                    // Una al centro, una a la izquierda (-6px) y otra a la derecha (+6px)
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
                                // El calor sube según cuántas balas dispares
                                int incremento = (naveSeleccionada == 2) ? 350 : CALOR_POR_DISPARO;
                                calorArma += incremento;

                                if (calorArma >= MAX_CALOR) armaSobrecalentada = 1;
                                ultimoDisparo = HAL_GetTick();
                                Peripherals_PlayTone(6000, 20); // Tono más agudo para ráfaga
                            }
                            flagDisparoInterrupcion = 0;
                        }
                    }

                    // 4. Actualizar Balas y Colisiones
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

                                        // --- SUMAR PUNTOS ---
                                        puntuacion += (5 - marcianos[m].tipo) * 10;
                                        char buf[16];
                                        sprintf(buf, "SCORE:%04d", puntuacion);
                                        ST7796_WriteString(10, 10, buf, 0xFFFF, 0x0000, 2);

                                        break;
                                    }
                                }
                                if(balas[i].activa) ST7796_DrawRect(balas[i].x, balas[i].y, 2, 5, 0xFFFF);
                            }
                        }
                    }

                    // --- DIBUJAR BARRA DE CALOR ---
                    int anchoBarra = (calorArma * 100) / MAX_CALOR; // Mapeo de 0-1200 a 0-100 píxeles
                    uint16_t colorBarra = armaSobrecalentada ? 0xF800 : 0xFFE0; // Rojo si está bloqueada, Amarillo si no

                    // Borramos el fondo de la barra y dibujamos el nivel actual
                    ST7796_DrawRect(200, 15, 100, 10, 0x3186); // Fondo gris oscuro
                    ST7796_DrawRect(200, 15, anchoBarra, 10, colorBarra);
                    ST7796_WriteString(185, 15, "HEAT", 0xFFFF, 0x0000, 1);


			// --- Disparos Enemigos  ---
			if (HAL_GetTick() - ultimoDisparoEnemigo > cooldownEnemigo) {
				int idx_random = rand() % TOTAL_MARCIANOS;
				if (marcianos[idx_random].vivo) {
					for (int i = 0; i < MAX_BALAS_ENEMIGAS; i++) {
						if (!balasEnemigas[i].activa) {
							balasEnemigas[i].activa = 1;
							balasEnemigas[i].x = marcianos[idx_random].x
									+ (marcianos[idx_random].ancho / 2);
							balasEnemigas[i].y = marcianos[idx_random].y
									+ marcianos[idx_random].alto;
							break;
						}
					}
				}
				ultimoDisparoEnemigo = HAL_GetTick();
			}

			// Mover Balas Enemigas (Movimiento fluido a 30 FPS)
			for (int i = 0; i < MAX_BALAS_ENEMIGAS; i++) {
				if (balasEnemigas[i].activa) {
					ST7796_DrawRect(balasEnemigas[i].x, balasEnemigas[i].y, 2,
							5, 0x0000);

					// Velocidad base + extra según oleada
					int velocidadBala = (oleada == 3) ? 12 : 8;
					balasEnemigas[i].y += velocidadBala;

					if (balasEnemigas[i].y > 480) {
						balasEnemigas[i].activa = 0;
					} else {
						if (balasEnemigas[i].x >= naveX
								&& balasEnemigas[i].x <= naveX + anchoNave
								&& balasEnemigas[i].y >= naveY
								&& balasEnemigas[i].y <= naveY + altoNave) {
							balasEnemigas[i].activa = 0;
							muerteJugador = 1;
						} else {
							ST7796_DrawRect(balasEnemigas[i].x,
									balasEnemigas[i].y, 2, 5, 0xF800);
						}
					}
				}
			}

			// 5. Mover Marcianos (Bloque de 100ms)
			if (HAL_GetTick() - ultimoMovMarcianos > 100) {
			    uint8_t rebote = 0; // Declaramos la variable que faltaba

			    // Primero comprobamos si alguno debe rebotar
			    for(int i=0; i<TOTAL_MARCIANOS; i++) {
			        if(marcianos[i].vivo) {
			            if ((direccionMarcianos == 1 && marcianos[i].x > 290) ||
			                (direccionMarcianos == -1 && marcianos[i].x < 10)) {
			                rebote = 1;
			                break;
			            }
			            // Aprovechamos para ver si invaden la nave
			            if (marcianos[i].y + marcianos[i].alto >= naveY) {
			                muerteJugador = 1;
			            }
			        }
			    }

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
			}
                    // Gestión de Muerte y Vidas
                    if (muerteJugador) {
                        vidas--;
                        Peripherals_UpdateLivesLEDs(vidas);
                        if (vidas <= 0) {
                        	ST7796_Fill(0x0000);
                            estadoActual = ESTADO_DERROTA;
                        } else {
                            ST7796_Fill(0x0000); // Limpiar para resetear oleada o posición
                            InicializarMarcianos(); // Los marcianos vuelven arriba
                            naveX = 152;
                            oldNaveX = -1;
                            HAL_Delay(1000);
                        }
                    }
                    //Volver al menú durante la partida con PB2 ---
                       if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
                            Peripherals_Beep(800, 100);
                            ST7796_Fill(0x0000);
                            ResetJuego();        // Reinicia variables de juego
                            estadoActual = ESTADO_MENU;
                            HAL_Delay(200);      // Debounce para evitar saltos
                         }

			if (marcianosVivos <= 0 && oleada > 0) {
				oleada++;
				if (oleada > 3) {
					ST7796_Fill(0x0000);
					estadoActual = ESTADO_VICTORIA;
				} else {
					// Ajuste de dificultad según oleada
					if (oleada == 2)
						cooldownEnemigo = 700;  // Más aliens disparando
					if (oleada == 3)
						cooldownEnemigo = 400; // Lluvia de balas y más rápidas!

					ST7796_Fill(0x0000);
					char txt[20];
					sprintf(txt, "OLEADA %d", oleada);
					ST7796_WriteString(80, 200, txt, 0xFFFF, 0x0000, 3);
					HAL_Delay(1500);
					ST7796_Fill(0x0000);

					InicializarMarcianos();
					oldNaveX = -1;
					// Limpiar balas pendientes para no morir al aparecer
					for (int i = 0; i < MAX_BALAS_ENEMIGAS; i++)
						balasEnemigas[i].activa = 0;
				}
			}
                    break;
           }

		case ESTADO_VICTORIA:
		{
			static uint8_t winPintado = 0;
			if (!winPintado) {
				ST7796_WriteString(60, 200, "VICTORIA!", 0x07E0, 0x0000, 3);
				ST7796_WriteString(50, 280, "PULSA RESET", 0xFFFF, 0x0000, 2);
				winPintado = 1;
			}
			if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
				winPintado = 0;
				ResetJuego();
				estadoActual = ESTADO_MENU;
			}
			break;
		}

		case ESTADO_DERROTA:
		{
			static uint8_t lossPintado = 0;
			if (!lossPintado) {
				ST7796_WriteString(80, 200, "GAME OVER", 0xF800, 0x0000, 3);
				ST7796_WriteString(50, 280, "PULSA RESET", 0xFFFF, 0x0000, 2);
				lossPintado = 1;
			}
			if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
				lossPintado = 0;
				ResetJuego();
				estadoActual = ESTADO_MENU;
			}
			break;
		}
    }
  }

