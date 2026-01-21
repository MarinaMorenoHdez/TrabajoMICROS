#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "main.h"

// --- LEDs de Vidas ---
void Peripherals_UpdateLivesLEDs(int lives);

// --- Sonido (Buzzer) ---
void Peripherals_PlayTone(uint16_t frequency, uint16_t duration);
void Peripherals_CheckBuzzerTimeout(void);
void Peripherals_Beep(uint16_t frequency, uint16_t duration);

// --- Entradas ---
uint32_t Peripherals_ReadJoystick(void);
// La interrupción se gestiona automáticamente por el hardware

#endif
