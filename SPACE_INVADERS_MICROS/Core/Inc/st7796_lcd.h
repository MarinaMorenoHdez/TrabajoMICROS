#ifndef ST7796_LCD_H
#define ST7796_LCD_H

#include "main.h" // Para acceder a los HAL y definiciones de pines

// --- Definiciones de comandos y macros ---
#define LCD_CS_LOW()    HAL_GPIO_WritePin(GPIOA, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_HIGH()   HAL_GPIO_WritePin(GPIOA, LCD_CS_Pin, GPIO_PIN_SET)
#define LCD_DC_CMD()    HAL_GPIO_WritePin(GPIOA, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_DATA()   HAL_GPIO_WritePin(GPIOA, LCD_DC_Pin, GPIO_PIN_SET)
#define LCD_RST_LOW()   HAL_GPIO_WritePin(GPIOA, LCD_RST_Pin, GPIO_PIN_RESET)
#define LCD_RST_HIGH()  HAL_GPIO_WritePin(GPIOA, LCD_RST_Pin, GPIO_PIN_SET)

// --- Prototipos de funciones ---
void ST7796_Init(void);
void ST7796_Fill(uint16_t color);
void ST7796_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7796_DrawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap);
void ST7796_WriteString(uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bg, uint8_t size);
void ST7796_DrawBitmapZoom(int x, int y, int w, int h, const uint16_t* bitmap, int zoom);

#endif
