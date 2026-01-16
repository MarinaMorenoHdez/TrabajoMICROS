#include "peripherals.h"

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

// Variables para sonido no bloqueante
static uint32_t soundEndTime = 0;
static uint8_t buzzerIsActive = 0;

void Peripherals_UpdateLivesLEDs(int lives) {
    // Apagamos todos primero [cite: 35]
    HAL_GPIO_WritePin(GPIOD, LED_1VIDA_Pin|LED_2VIDA_Pin|LED_3VIDA_Pin, GPIO_PIN_RESET);

    if (lives >= 1) HAL_GPIO_WritePin(LED_1VIDA_GPIO_Port, LED_1VIDA_Pin, GPIO_PIN_SET);
    if (lives >= 2) HAL_GPIO_WritePin(LED_2VIDA_GPIO_Port, LED_2VIDA_Pin, GPIO_PIN_SET);
    if (lives >= 3) HAL_GPIO_WritePin(LED_3VIDA_GPIO_Port, LED_3VIDA_Pin, GPIO_PIN_SET);
}

void Peripherals_PlayTone(uint16_t frequency, uint16_t duration) {
    if(frequency == 0) return;
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);

    uint32_t period = 1000000 / frequency;
    __HAL_TIM_SET_AUTORELOAD(&htim2, period);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, period / 2);
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    soundEndTime = HAL_GetTick() + duration;
    buzzerIsActive = 1;
}

void Peripherals_CheckBuzzerTimeout(void) {
    if (buzzerIsActive && HAL_GetTick() >= soundEndTime) {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
        buzzerIsActive = 0;
    }
}

void Peripherals_Beep(uint16_t frequency, uint16_t duration) {
    Peripherals_PlayTone(frequency, duration);
    HAL_Delay(duration); // Versión bloqueante para menús
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
}

uint32_t Peripherals_ReadJoystick(void) {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    return HAL_ADC_GetValue(&hadc1);
}


