/**
 * @file sht31.h
 * @brief STM32WL55 STM32 HAL driver interface for the Sensirion SHT31 sensor.
 *
 * This module reads temperature and relative humidity data from an SHT31 sensor
 * over I2C on STM32WL55 using STM32 HAL. The sensor uses 0x44 as its 7-bit I2C
 * address, passed to STM32 HAL as 0x88 in 8-bit address format.
 *
 * Measurement command: 0x2C 0x06 (High Repeatability, Clock Stretch).
 * CRC: polynomial 0x31, init 0xFF.
 */
#ifndef SHT31_H
#define SHT31_H /**< Include guard for the SHT31 driver header. */

#include <stdint.h>
#include <stddef.h>


#include "stm32wlxx.h"
#include "stm32wlxx_hal.h"


/* Adres */
#define SHT31_ADDR        (0x44 << 1)   /**< SHT31 I2C address in STM32 HAL 8-bit format: 0x88. */

/* Hata kodlari */
#define SHT31_OK          0             /**< Operation completed successfully. */
#define SHT31_ERR_WRITE   1             /**< I2C transmit operation failed. */
#define SHT31_ERR_READ    2             /**< I2C receive operation failed. */
#define SHT31_ERR_CRC     3             /**< CRC check failed for received measurement data. */

/* Veri yapisi */
typedef struct {
    float temperature;                  /**< Temperature value in degrees Celsius. */
    float humidity;                     /**< Relative humidity value in percent. */
} SHT31_Data;

/* Fonksiyon prototipleri */
/**
 * @brief Read temperature and humidity from the SHT31 sensor.
 * @param hi2c Pointer to the STM32 HAL I2C handle.
 * @param data Pointer to the output data structure.
 * @retval SHT31_OK on success, otherwise one of the SHT31_ERR_* codes.
 */
uint8_t SHT31_Read(I2C_HandleTypeDef *hi2c, SHT31_Data *data);

#endif /* SHT31_H */