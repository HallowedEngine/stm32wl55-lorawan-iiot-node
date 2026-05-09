#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "stm32wlxx_hal.h"

#define MODBUS_MAX_PDU_SIZE              (253U)
#define MODBUS_MAX_ADU_SIZE              (256U)
#define MODBUS_BROADCAST_ADDR            (0U)
#define MODBUS_MIN_SLAVE_ADDR            (1U)
#define MODBUS_MAX_SLAVE_ADDR            (247U)
#define MODBUS_MAX_READ_REGISTERS        (125U)
#define MODBUS_RTU_CRC_SIZE              (2U)

typedef enum
{
    MODBUS_OK = 0,
    MODBUS_ERR_TIMEOUT,
    MODBUS_ERR_CRC,
    MODBUS_ERR_EXCEPTION,
    MODBUS_ERR_INVALID_ARG
} ModbusStatus_t;

typedef struct
{
    UART_HandleTypeDef *huart;
    GPIO_TypeDef *dir_port;
    uint16_t dir_pin;
    uint32_t timeout_ms;
} ModbusHandle_t;

/**
 * @brief Initialize a Modbus RTU master handle.
 * @param hmodbus Pointer to the Modbus handle to initialize.
 * @param huart Pointer to the STM32 HAL UART handle.
 * @param dir_port GPIO port used for MAX485 DE/RE direction control.
 * @param dir_pin GPIO pin used for MAX485 DE/RE direction control.
 * @param timeout_ms UART transmit and receive timeout in milliseconds.
 * @return MODBUS_OK on success, otherwise MODBUS_ERR_INVALID_ARG.
 */
ModbusStatus_t modbus_init(ModbusHandle_t *hmodbus,
                           UART_HandleTypeDef *huart,
                           GPIO_TypeDef *dir_port,
                           uint16_t dir_pin,
                           uint32_t timeout_ms);

/**
 * @brief Read holding registers using Modbus function code 03.
 * @param hmodbus Pointer to an initialized Modbus handle.
 * @param slave_addr Slave address in range 1..247.
 * @param start_reg First register address to read.
 * @param count Number of registers to read.
 * @param buf Destination buffer for received registers.
 * @param buf_size Number of uint16_t elements available in buf.
 * @return MODBUS_OK on success, otherwise an error status.
 */
ModbusStatus_t modbus_read_holding_registers(ModbusHandle_t *hmodbus,
                                             uint8_t slave_addr,
                                             uint16_t start_reg,
                                             uint16_t count,
                                             uint16_t *buf,
                                             uint16_t buf_size);

/**
 * @brief Read input registers using Modbus function code 04.
 * @param hmodbus Pointer to an initialized Modbus handle.
 * @param slave_addr Slave address in range 1..247.
 * @param start_reg First register address to read.
 * @param count Number of registers to read.
 * @param buf Destination buffer for received registers.
 * @param buf_size Number of uint16_t elements available in buf.
 * @return MODBUS_OK on success, otherwise an error status.
 */
ModbusStatus_t modbus_read_input_registers(ModbusHandle_t *hmodbus,
                                           uint8_t slave_addr,
                                           uint16_t start_reg,
                                           uint16_t count,
                                           uint16_t *buf,
                                           uint16_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_RTU_H */
