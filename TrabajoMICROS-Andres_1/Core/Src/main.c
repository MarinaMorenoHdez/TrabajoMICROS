/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#include "sprites.h"
#include "fonts.h"
#include <stdio.h>
#include <stdlib.h>

// --- 1. DEFINICIONES DE PINES ---
#define LCD_CS_LOW()    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define LCD_DC_CMD()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_DATA()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)
#define LCD_RST_LOW()   HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define LCD_RST_HIGH()  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;
extern ADC_HandleTypeDef hadc1;

// --- 2. FUNCIONES BASE PANTALLA ---
void ST7796_Cmd(uint8_t cmd) {
    LCD_DC_CMD(); LCD_CS_LOW(); HAL_SPI_Transmit(&hspi1, &cmd, 1, 10); LCD_CS_HIGH();
}

void ST7796_Data(uint8_t data) {
    LCD_DC_DATA(); LCD_CS_LOW(); HAL_SPI_Transmit(&hspi1, &data, 1, 10); LCD_CS_HIGH();
}

void ST7796_Init(void) {
    LCD_CS_HIGH(); LCD_RST_LOW(); HAL_Delay(100); LCD_RST_HIGH(); HAL_Delay(100);
    ST7796_Cmd(0x01); HAL_Delay(120);
    ST7796_Cmd(0x11); HAL_Delay(120);
    ST7796_Cmd(0x3A); ST7796_Data(0x55);
    ST7796_Cmd(0x36); ST7796_Data(0x48);
    ST7796_Cmd(0x29); HAL_Delay(10);
}

void ST7796_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    ST7796_Cmd(0x2A);
    uint8_t dataX[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    LCD_DC_DATA(); LCD_CS_LOW(); HAL_SPI_Transmit(&hspi1, dataX, 4, 10); LCD_CS_HIGH();

    ST7796_Cmd(0x2B);
    uint8_t dataY[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
    LCD_DC_DATA(); LCD_CS_LOW(); HAL_SPI_Transmit(&hspi1, dataY, 4, 10); LCD_CS_HIGH();
    ST7796_Cmd(0x2C);
}

void ST7796_Fill(uint16_t color) {
    ST7796_SetWindow(0, 0, 319, 479);
    uint8_t colorBytes[2] = {color >> 8, color & 0xFF};
    LCD_DC_DATA(); LCD_CS_LOW();
    for(long i = 0; i < (320 * 480); i++) HAL_SPI_Transmit(&hspi1, colorBytes, 2, 10);
    LCD_CS_HIGH();
}

void ST7796_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if((x+w) > 320 || (y+h) > 480) return;
    ST7796_SetWindow(x, y, x + w - 1, y + h - 1);
    uint8_t colorBytes[2] = {color >> 8, color & 0xFF};
    LCD_DC_DATA(); LCD_CS_LOW();
    for(uint32_t i = 0; i < (w * h); i++) HAL_SPI_Transmit(&hspi1, colorBytes, 2, 10);
    LCD_CS_HIGH();
}

void ST7796_DrawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap) {
    if((x+w) > 320 || (y+h) > 480) return;
    ST7796_SetWindow(x, y, x + w - 1, y + h - 1);
    LCD_DC_DATA(); LCD_CS_LOW();
    for(uint32_t i = 0; i < (w * h); i++) {
        uint16_t color = bitmap[i];
        uint8_t colorBytes[2] = {color >> 8, color & 0xFF};
        HAL_SPI_Transmit(&hspi1, colorBytes, 2, 10);
    }
    LCD_CS_HIGH();
}

// --- 3. FUNCIONES DE TEXTO (FONTS) ---
void ST7796_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if((x+7*size) > 320 || (y+10*size) > 480) return;
    if(c < 32 || c > 127) return;
    const uint16_t *pChar = &Font7x10[(c - 32) * 10];

    ST7796_SetWindow(x, y, x + (7*size) - 1, y + (10*size) - 1);
    LCD_DC_DATA(); LCD_CS_LOW();

    for(int i=0; i<10; i++) {
        uint16_t line = pChar[i];
        for(int r=0; r<size; r++) {
            for(int j=0; j<7; j++) {
                uint16_t pixelColor = (line & (0x8000 >> j)) ? color : bg;
                uint8_t colorBytes[2] = {pixelColor >> 8, pixelColor & 0xFF};
                for(int k=0; k<size; k++) {
                   HAL_SPI_Transmit(&hspi1, colorBytes, 2, 10);
                }
            }
        }
    }
    LCD_CS_HIGH();
}

void ST7796_WriteString(uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bg, uint8_t size) {
    while(*str) {
        ST7796_DrawChar(x, y, *str, color, bg, size);
        x += 7 * size;
        str++;
    }
}

// --- FUNCIÓN PARA DIBUJAR CON ZOOM ---
// x, y: Coordenadas
// w, h: Ancho y alto original
// bitmap: El array de la imagen
// zoom: Cuántas veces más grande (1 = normal, 2 = doble, 3 = triple...)
void ST7796_DrawBitmapZoom(int x, int y, int w, int h, const uint16_t* bitmap, int zoom) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            // Obtenemos el color del píxel original
            uint16_t color = bitmap[j * w + i];

            // Si no es negro (transparencia), pintamos un cuadrado gordo
            if (color != 0x0000) {
                ST7796_DrawRect(x + (i * zoom), y + (j * zoom), zoom, zoom, color);
            }
        }
    }
}

// --- 4. SONIDO ---
void Beep(uint16_t frecuencia, uint16_t duracion) {
    if(frecuencia == 0) return;
    uint32_t periodo = 1000000 / frecuencia;
    __HAL_TIM_SET_AUTORELOAD(&htim2, periodo);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, periodo / 2);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_Delay(duracion);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
}

// --- 5. VARIABLES GLOBALES
int naveX = 152;
int naveY = 420;

typedef enum {
    ESTADO_MENU,
    ESTADO_SELECCION,
    ESTADO_JUEGO,
    ESTADO_VICTORIA,
    ESTADO_DERROTA
} EstadoJuego;

// --- DISPAROS ENEMIGOS ---
#define MAX_BALAS_ENEMIGAS 5      // Máximo de balas enemigas en pantalla
#define VELOCIDAD_BALA_ENEMIGA 6  // Bajan un poco más lento que las tuyas
typedef struct {
    int x, y;
    uint8_t activa;
} BalaEnemiga;
BalaEnemiga balasEnemigas[MAX_BALAS_ENEMIGAS];

// --- GESTIÓN DE LEDS DE VIDA (PD13, PD14, PD15) ---
void ActualizarLEDsVidas(int numVidas) {
    // 1. Apagamos todos primero (para limpiar estado)
    HAL_GPIO_WritePin(LED_1VIDA_GPIO_Port, LED_1VIDA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_2VIDA_GPIO_Port, LED_2VIDA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_3VIDA_GPIO_Port, LED_3VIDA_Pin, GPIO_PIN_RESET);
    if (numVidas >= 1) HAL_GPIO_WritePin(LED_1VIDA_GPIO_Port, LED_1VIDA_Pin, GPIO_PIN_SET);
    if (numVidas >= 2) HAL_GPIO_WritePin(LED_2VIDA_GPIO_Port, LED_2VIDA_Pin, GPIO_PIN_SET);
    if (numVidas >= 3) HAL_GPIO_WritePin(LED_3VIDA_GPIO_Port, LED_3VIDA_Pin, GPIO_PIN_SET);
}

EstadoJuego estadoActual = ESTADO_MENU;

uint32_t ultimoTiempoRefresco = 0;
const uint32_t TIEMPO_ENTRE_FRAMES = 33;

int vidas = 3;
int naveSeleccionada = 0;

// --- DEFINICIONES DE ENEMIGOS
#define FILAS_MARCIANOS 5  // 1 fila de cada tipo
#define COLS_MARCIANOS 6   // 6 columnas
#define TOTAL_MARCIANOS (FILAS_MARCIANOS * COLS_MARCIANOS)

typedef struct {
    int x;
    int y;
    int tipo;
    int ancho;
    int alto;
    uint8_t vivo;
} Marciano;

Marciano marcianos[TOTAL_MARCIANOS];
int velocidadMarcianos = 4; // ¡Más rápido porque hay menos!
int direccionMarcianos = 1;

// --- DEFINICIONES DISPAROS ---
#define MAX_BALAS 10       // Bajamos un poco para ahorrar cálculos
#define VELOCIDAD_BALA 12
#define COOLDOWN_DISPARO 150

typedef struct {
    int x;
    int y;
    uint8_t activa;
} Bala;

Bala balas[MAX_BALAS];

// --- SISTEMA DE SOBRECALENTAMIENTO DEL ARMA, COOLDOWN ---
#define MAX_CALOR 1200           // Tiempo de cooldown de la rafaga
#define CALOR_POR_DISPARO 300    // Cuánto calienta cada bala
#define ENFRIAMIENTO_FRAME 33    // Cuánto enfría cada vuelta del bucle
// CÁLCULO: 300 calor / 150ms cadencia vs 33 enfriamiento * 4.5 frames = Sube el calor neto.

int calorArma = 0;               // Variable actual de calor (0 a 2000)
uint8_t armaSobrecalentada = 0;  // Flag: 1 = Bloqueada, 0 = Lista

// --- SISTEMA DE SONIDO NO BLOQUEANTE (REFORZADO) ---
uint32_t tiempoFinSonido = 0;
uint8_t buzzerActivo = 0;

void PlayTone(uint16_t frecuencia, uint16_t duracion) {
    if(frecuencia == 0) return;

    // 1. Parar PWM para configurar
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);

    // 2. Calcular periodo (Reloj 1MHz)
    uint32_t periodo = 1000000 / frecuencia;

    // 3. Aplicar configuración
    __HAL_TIM_SET_AUTORELOAD(&htim2, periodo);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, periodo / 2); // 50% Duty Cycle
    // Forzar actualización de registros sombra (Importante para STM32)
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    // 4. Arrancar de nuevo
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

    // 5. Programar apagado
    tiempoFinSonido = HAL_GetTick() + duracion;
    buzzerActivo = 1;
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

    // 1. Inicializamos hardware
    ST7796_Init();
    ST7796_Fill(0x0000); // Fondo negro

        Beep(1500, 100);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	      uint32_t tiempoActual = HAL_GetTick();

	      if (tiempoActual - ultimoTiempoRefresco >= TIEMPO_ENTRE_FRAMES) {

	          ultimoTiempoRefresco = tiempoActual;

	          // MÁQUINA DE ESTADOS
	          switch (estadoActual) {

	          case ESTADO_MENU:

	                          static uint8_t menuPintado = 0;

	                          // 1. DIBUJAR
	                          if (menuPintado == 0) {
	                              ST7796_Fill(0x0000); // Limpiar pantalla a negro

	                              // Título Grande - Color Verde
	                              ST7796_WriteString(40, 100, "SPACE", 0x07E0, 0x0000, 5);
	                              ST7796_WriteString(20, 160, "INVADERS", 0xF800, 0x0000, 5);

	                              // Nombres - Color Blanco
	                              ST7796_WriteString(50, 230, "Desarrollado por", 0xFFFF, 0x0000, 1);
	                              ST7796_WriteString(50, 250, "- Andres Galindo Gordon", 0xFFFF, 0x0000, 1);
	                              ST7796_WriteString(50, 260, "- Marina Moreno Hernandez", 0xFFFF, 0x0000, 1);
	                              ST7796_WriteString(50, 270, "- Segio Llana Ayen", 0xFFFF, 0x0000, 1);

	                              // Instrucción
	                              ST7796_WriteString(60, 350, "PULSA START", 0xFFE0, 0x0000, 2);

	                              menuPintado = 1;
	                          }

	                          // 2. LOGICA (Esperar botón START - PB0)
	                          if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
	                              Beep(1000, 100);
	                              ST7796_Fill(0x0000);
	                              menuPintado = 0;
	                              estadoActual = ESTADO_SELECCION;
	                              HAL_Delay(300);
	                          }
	                          break;

	          case ESTADO_SELECCION:
					  static int seleccionAnterior = -1;
					  static uint8_t primeraVez = 1;

					  // 1. DIBUJAR INTERFAZ FIJA
					  if (primeraVez) {
						  ST7796_Fill(0x0000);

						  // Títulos estáticos
						  ST7796_WriteString(80, 40, "SELECCIONA", 0xFFFF, 0x0000, 2);
						  ST7796_WriteString(100, 70, "TU NAVE", 0xFFFF, 0x0000, 2);

						  // Flechas estáticas
						  ST7796_WriteString(20, 200, "<", 0xFFFF, 0x0000, 2);
						  ST7796_WriteString(290, 200, ">", 0xFFFF, 0x0000, 2);

						  ST7796_WriteString(60, 400, "PULSA START", 0xFF00, 0x0000, 2);

						  primeraVez = 0;
						  seleccionAnterior = -1;
					  }

					  // 2. LEER JOYSTICK
					  HAL_ADC_Start(&hadc1);
					  HAL_ADC_PollForConversion(&hadc1, 10);
					  uint32_t joyValSel = HAL_ADC_GetValue(&hadc1);

					  if (joyValSel > 3000) {
						  naveSeleccionada++;
						  if (naveSeleccionada > 2) naveSeleccionada = 0;
						  PlayTone(2000, 30);
						  HAL_Delay(150);
					  }
					  else if (joyValSel < 1000) {
						  naveSeleccionada--;
						  if (naveSeleccionada < 0) naveSeleccionada = 2;
						  PlayTone(2000, 30);
						  HAL_Delay(150);
					  }

					  // 3. DIBUJAR NAVES (al cambiar selección)
					  if (naveSeleccionada != seleccionAnterior) {

						  //BORRAR (solo zona central) ---
						  ST7796_DrawRect(40, 150, 240, 200, 0x0000);

						  // --- LOGICA DE POSICIONES ---
						  int naveIzquierda = (naveSeleccionada == 0) ? 2 : naveSeleccionada - 1;
						  int naveDerecha   = (naveSeleccionada == 2) ? 0 : naveSeleccionada + 1;

						  // A. NAVE IZQUIERDA
						  if (naveIzquierda == 0) ST7796_DrawBitmap(40, 200, 16, 8, nave_16x8);
						  else if (naveIzquierda == 1) ST7796_DrawBitmap(40, 200, 20, 10, nave_delta_20x10);
						  else ST7796_DrawBitmap(40, 200, 16, 10, nave_interceptor_16x10);

						  // B. NAVE DERECHA
						  if (naveDerecha == 0) ST7796_DrawBitmap(260, 200, 16, 8, nave_16x8);
						  else if (naveDerecha == 1) ST7796_DrawBitmap(260, 200, 20, 10, nave_delta_20x10);
						  else ST7796_DrawBitmap(260, 200, 16, 10, nave_interceptor_16x10);

						  // C. NAVE CENTRAL (x4)
						  if (naveSeleccionada == 0) {
							  ST7796_DrawBitmapZoom(128, 180, 16, 8, nave_16x8, 4);
							  ST7796_WriteString(100, 260, "NORMAL", 0x07E0, 0x0000, 2);
							  ST7796_WriteString(80, 290, "Salud        -----", 0xFFFF, 0x0000, 1);
							  ST7796_WriteString(80, 310, "Disparo      -----", 0xFFFF, 0x0000, 1);
							  ST7796_WriteString(80, 330, "Velocidad    -----", 0xFFFF, 0x0000, 1);
						  }
						  else if (naveSeleccionada == 1) {
							  ST7796_DrawBitmapZoom(120, 180, 20, 10, nave_delta_20x10, 4);
							  ST7796_WriteString(110, 260, "DELTA", 0x07E0, 0x0000, 2);
							  ST7796_WriteString(80, 290, "Salud        -----", 0xFFFF, 0x0000, 1);
							  ST7796_WriteString(80, 310, "Disparo      ---", 0xFFFF, 0x0000, 1);
							  ST7796_WriteString(80, 330, "Velocidad    ----------", 0xFFFF, 0x0000, 1);
						  }
						  else {
							  ST7796_DrawBitmapZoom(128, 180, 16, 10, nave_interceptor_16x10, 4);
							  ST7796_WriteString(80, 260, "INTERCEPTOR", 0x07E0, 0x0000, 2);
							  ST7796_WriteString(80, 290, "Salud        ------", 0xFFFF, 0x0000, 1);
							  ST7796_WriteString(80, 310, "Disparo      ---------", 0xFFFF, 0x0000, 1);
							  ST7796_WriteString(80, 330, "Velocidad    --", 0xFFFF, 0x0000, 1);
						  }

						  seleccionAnterior = naveSeleccionada;
					  }

					  // 4. CONFIRMAR (START)
					  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) {
						  PlayTone(1000, 100);
						  PlayTone(1500, 200);

						  primeraVez = 1; // Resetear para la próxima vez que volvamos al menú
						  seleccionAnterior = -1;

						  estadoActual = ESTADO_JUEGO;
						  HAL_Delay(300);
					  }
					  break;

	              case ESTADO_JUEGO:
					  // --- VARIABLES ESTÁTICAS
					  static uint8_t juegoIniciado = 0;
					  static int anchoNave = 16;
					  static int altoNave = 8;
					  static int oldNaveX = 0;
					  static int oldNaveY = 0;

					  static uint32_t ultimoDisparo = 0;
					  static uint32_t ultimoMovMarcianos = 0;
					  static int marcianosVivos = 0;
					  static int oleada = 1;

					  // Variable de muerte unificada (para colisión física o disparo)
					  uint8_t muerteJugador = 0;

					  // --- 1. INICIALIZACIÓN
					  if (juegoIniciado == 0) {
						  ST7796_Fill(0x0000);

						  if (naveSeleccionada == 0) { anchoNave = 16; altoNave = 8; }
						  else if (naveSeleccionada == 1) { anchoNave = 20; altoNave = 10; }
						  else if (naveSeleccionada == 2) { anchoNave = 16; altoNave = 10; }

						  naveX = 152;
						  naveY = 420;

						  for(int i=0; i<MAX_BALAS; i++) balas[i].activa = 0;
						  for(int i=0; i<MAX_BALAS_ENEMIGAS; i++) balasEnemigas[i].activa = 0; // Reset balas enemigas

						  calorArma = 0;
						  armaSobrecalentada = 0;

						  oleada = 1;
						  velocidadMarcianos = 4;
						  ActualizarLEDsVidas(vidas);

						  // INICIALIZAR MARCIANOS
						  int idx = 0;
						  direccionMarcianos = 1;
						  marcianosVivos = TOTAL_MARCIANOS;

						  for(int fila=0; fila < FILAS_MARCIANOS; fila++) {
							  for(int col=0; col < COLS_MARCIANOS; col++) {
								  marcianos[idx].vivo = 1;
								  marcianos[idx].x = 30 + (col * 45);
								  marcianos[idx].y = 40 + (fila * 30);
								  marcianos[idx].tipo = fila;

								  if(fila==0) { marcianos[idx].ancho=16; marcianos[idx].alto=11; }
								  else if(fila==1) { marcianos[idx].ancho=16; marcianos[idx].alto=11; }
								  else if(fila==2) { marcianos[idx].ancho=16; marcianos[idx].alto=11; }
								  else if(fila==3) { marcianos[idx].ancho=18; marcianos[idx].alto=12; }
								  else { marcianos[idx].ancho=24; marcianos[idx].alto=16; }
								  idx++;
							  }
						  }

						  // Animación
						  for (int y = 480; y >= 420; y -= 4) {
							  ST7796_DrawRect(naveX, y + 4, anchoNave, altoNave, 0x0000);
							  if (naveSeleccionada == 0) ST7796_DrawBitmap(naveX, y, 16, 8, nave_16x8);
							  else if (naveSeleccionada == 1) ST7796_DrawBitmap(naveX, y, 20, 10, nave_delta_20x10);
							  else if (naveSeleccionada == 2) ST7796_DrawBitmap(naveX, naveY, 16, 10, nave_interceptor_16x10);
							  HAL_Delay(5);
						  }
						  oldNaveX = naveX;
						  oldNaveY = naveY;

						  char bufferVidas[10];
						  sprintf(bufferVidas, "VIDAS: %d", vidas);
						  ST7796_WriteString(10, 10, bufferVidas, 0xFFFF, 0x0000, 1);

						  juegoIniciado = 1;
					  }

					  // --- 2. ENFRIAMIENTO Y MOVIMIENTO NAVE ---
					  if (calorArma > 0) {
						  calorArma -= ENFRIAMIENTO_FRAME;
						  if (calorArma < 0) calorArma = 0;
					  }
					  if (armaSobrecalentada && calorArma == 0) {
						  armaSobrecalentada = 0;
						  ST7796_WriteString(200, 10, "            ", 0x0000, 0x0000, 1);
						  PlayTone(1000, 50);
					  }

					  HAL_ADC_Start(&hadc1);
					  HAL_ADC_PollForConversion(&hadc1, 10);
					  uint32_t valJoy = HAL_ADC_GetValue(&hadc1);

					  if (valJoy < 1000) { naveX -= 8; if (naveX < 0) naveX = 0; }
					  else if (valJoy > 3000) { naveX += 8; if (naveX > (320 - anchoNave)) naveX = 320 - anchoNave; }

					  // --- 3. DISPAROS JUGADOR ---
					  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) {
						  if (!armaSobrecalentada) {
							  if (HAL_GetTick() - ultimoDisparo > COOLDOWN_DISPARO) {
								  for (int i = 0; i < MAX_BALAS; i++) {
									  if (balas[i].activa == 0) {
										  balas[i].activa = 1;
										  balas[i].x = naveX + (anchoNave / 2) - 1;
										  balas[i].y = naveY - 4;
										  PlayTone(5000, 30);
										  calorArma += CALOR_POR_DISPARO;
										  if (calorArma >= MAX_CALOR) {
											  armaSobrecalentada = 1;
											  calorArma = MAX_CALOR;
											  ST7796_WriteString(200, 10, "ENFRIANDO...", 0xF800, 0x0000, 1);
											  PlayTone(100, 100);
										  }
										  ultimoDisparo = HAL_GetTick();
										  break;
									  }
								  }
							  }
						  }
					  }

					  // --- 4. ACTUALIZAR BALAS JUGADOR ---
					  for (int i = 0; i < MAX_BALAS; i++) {
						  if (balas[i].activa == 1) {
							  ST7796_DrawRect(balas[i].x, balas[i].y, 2, 5, 0x0000);
							  balas[i].y -= VELOCIDAD_BALA;

							  if (balas[i].y < 0) balas[i].activa = 0;
							  else {
								  uint8_t impacto = 0;
								  for(int m=0; m < TOTAL_MARCIANOS; m++) {
									  if(marcianos[m].vivo) {
										  if(balas[i].x >= marcianos[m].x &&
											 balas[i].x <= (marcianos[m].x + marcianos[m].ancho) &&
											 balas[i].y >= marcianos[m].y &&
											 balas[i].y <= (marcianos[m].y + marcianos[m].alto)) {

											  marcianos[m].vivo = 0;
											  ST7796_DrawRect(marcianos[m].x, marcianos[m].y, marcianos[m].ancho, marcianos[m].alto, 0x0000);
											  balas[i].activa = 0;
											  impacto = 1;
											  marcianosVivos--;
											  PlayTone(100, 40);
											  break;
										  }
									  }
								  }
								  if(!impacto) ST7796_DrawRect(balas[i].x, balas[i].y, 2, 5, 0xFFFF);
							  }
						  }
					  }

					  // --- 5. INTELIGENCIA ARTIFICIAL ENEMIGA (MEJORADA) ---
					  if (oleada >= 2) {
						  for(int i=0; i<TOTAL_MARCIANOS; i++) {
							  // Solo procesamos si el marciano está vivo
							  if (marcianos[i].vivo) {

								  int puedeDisparar = 0; // Bandera de permiso

								  // REGLA 1: Los Morados (Tipo 4) disparan desde la Oleada 2
								  if (marcianos[i].tipo == 4 && oleada >= 2) {
									  puedeDisparar = 1;
								  }

								  // REGLA 2: Los Verdes (Tipo 0) disparan desde la Oleada 3
								  if (marcianos[i].tipo == 0 && oleada >= 3) {
									  puedeDisparar = 1;
								  }

								  // Si tiene permiso, tiramos los dados (1% de probabilidad)
								  if (puedeDisparar && (rand() % 100) < 1) {
									  // Buscar bala enemiga libre
									  for(int b=0; b<MAX_BALAS_ENEMIGAS; b++) {
										  if(balasEnemigas[b].activa == 0) {
											  balasEnemigas[b].activa = 1;
											  // La bala sale del centro del marciano
											  balasEnemigas[b].x = marcianos[i].x + (marcianos[i].ancho/2);
											  balasEnemigas[b].y = marcianos[i].y + marcianos[i].alto;
											  break;
										  }
									  }
								  }
							  }
						  }
					  }

					  // --- 6. MOVER BALAS ENEMIGAS Y DETECTAR IMPACTO ---
					  for(int b=0; b<MAX_BALAS_ENEMIGAS; b++) {
						  if(balasEnemigas[b].activa) {
							  ST7796_DrawRect(balasEnemigas[b].x, balasEnemigas[b].y, 3, 6, 0x0000); // Borrar
							  balasEnemigas[b].y += VELOCIDAD_BALA_ENEMIGA; // Caen hacia abajo

							  // Salida de pantalla
							  if(balasEnemigas[b].y > 480) {
								  balasEnemigas[b].activa = 0;
							  }
							  else {
								  // COLISIÓN CON JUGADOR (Hitbox)
								  if (balasEnemigas[b].x >= naveX &&
									  balasEnemigas[b].x <= (naveX + anchoNave) &&
									  balasEnemigas[b].y >= naveY &&
									  balasEnemigas[b].y <= (naveY + altoNave)) {

									  muerteJugador = 1; // ¡TE HAN DADO!
									  balasEnemigas[b].activa = 0;
								  } else {
									  // Pintar Bala ROJA (0xF800)
									  ST7796_DrawRect(balasEnemigas[b].x, balasEnemigas[b].y, 3, 6, 0xF800);
								  }
							  }
						  }
					  }

					  // --- 7. MOVER MARCIANOS ---
					  if (HAL_GetTick() - ultimoMovMarcianos > 50) {
						  ultimoMovMarcianos = HAL_GetTick();
						  uint8_t tocarBorde = 0;

						  for (int i = 0; i < TOTAL_MARCIANOS; i++) {
							  if (marcianos[i].vivo) {
								  if ((direccionMarcianos == 1 && marcianos[i].x > 290) ||
									  (direccionMarcianos == -1 && marcianos[i].x < 10)) {
									  tocarBorde = 1;
								  }
								  // Colisión física (cuerpo a cuerpo)
								  if (marcianos[i].y + marcianos[i].alto >= naveY &&
									  marcianos[i].x + marcianos[i].ancho >= naveX &&
									  marcianos[i].x <= naveX + anchoNave) {
									  muerteJugador = 1;
								  }
							  }
						  }

						  if (tocarBorde) {
							  direccionMarcianos *= -1;
							  for (int i = 0; i < TOTAL_MARCIANOS; i++) {
								  if(marcianos[i].vivo) {
									  ST7796_DrawRect(marcianos[i].x, marcianos[i].y, marcianos[i].ancho, marcianos[i].alto, 0x0000);
									  marcianos[i].y += 4;
									  const uint16_t* sprite = (marcianos[i].tipo == 0) ? marciano1_Verde_16x11 :
															   (marcianos[i].tipo == 1) ? marciano1_Amarillo_16x11 :
															   (marcianos[i].tipo == 2) ? marciano1_Azul_16x11 :
															   (marcianos[i].tipo == 3) ? marciano2_Rojo_18x12 : marciano3_Morado_24x16;
									  ST7796_DrawBitmap(marcianos[i].x, marcianos[i].y, marcianos[i].ancho, marcianos[i].alto, sprite);
								  }
							  }
						  } else {
							  for (int i = 0; i < TOTAL_MARCIANOS; i++) {
								  if(marcianos[i].vivo) {
									  ST7796_DrawRect(marcianos[i].x, marcianos[i].y, marcianos[i].ancho, marcianos[i].alto, 0x0000);
									  marcianos[i].x += (velocidadMarcianos * direccionMarcianos);
									  const uint16_t* sprite = (marcianos[i].tipo == 0) ? marciano1_Verde_16x11 :
															   (marcianos[i].tipo == 1) ? marciano1_Amarillo_16x11 :
															   (marcianos[i].tipo == 2) ? marciano1_Azul_16x11 :
															   (marcianos[i].tipo == 3) ? marciano2_Rojo_18x12 : marciano3_Morado_24x16;
									  ST7796_DrawBitmap(marcianos[i].x, marcianos[i].y, marcianos[i].ancho, marcianos[i].alto, sprite);
								  }
							  }
						  }
					  }

					  // --- 8. GESTIÓN DE MUERTE (Unificada) ---
					  if (muerteJugador) {
						  PlayTone(300, 300);
						  vidas--;
						  ActualizarLEDsVidas(vidas);

						  char bufferVidas[10];
						  sprintf(bufferVidas, "VIDAS: %d", vidas);
						  ST7796_WriteString(10, 10, bufferVidas, 0xFFFF, 0x0000, 1);

						  ST7796_DrawRect(naveX, naveY, anchoNave, altoNave, 0x0000);
						  ST7796_DrawRect(oldNaveX, oldNaveY, anchoNave, altoNave, 0x0000);

						  // Limpiar balas enemigas para que no te maten al reaparecer
						  for(int b=0; b<MAX_BALAS_ENEMIGAS; b++) {
							  if(balasEnemigas[b].activa) {
								   ST7796_DrawRect(balasEnemigas[b].x, balasEnemigas[b].y, 3, 6, 0x0000);
								   balasEnemigas[b].activa = 0;
							  }
						  }

						  if (vidas == 0) {
							  juegoIniciado = 0;
							  estadoActual = ESTADO_DERROTA;
						  } else {
							  naveX = 152; oldNaveX = 152; oldNaveY = 420;
							  calorArma = 0; armaSobrecalentada = 0;
							  ST7796_WriteString(200, 10, "            ", 0x0000, 0x0000, 1);
							  if (naveSeleccionada == 0) ST7796_DrawBitmap(naveX, naveY, 16, 8, nave_16x8);
							  else if (naveSeleccionada == 1) ST7796_DrawBitmap(naveX, naveY, 20, 10, nave_delta_20x10);
							  else if (naveSeleccionada == 2) ST7796_DrawBitmap(naveX, naveY, 16, 10, nave_interceptor_16x10);
							  HAL_Delay(200);
						  }
					  }

					  // --- 9. VICTORIA/OLEADAS ---
					  if(!muerteJugador && marcianosVivos == 0) { // Solo si no has muerto en este frame
						  for(int i=0; i<MAX_BALAS; i++) balas[i].activa = 0;
						  for(int i=0; i<MAX_BALAS_ENEMIGAS; i++) balasEnemigas[i].activa = 0;
						  PlayTone(1000, 100); HAL_Delay(100); PlayTone(2000, 200);

						  oleada++;
						  if (oleada > 3) {
							  juegoIniciado = 0;
							  estadoActual = ESTADO_VICTORIA;
						  }
						  else {
							  char bufferOleada[20];
							  sprintf(bufferOleada, "OLEADA %d", oleada);
							  ST7796_WriteString(100, 220, bufferOleada, 0x07E0, 0x0000, 3);
							  HAL_Delay(2000);
							  ST7796_Fill(0x0000);
							  velocidadMarcianos += 2;

							  int idx = 0; direccionMarcianos = 1; marcianosVivos = TOTAL_MARCIANOS;
							  for(int fila=0; fila < FILAS_MARCIANOS; fila++) {
								  for(int col=0; col < COLS_MARCIANOS; col++) {
									  marcianos[idx].vivo = 1;
									  marcianos[idx].x = 30 + (col * 45);
									  marcianos[idx].y = 40 + (fila * 30);
									  marcianos[idx].tipo = fila;
									  if(fila==0) { marcianos[idx].ancho=16; marcianos[idx].alto=11; }
									  else if(fila==1) { marcianos[idx].ancho=16; marcianos[idx].alto=11; }
									  else if(fila==2) { marcianos[idx].ancho=16; marcianos[idx].alto=11; }
									  else if(fila==3) { marcianos[idx].ancho=18; marcianos[idx].alto=12; }
									  else { marcianos[idx].ancho=24; marcianos[idx].alto=16; }
									  idx++;
								  }
							  }
							  char bufferVidas[10]; sprintf(bufferVidas, "VIDAS: %d", vidas);
							  ST7796_WriteString(10, 10, bufferVidas, 0xFFFF, 0x0000, 1);
							  if (naveSeleccionada == 0) ST7796_DrawBitmap(naveX, naveY, 16, 8, nave_16x8);
							  else if (naveSeleccionada == 1) ST7796_DrawBitmap(naveX, naveY, 20, 10, nave_delta_20x10);
							  else if (naveSeleccionada == 2) ST7796_DrawBitmap(naveX, naveY, 16, 10, nave_interceptor_16x10);
						  }
					  }

					  // --- 10. RENDERIZADO NAVE ---
					  if (!muerteJugador && (naveX != oldNaveX || naveY != oldNaveY)) {
						  ST7796_DrawRect(oldNaveX, oldNaveY, anchoNave, altoNave, 0x0000);
						  if (naveSeleccionada == 0) ST7796_DrawBitmap(naveX, naveY, 16, 8, nave_16x8);
						  else if (naveSeleccionada == 1) ST7796_DrawBitmap(naveX, naveY, 20, 10, nave_delta_20x10);
						  else if (naveSeleccionada == 2) ST7796_DrawBitmap(naveX, naveY, 16, 10, nave_interceptor_16x10);
						  oldNaveX = naveX;
						  oldNaveY = naveY;
					  }

					  // --- 11. SALIDA MANUAL ---
					  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
						  PlayTone(500, 300);
						  juegoIniciado = 0;
						  estadoActual = ESTADO_MENU;
						  ActualizarLEDsVidas(0);
						  HAL_Delay(300);
					  }
					  break;


	              case ESTADO_VICTORIA:
					  static uint8_t victoriaPintada = 0;

					  juegoIniciado = 0;

					  // 1. DIBUJAR Y SONIDO (Solo la primera vez)
					  if (victoriaPintada == 0) {
						  ST7796_Fill(0x0000); // Fondo negro

						  // Texto de Victoria
						  ST7796_WriteString(60, 150, "MISION", 0x07E0, 0x0000, 3); // Verde grande
						  ST7796_WriteString(40, 190, "CUMPLIDA", 0x07E0, 0x0000, 3);

						  // Mostrar vidas restantes como puntuación
						  char bufferScore[20];
						  sprintf(bufferScore, "Vidas Restantes: %d", vidas);
						  ST7796_WriteString(50, 250, bufferScore, 0xFFFF, 0x0000, 1);

						  ST7796_WriteString(60, 400, "PULSA RESET", 0xFFFF, 0x0000, 2);

						  // Melodía de Victoria (Ta-da-da-daaa!)
						  PlayTone(1000, 100); HAL_Delay(100);
						  PlayTone(1500, 100); HAL_Delay(100);
						  PlayTone(2000, 100); HAL_Delay(100);
						  PlayTone(2500, 400);

						  victoriaPintada = 1; // Bloquear para no repetir
					  }

					  // 2. SALIDA (Botón RESET - PB2)
					  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
						  PlayTone(1000, 100); // Confirmación
						  victoriaPintada = 0; // Resetear bandera para la próxima vez

						  // Resetear variables globales importantes
						  vidas = 3;
						  ActualizarLEDsVidas(vidas);
						  naveSeleccionada = 0;

						  estadoActual = ESTADO_MENU; // Volver al inicio
						  HAL_Delay(300); // Anti-rebote
					  }
					  break;

				  case ESTADO_DERROTA:
					  static uint8_t derrotaPintada = 0;

					  juegoIniciado = 0;

					  // 1. DIBUJAR Y SONIDO (Solo la primera vez)
					  if (derrotaPintada == 0) {
						  ST7796_Fill(0x0000); // Fondo negro

						  // Texto de Derrota (Rojo Sangre)
						  ST7796_WriteString(80, 150, "GAME", 0xF800, 0x0000, 3);
						  ST7796_WriteString(80, 190, "OVER", 0xF800, 0x0000, 3);

						  ST7796_WriteString(40, 280, "La flota ha caido...", 0xFFFF, 0x0000, 1);
						  ST7796_WriteString(60, 400, "PULSA RESET", 0xFFFF, 0x0000, 2);

						  // Melodía de Derrota (Triste y descendente)
						  PlayTone(1000, 300); HAL_Delay(300);
						  PlayTone(800, 300);  HAL_Delay(300);
						  PlayTone(600, 300);  HAL_Delay(300);
						  PlayTone(400, 600);  // Womp womp womp...

						  derrotaPintada = 1; // Bloquear
					  }

					  // 2. SALIDA (Botón RESET - PB2)
					  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) {
						  PlayTone(1000, 100);
						  derrotaPintada = 0;

						  // Resetear variables
						  vidas = 3;
						  ActualizarLEDsVidas(vidas);
						  naveSeleccionada = 0;

						  estadoActual = ESTADO_MENU;
						  HAL_Delay(300);
					  }
					  break;
	          }
	          // --- GESTOR DE SONIDO DE FONDO (Fuera del if de 33ms para mayor precisión) ---
	              if (buzzerActivo && HAL_GetTick() >= tiempoFinSonido) {
	                  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
	                  buzzerActivo = 0;
	              }
	      }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RST_Pin|LCD_DC_Pin|LCD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LED_1VIDA_Pin|LED_2VIDA_Pin|LED_3VIDA_Pin
                          |Audio_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RST_Pin LCD_DC_Pin LCD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_DC_Pin|LCD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_START_Pin BTN_FIRE_Pin BTN_MENU_Pin */
  GPIO_InitStruct.Pin = BTN_START_Pin|BTN_FIRE_Pin|BTN_MENU_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LED_1VIDA_Pin LED_2VIDA_Pin LED_3VIDA_Pin
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LED_1VIDA_Pin|LED_2VIDA_Pin|LED_3VIDA_Pin
                          |Audio_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
