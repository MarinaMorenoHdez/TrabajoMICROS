#include "st7796_lcd.h"
#include "fonts.h"
#include <stdio.h>

// Referencias a los periféricos definidos en el main.c
extern SPI_HandleTypeDef hspi1;

// --- FUNCIONES PRIVADAS (Auxiliares del driver) ---

/**
 * @brief Envía un comando al controlador ST7796
 */
static void ST7796_Cmd(uint8_t cmd) {
    LCD_DC_CMD();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    LCD_CS_HIGH();
}

/**
 * @brief Envía un dato al controlador ST7796
 */
static void ST7796_Data(uint8_t data) {
    LCD_DC_DATA();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &data, 1, 10);
    LCD_CS_HIGH();
}

/**
 * @brief Define la ventana de dibujo en la pantalla
 */
void ST7796_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    ST7796_Cmd(0x2A);
    uint8_t dataX[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    LCD_DC_DATA();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi1, dataX, 4, 10);
    LCD_CS_HIGH();

    ST7796_Cmd(0x2B);
    uint8_t dataY[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
    LCD_DC_DATA();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi1, dataY, 4, 10);
    LCD_CS_HIGH();

    ST7796_Cmd(0x2C);
}

// --- FUNCIONES PÚBLICAS (Las que usará el juego) ---

void ST7796_Init(void) {
    LCD_CS_HIGH();
    LCD_RST_LOW();
    HAL_Delay(100);
    LCD_RST_HIGH();
    HAL_Delay(100);

    ST7796_Cmd(0x01); // Software reset
    HAL_Delay(120);

    ST7796_Cmd(0x11); // Sleep out
    HAL_Delay(120);

    ST7796_Cmd(0x3A); // Interface pixel format
    ST7796_Data(0x55); // 16-bit color

    ST7796_Cmd(0x36); // Memory Access Control
    ST7796_Data(0x48);

    ST7796_Cmd(0x29); // Display ON
    HAL_Delay(10);
}

void ST7796_Fill(uint16_t color) {
    ST7796_SetWindow(0, 0, 319, 479);
    uint8_t colorBytes[2] = {color >> 8, color & 0xFF};
    LCD_DC_DATA();
    LCD_CS_LOW();
    for(long i = 0; i < (320 * 480); i++) {
        HAL_SPI_Transmit(&hspi1, colorBytes, 2, 10);
    }
    LCD_CS_HIGH();
}

void ST7796_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if((x+w) > 320 || (y+h) > 480) return;
    ST7796_SetWindow(x, y, x + w - 1, y + h - 1);
    uint8_t colorBytes[2] = {color >> 8, color & 0xFF};
    LCD_DC_DATA();
    LCD_CS_LOW();
    for(uint32_t i = 0; i < (w * h); i++) {
        HAL_SPI_Transmit(&hspi1, colorBytes, 2, 10);
    }
    LCD_CS_HIGH();
}

void ST7796_DrawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *bitmap) {
    if((x+w) > 320 || (y+h) > 480) return;
    ST7796_SetWindow(x, y, x + w - 1, y + h - 1);
    LCD_DC_DATA();
    LCD_CS_LOW();
    for(uint32_t i = 0; i < (w * h); i++) {
        uint16_t color = bitmap[i];
        uint8_t colorBytes[2] = {color >> 8, color & 0xFF};
        HAL_SPI_Transmit(&hspi1, colorBytes, 2, 10);
    }
    LCD_CS_HIGH();
}

void ST7796_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if((x+7*size) > 320 || (y+10*size) > 480) return;
    if(c < 32 || c > 127) return;
    const uint16_t *pChar = &Font7x10[(c - 32) * 10];

    ST7796_SetWindow(x, y, x + (7*size) - 1, y + (10*size) - 1);
    LCD_DC_DATA();
    LCD_CS_LOW();

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

void ST7796_DrawBitmapZoom(int x, int y, int w, int h, const uint16_t* bitmap, int zoom) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            uint16_t color = bitmap[j * w + i];
            if (color != 0x0000) {
                ST7796_DrawRect(x + (i * zoom), y + (j * zoom), zoom, zoom, color);
            }
        }
    }
}

