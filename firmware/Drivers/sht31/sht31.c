/**
 * @file sht31.c
 * @brief STM32WL55 STM32 HAL driver implementation for the Sensirion SHT31 sensor.
 *
 * This module sends the SHT31 high repeatability, clock stretching measurement
 * command over I2C and converts the received raw temperature and humidity data.
 * The sensor uses 0x44 as its 7-bit I2C address, passed to STM32 HAL as 0x88
 * in 8-bit address format.
 *
 * Measurement command: 0x2C 0x06 (High Repeatability, Clock Stretch).
 * CRC: polynomial 0x31, init 0xFF.
 */
#include "sht31.h"

/* Olcum komutu: High Repeatability, Clock Stretch */
static const uint8_t SHT31_CMD[2] = {0x2C, 0x06};

/**
 * @brief Calculate SHT31 CRC-8 for received data bytes.
 * @param data Pointer to the data buffer.
 * @param len Number of bytes to include in the CRC calculation.
 * @retval Calculated CRC-8 value.
 */
static uint8_t SHT31_CRC(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    uint8_t i;
    uint8_t b;

    for (i = 0; i < len; i++)
    {
        crc ^= data[i];

        for (b = 0; b < 8; b++)
        {
            if ((crc & 0x80U) != 0U)
            {
                crc = (uint8_t)((crc << 1U) ^ 0x31U);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    return crc;
}

/**
 * @brief Read temperature and humidity from the SHT31 sensor.
 * @param hi2c Pointer to the STM32 HAL I2C handle.
 * @param data Pointer to the output data structure.
 * @retval SHT31_OK on success, otherwise one of the SHT31_ERR_* codes.
 */
uint8_t SHT31_Read(I2C_HandleTypeDef *hi2c, SHT31_Data *data)
{
    uint8_t raw[6];
    HAL_StatusTypeDef ret;
    uint8_t status = SHT31_OK;

    ret = HAL_I2C_Master_Transmit(hi2c, SHT31_ADDR,
                                  (uint8_t *)SHT31_CMD, 2,
                                  HAL_MAX_DELAY);

    if (ret != HAL_OK)
    {
        status = SHT31_ERR_WRITE;
    }
    else
    {
        HAL_Delay(20);

        ret = HAL_I2C_Master_Receive(hi2c, SHT31_ADDR,
                                     raw, 6,
                                     HAL_MAX_DELAY);

        if (ret != HAL_OK)
        {
            status = SHT31_ERR_READ;
        }
        else
        {
            if ((SHT31_CRC(&raw[0], 2) != raw[2]) ||
                (SHT31_CRC(&raw[3], 2) != raw[5]))
            {
                status = SHT31_ERR_CRC;
            }
            else
            {
                uint16_t raw_temp = ((uint16_t)raw[0] << 8) | raw[1];
                uint16_t raw_hum  = ((uint16_t)raw[3] << 8) | raw[4];

                data->temperature = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);
                data->humidity    = 100.0f * ((float)raw_hum  / 65535.0f);
            }
        }
    }

    return status;
}