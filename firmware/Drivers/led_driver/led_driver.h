#ifndef INC_LED_DRIVER_H_
#define INC_LED_DRIVER_H_

/**
 * @file led_driver.h
 * @brief STM32WL55 NUCLEO-WL55JC karti icin RGB LED surucu arayuzu.
 *
 * Bu driver, STM32 HAL GPIO API'sini kullanarak NUCLEO-WL55JC uzerindeki
 * kullanici LED'lerini kontrol etmek icin gerekli pin tanimlarini ve fonksiyon
 * prototiplerini saglar.
 *
 * LED pinleri:
 * - PB15: Mavi LED
 * - PB9 : Yesil LED
 * - PB11: Kirmizi LED
 */

#include "stm32wlxx_hal.h"

// LED Tanımları (Nucleo-WL55)
#define LED_BLUE_PIN     GPIO_PIN_15  /**< Mavi LED pini: PB15. */
#define LED_BLUE_PORT    GPIOB        /**< Mavi LED GPIO portu: GPIOB. */

#define LED_GREEN_PIN    GPIO_PIN_9   /**< Yesil LED pini: PB9. */
#define LED_GREEN_PORT   GPIOB        /**< Yesil LED GPIO portu: GPIOB. */

#define LED_RED_PIN      GPIO_PIN_11  /**< Kirmizi LED pini: PB11. */
#define LED_RED_PORT     GPIOB        /**< Kirmizi LED GPIO portu: GPIOB. */

// Fonksiyon Prototipleri
/**
 * @brief LED GPIO pinlerini cikis olarak baslatir.
 * @retval None
 */
void LED_Init(void);

/**
 * @brief Mavi LED'i yakar.
 * @retval None
 */
void LED_Blue_On(void);

/**
 * @brief Mavi LED'i sondurur.
 * @retval None
 */
void LED_Blue_Off(void);

/**
 * @brief Mavi LED'in durumunu degistirir.
 * @retval None
 */
void LED_Blue_Toggle(void);

/**
 * @brief Yesil LED'i yakar.
 * @retval None
 */
void LED_Green_On(void);

/**
 * @brief Yesil LED'i sondurur.
 * @retval None
 */
void LED_Green_Off(void);

/**
 * @brief Yesil LED'in durumunu degistirir.
 * @retval None
 */
void LED_Green_Toggle(void);

/**
 * @brief Kirmizi LED'i yakar.
 * @retval None
 */
void LED_Red_On(void);

/**
 * @brief Kirmizi LED'i sondurur.
 * @retval None
 */
void LED_Red_Off(void);

/**
 * @brief Kirmizi LED'in durumunu degistirir.
 * @retval None
 */
void LED_Red_Toggle(void);

/**
 * @brief Tum LED'leri sondurur.
 * @retval None
 */
void LED_All_Off(void);

#endif
