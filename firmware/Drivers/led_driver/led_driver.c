/**
 * @file led_driver.c
 * @brief STM32WL55 NUCLEO-WL55JC karti icin RGB LED surucu uygulamasi.
 *
 * Bu dosya, STM32 HAL GPIO API'si ile NUCLEO-WL55JC uzerindeki LED'lerin
 * baslatilmasini ve acma, kapama, durum degistirme islemlerini gerceklestirir.
 *
 * LED pinleri:
 * - PB15: Mavi LED
 * - PB9 : Yesil LED
 * - PB11: Kirmizi LED
 */

#include "led_driver.h"

/**
 * @brief LED GPIO pinlerini cikis olarak baslatir.
 * @retval None
 */
void LED_Init(void) {
    // GPIO Clock'u aç
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // GPIO ayarları
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = LED_BLUE_PIN | LED_GREEN_PIN | LED_RED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Başlangıçta tüm LED'leri kapat
    LED_All_Off();
}

/**
 * @brief Mavi LED'i yakar.
 * @retval None
 */
void LED_Blue_On(void) {
    HAL_GPIO_WritePin(LED_BLUE_PORT, LED_BLUE_PIN, GPIO_PIN_SET);
}

/**
 * @brief Mavi LED'i sondurur.
 * @retval None
 */
void LED_Blue_Off(void) {
    HAL_GPIO_WritePin(LED_BLUE_PORT, LED_BLUE_PIN, GPIO_PIN_RESET);
}

/**
 * @brief Mavi LED'in durumunu degistirir.
 * @retval None
 */
void LED_Blue_Toggle(void) {
    HAL_GPIO_TogglePin(LED_BLUE_PORT, LED_BLUE_PIN);
}

/**
 * @brief Yesil LED'i yakar.
 * @retval None
 */
void LED_Green_On(void) {
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
}

/**
 * @brief Yesil LED'i sondurur.
 * @retval None
 */
void LED_Green_Off(void) {
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
}

/**
 * @brief Yesil LED'in durumunu degistirir.
 * @retval None
 */
void LED_Green_Toggle(void) {
    HAL_GPIO_TogglePin(LED_GREEN_PORT, LED_GREEN_PIN);
}

/**
 * @brief Kirmizi LED'i yakar.
 * @retval None
 */
void LED_Red_On(void) {
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
}

/**
 * @brief Kirmizi LED'i sondurur.
 * @retval None
 */
void LED_Red_Off(void) {
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
}

/**
 * @brief Kirmizi LED'in durumunu degistirir.
 * @retval None
 */
void LED_Red_Toggle(void) {
    HAL_GPIO_TogglePin(LED_RED_PORT, LED_RED_PIN);
}

/**
 * @brief Tum LED'leri sondurur.
 * @retval None
 */
void LED_All_Off(void) {
    LED_Blue_Off();
    LED_Green_Off();
    LED_Red_Off();
}
