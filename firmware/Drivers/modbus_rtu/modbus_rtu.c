#include "modbus_rtu.h"

#define MODBUS_FC_READ_HOLDING_REGISTERS    (0x03U)
#define MODBUS_FC_READ_INPUT_REGISTERS      (0x04U)
#define MODBUS_EXCEPTION_MASK               (0x80U)
#define MODBUS_REQUEST_SIZE                 (8U)
#define MODBUS_RESPONSE_HEADER_SIZE         (3U)
#define MODBUS_EXCEPTION_RESPONSE_SIZE      (5U)
#define MODBUS_CRC_INITIAL_VALUE            (0xFFFFU)
#define MODBUS_CRC_POLYNOMIAL               (0xA001U)
#define MODBUS_CRC_LSB_MASK                 (0x0001U)
#define MODBUS_BYTE_MASK                    (0x00FFU)
#define MODBUS_BITS_PER_BYTE                (8U)
#define MODBUS_BYTES_PER_REGISTER           (2U)
#define MODBUS_MAX_RESPONSE_SIZE            (MODBUS_RESPONSE_HEADER_SIZE + \
                                             (MODBUS_MAX_READ_REGISTERS * MODBUS_BYTES_PER_REGISTER) + \
                                             MODBUS_RTU_CRC_SIZE)

/**
 * @brief Set MAX485 transceiver to transmit mode.
 * @param hmodbus Pointer to an initialized Modbus handle.
 */
static inline void dir_set_tx(const ModbusHandle_t *hmodbus)
{
    HAL_GPIO_WritePin(hmodbus->dir_port, hmodbus->dir_pin, GPIO_PIN_SET);
}

/**
 * @brief Set MAX485 transceiver to receive mode.
 * @param hmodbus Pointer to an initialized Modbus handle.
 */
static inline void dir_set_rx(const ModbusHandle_t *hmodbus)
{
    HAL_GPIO_WritePin(hmodbus->dir_port, hmodbus->dir_pin, GPIO_PIN_RESET);
}

/**
 * @brief Calculate Modbus RTU CRC16 using polynomial 0xA001.
 * @param data Pointer to the byte buffer.
 * @param length Number of bytes in the byte buffer.
 * @return Calculated CRC value, low byte transmitted first on the wire.
 */
static uint16_t modbus_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = MODBUS_CRC_INITIAL_VALUE;
    uint16_t index;
    uint8_t bit;

    for (index = 0U; index < length; index++)
    {
        crc ^= (uint16_t)data[index];

        for (bit = 0U; bit < MODBUS_BITS_PER_BYTE; bit++)
        {
            if ((crc & MODBUS_CRC_LSB_MASK) != 0U)
            {
                crc = (uint16_t)((crc >> 1U) ^ MODBUS_CRC_POLYNOMIAL);
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

/**
 * @brief Validate common Modbus read-register arguments.
 * @param hmodbus Pointer to an initialized Modbus handle.
 * @param slave_addr Slave address in range 1..247.
 * @param count Number of registers to read.
 * @param buf Destination buffer for received registers.
 * @param buf_size Number of uint16_t elements available in buf.
 * @return MODBUS_OK when arguments are valid, otherwise MODBUS_ERR_INVALID_ARG.
 */
static ModbusStatus_t validate_read_args(const ModbusHandle_t *hmodbus,
                                         uint8_t slave_addr,
                                         uint16_t count,
                                         const uint16_t *buf,
                                         uint16_t buf_size)
{
    ModbusStatus_t status = MODBUS_OK;

    if ((hmodbus == NULL) ||
        (hmodbus->huart == NULL) ||
        (hmodbus->dir_port == NULL) ||
        (buf == NULL))
    {
        status = MODBUS_ERR_INVALID_ARG;
    }
    else if ((slave_addr < MODBUS_MIN_SLAVE_ADDR) ||
             (slave_addr > MODBUS_MAX_SLAVE_ADDR) ||
             (slave_addr == MODBUS_BROADCAST_ADDR))
    {
        status = MODBUS_ERR_INVALID_ARG;
    }
    else if ((count == 0U) ||
             (count > MODBUS_MAX_READ_REGISTERS) ||
             (buf_size < count))
    {
        status = MODBUS_ERR_INVALID_ARG;
    }
    else
    {
        status = MODBUS_OK;
    }

    return status;
}

/**
 * @brief Append Modbus CRC low byte and high byte to a frame.
 * @param frame Pointer to frame buffer.
 * @param payload_length Frame length before CRC bytes.
 */
static void append_crc(uint8_t *frame, uint16_t payload_length)
{
    const uint16_t crc = modbus_crc16(frame, payload_length);

    frame[payload_length] = (uint8_t)(crc & MODBUS_BYTE_MASK);
    frame[payload_length + 1U] = (uint8_t)((crc >> 8U) & MODBUS_BYTE_MASK);
}

/**
 * @brief Verify the CRC bytes at the end of a Modbus frame.
 * @param frame Pointer to the received frame.
 * @param frame_length Total frame length including two CRC bytes.
 * @return MODBUS_OK when CRC is valid, otherwise MODBUS_ERR_CRC.
 */
static ModbusStatus_t verify_crc(const uint8_t *frame, uint16_t frame_length)
{
    const uint16_t payload_length = (uint16_t)(frame_length - MODBUS_RTU_CRC_SIZE);
    const uint16_t received_crc = (uint16_t)frame[payload_length] |
                                  ((uint16_t)frame[payload_length + 1U] << 8U);
    const uint16_t calculated_crc = modbus_crc16(frame, payload_length);

    return (received_crc == calculated_crc) ? MODBUS_OK : MODBUS_ERR_CRC;
}

/**
 * @brief Wait until the UART transmission complete flag is set.
 * @param hmodbus Pointer to an initialized Modbus handle.
 * @return MODBUS_OK when TC is observed, otherwise MODBUS_ERR_TIMEOUT.
 */
static ModbusStatus_t wait_uart_tc(const ModbusHandle_t *hmodbus)
{
    const uint32_t started_at = HAL_GetTick();
    ModbusStatus_t status = MODBUS_OK;

    while (__HAL_UART_GET_FLAG(hmodbus->huart, UART_FLAG_TC) == RESET)
    {
        if ((HAL_GetTick() - started_at) >= hmodbus->timeout_ms)
        {
            status = MODBUS_ERR_TIMEOUT;
            break;
        }
    }

    return status;
}

/**
 * @brief Send a complete Modbus RTU frame and return transceiver to RX mode.
 * @param hmodbus Pointer to an initialized Modbus handle.
 * @param frame Pointer to frame bytes.
 * @param frame_length Number of bytes to transmit.
 * @return MODBUS_OK on success, otherwise MODBUS_ERR_TIMEOUT.
 */
static ModbusStatus_t transmit_frame(ModbusHandle_t *hmodbus,
                                     const uint8_t *frame,
                                     uint16_t frame_length)
{
    HAL_StatusTypeDef hal_status;
    ModbusStatus_t status = MODBUS_OK;

    dir_set_tx(hmodbus);
    hal_status = HAL_UART_Transmit(hmodbus->huart,
                                   (uint8_t *)frame,
                                   frame_length,
                                   hmodbus->timeout_ms);

    if (hal_status == HAL_OK)
    {
        status = wait_uart_tc(hmodbus);
    }

    dir_set_rx(hmodbus);

    if (hal_status != HAL_OK)
    {
        status = MODBUS_ERR_TIMEOUT;
    }

    return status;
}

/**
 * @brief Build and execute a Modbus read-register transaction.
 * @param hmodbus Pointer to an initialized Modbus handle.
 * @param function_code Modbus function code, 03 or 04.
 * @param slave_addr Slave address in range 1..247.
 * @param start_reg First register address to read.
 * @param count Number of registers to read.
 * @param buf Destination buffer for received registers.
 * @param buf_size Number of uint16_t elements available in buf.
 * @return MODBUS_OK on success, otherwise an error status.
 */
static ModbusStatus_t read_registers(ModbusHandle_t *hmodbus,
                                     uint8_t function_code,
                                     uint8_t slave_addr,
                                     uint16_t start_reg,
                                     uint16_t count,
                                     uint16_t *buf,
                                     uint16_t buf_size)
{
    uint8_t request[MODBUS_REQUEST_SIZE];
    uint8_t response[MODBUS_MAX_RESPONSE_SIZE];
    const uint8_t expected_byte_count = (uint8_t)(count * MODBUS_BYTES_PER_REGISTER);
    const uint16_t normal_tail_size = (uint16_t)(expected_byte_count + MODBUS_RTU_CRC_SIZE);
    const uint16_t normal_response_size = (uint16_t)(MODBUS_RESPONSE_HEADER_SIZE + normal_tail_size);
    HAL_StatusTypeDef hal_status;
    ModbusStatus_t status;
    uint16_t index;

    status = validate_read_args(hmodbus, slave_addr, count, buf, buf_size);

    if (status == MODBUS_OK)
    {
        request[0] = slave_addr;
        request[1] = function_code;
        request[2] = (uint8_t)((start_reg >> 8U) & MODBUS_BYTE_MASK);
        request[3] = (uint8_t)(start_reg & MODBUS_BYTE_MASK);
        request[4] = (uint8_t)((count >> 8U) & MODBUS_BYTE_MASK);
        request[5] = (uint8_t)(count & MODBUS_BYTE_MASK);
        append_crc(request, (uint16_t)(MODBUS_REQUEST_SIZE - MODBUS_RTU_CRC_SIZE));

        status = transmit_frame(hmodbus, request, MODBUS_REQUEST_SIZE);
    }

    if (status == MODBUS_OK)
    {
        hal_status = HAL_UART_Receive(hmodbus->huart,
                                      response,
                                      MODBUS_RESPONSE_HEADER_SIZE,
                                      hmodbus->timeout_ms);

        if (hal_status != HAL_OK)
        {
            status = MODBUS_ERR_TIMEOUT;
        }
    }

    if (status == MODBUS_OK)
    {
        if (response[0] != slave_addr)
        {
            status = MODBUS_ERR_INVALID_ARG;
        }
        else if ((response[1] & MODBUS_EXCEPTION_MASK) != 0U)
        {
            hal_status = HAL_UART_Receive(hmodbus->huart,
                                          &response[MODBUS_RESPONSE_HEADER_SIZE],
                                          MODBUS_RTU_CRC_SIZE,
                                          hmodbus->timeout_ms);

            if (hal_status != HAL_OK)
            {
                status = MODBUS_ERR_TIMEOUT;
            }
            else if (verify_crc(response, MODBUS_EXCEPTION_RESPONSE_SIZE) != MODBUS_OK)
            {
                status = MODBUS_ERR_CRC;
            }
            else
            {
                status = MODBUS_ERR_EXCEPTION;
            }
        }
        else if ((response[1] != function_code) || (response[2] != expected_byte_count))
        {
            status = MODBUS_ERR_INVALID_ARG;
        }
        else
        {
            hal_status = HAL_UART_Receive(hmodbus->huart,
                                          &response[MODBUS_RESPONSE_HEADER_SIZE],
                                          normal_tail_size,
                                          hmodbus->timeout_ms);

            if (hal_status != HAL_OK)
            {
                status = MODBUS_ERR_TIMEOUT;
            }
            else
            {
                status = verify_crc(response, normal_response_size);
            }
        }
    }

    if (status == MODBUS_OK)
    {
        for (index = 0U; index < count; index++)
        {
            const uint16_t msb_index = (uint16_t)(MODBUS_RESPONSE_HEADER_SIZE +
                                                 (index * MODBUS_BYTES_PER_REGISTER));
            const uint16_t lsb_index = (uint16_t)(msb_index + 1U);

            buf[index] = (uint16_t)(((uint16_t)response[msb_index] << 8U) |
                                    (uint16_t)response[lsb_index]);
        }
    }

    return status;
}

ModbusStatus_t modbus_init(ModbusHandle_t *hmodbus,
                           UART_HandleTypeDef *huart,
                           GPIO_TypeDef *dir_port,
                           uint16_t dir_pin,
                           uint32_t timeout_ms)
{
    ModbusStatus_t status = MODBUS_OK;

    if ((hmodbus == NULL) || (huart == NULL) || (dir_port == NULL) || (timeout_ms == 0U))
    {
        status = MODBUS_ERR_INVALID_ARG;
    }
    else
    {
        hmodbus->huart = huart;
        hmodbus->dir_port = dir_port;
        hmodbus->dir_pin = dir_pin;
        hmodbus->timeout_ms = timeout_ms;
        dir_set_rx(hmodbus);
    }

    return status;
}

ModbusStatus_t modbus_read_holding_registers(ModbusHandle_t *hmodbus,
                                             uint8_t slave_addr,
                                             uint16_t start_reg,
                                             uint16_t count,
                                             uint16_t *buf,
                                             uint16_t buf_size)
{
    return read_registers(hmodbus,
                          MODBUS_FC_READ_HOLDING_REGISTERS,
                          slave_addr,
                          start_reg,
                          count,
                          buf,
                          buf_size);
}

ModbusStatus_t modbus_read_input_registers(ModbusHandle_t *hmodbus,
                                           uint8_t slave_addr,
                                           uint16_t start_reg,
                                           uint16_t count,
                                           uint16_t *buf,
                                           uint16_t buf_size)
{
    return read_registers(hmodbus,
                          MODBUS_FC_READ_INPUT_REGISTERS,
                          slave_addr,
                          start_reg,
                          count,
                          buf,
                          buf_size);
}
