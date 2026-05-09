# STM32WL55 LoRaWAN IIoT Node

[![CI](https://github.com/HallowedEngine/stm32wl55-lorawan-iiot-node/actions/workflows/ci.yml/badge.svg)](https://github.com/HallowedEngine/stm32wl55-lorawan-iiot-node/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: STM32WL55](https://img.shields.io/badge/Platform-STM32WL55-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32wl55jc.html)

## Overview

STM32WL55 LoRaWAN IIoT Node is an end-to-end industrial telemetry project built around a bare-metal STM32 HAL firmware stack, LoRaWAN OTAA connectivity, and a Python backend. The firmware reads environmental data from an SHT31 sensor, communicates with RS485 Modbus RTU slave devices, stores records in W25Q16JV SPI flash, and uplinks telemetry through a Chirpstack v4 LoRaWAN network. The backend consumes MQTT messages, writes time-series data to InfluxDB 2.x, evaluates alarm rules, and exposes dashboards through Grafana 10.x. It is useful for embedded developers, IIoT prototyping teams, and engineers building low-power wireless monitoring nodes.

## System Architecture

```text
STM32WL55 -> (LoRa RF) -> LoRaWAN Gateway -> Chirpstack v4 -> MQTT Broker -> Python Backend -> InfluxDB -> Grafana
```

## Hardware Requirements

| Component          | Part                         | Notes                                      |
| ------------------ | ---------------------------- | ------------------------------------------ |
| MCU board          | NUCLEO-WL55JC or custom board | STM32WL55 LoRa-capable target hardware     |
| Temperature sensor | SHT31 breakout               | I2C temperature and humidity sensor        |
| RS485 transceiver  | MAX485 module                | Half-duplex RS485 interface for Modbus RTU |
| Test device        | RS485 slave device           | Used to validate Modbus master polling     |

## Repository Structure

```text
stm32wl55-lorawan-iiot-node/
|-- .github/workflows/ci.yml              # CI pipeline for firmware and backend checks
|-- backend/                              # Python MQTT subscriber, InfluxDB writer, and alarm engine
|   |-- alarm_engine.py                   # Telemetry alarm rule evaluation
|   |-- config.py                         # Backend environment configuration
|   |-- influxdb_writer.py                # InfluxDB 2.x write helper
|   |-- mqtt_subscriber.py                # MQTT consumer entry point
|   |-- Dockerfile                        # Backend container image
|   `-- requirements.txt                  # Python runtime and tooling dependencies
|-- dashboard/grafana/dashboard.json      # Grafana dashboard definition
|-- devops/                               # Service configuration for local infrastructure
|   |-- grafana/provisioning/             # Grafana datasource provisioning
|   `-- mosquitto/mosquitto.conf          # Mosquitto MQTT broker configuration
|-- firmware/                             # STM32WL55 bare-metal firmware sources
|   `-- Drivers/                          # STM32 peripheral and device drivers
|       |-- modbus_rtu/                    # Modbus RTU master over RS485
|       |-- sht31/                         # SHT31 I2C temperature and humidity driver
|       `-- w25q16jv/                     # W25Q16JV SPI flash driver
|-- .env.example                          # Default backend and service environment variables
`-- docker-compose.yml                    # Mosquitto, InfluxDB, Grafana, and backend stack
```

## Firmware Setup

1. Install [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) with STM32WL series device support.
2. Clone this repository and open STM32CubeIDE.
3. Select **File > Import > Existing Projects into Workspace**.
4. Choose the `firmware/` directory as the project root and complete the import.
5. Build the firmware with **Project > Build Project**.
6. Connect the NUCLEO-WL55JC or custom STM32WL55 board over ST-LINK.
7. Flash the target with **Run > Debug** or **Run > Run**.

## Backend Setup

1. Install Docker and Docker Compose.
2. Create a local environment file:

   ```bash
   cp .env.example .env
   ```

3. Edit `.env` and change the default tokens and passwords before running the stack.
4. Start the services:

   ```bash
   docker compose up -d
   ```

5. Open the local service URLs:

   | Service  | URL                                            |
   | -------- | ---------------------------------------------- |
   | InfluxDB | [http://localhost:8086](http://localhost:8086) |
   | Grafana  | [http://localhost:3000](http://localhost:3000) |
   | MQTT     | `localhost:1883`                               |

## Configuration

| Variable                            | Default                 | Description                                      |
| ----------------------------------- | ----------------------- | ------------------------------------------------ |
| `MQTT_HOST`                         | `mosquitto`             | MQTT broker hostname used by the backend         |
| `MQTT_PORT`                         | `1883`                  | MQTT broker port                                 |
| `INFLUXDB_URL`                      | `http://influxdb:8086`  | InfluxDB HTTP API URL inside Docker Compose      |
| `INFLUXDB_TOKEN`                    | `changeme`              | Token used by the backend and Grafana datasource |
| `INFLUXDB_ORG`                      | `iiot`                  | InfluxDB organization name                       |
| `INFLUXDB_BUCKET`                   | `telemetry`             | InfluxDB bucket for telemetry and alarms         |
| `DOCKER_INFLUXDB_INIT_MODE`         | `setup`                 | Enables first-run InfluxDB initialization        |
| `DOCKER_INFLUXDB_INIT_USERNAME`     | `admin`                 | Initial InfluxDB admin username                  |
| `DOCKER_INFLUXDB_INIT_PASSWORD`     | `changeme123`           | Initial InfluxDB admin password                  |
| `DOCKER_INFLUXDB_INIT_ORG`          | `iiot`                  | Initial InfluxDB organization                    |
| `DOCKER_INFLUXDB_INIT_BUCKET`       | `telemetry`             | Initial InfluxDB bucket                          |
| `DOCKER_INFLUXDB_INIT_ADMIN_TOKEN`  | `changeme`              | Initial InfluxDB admin token                     |
| `GF_SECURITY_ADMIN_PASSWORD`        | `changeme`              | Grafana admin password                           |

## Wiring

### SHT31

| STM32WL55 Pin | SHT31 Pin | Description |
| ------------- | --------- | ----------- |
| PB6           | SCL       | I2C clock   |
| PB7           | SDA       | I2C data    |
| 3.3V          | VCC       | Power       |
| GND           | GND       | Ground      |

### MAX485

| STM32WL55 Pin | MAX485 Pin | Description       |
| ------------- | ---------- | ----------------- |
| PA2           | DI         | USART2 TX         |
| PA3           | RO         | USART2 RX         |
| PA9           | DE + RE    | Direction control |
| 3.3V          | VCC        | Power             |
| GND           | GND        | Ground            |

## Modbus RTU Usage

```c
ModbusHandle_t hmodbus;
modbus_init(&hmodbus, &huart2, GPIOA, GPIO_PIN_9, 100);

uint16_t regs[4];
ModbusStatus_t ret = modbus_read_holding_registers(&hmodbus, 1, 0, 4, regs, 4);
```

| FC  | Name                   | Function                         |
| --- | ---------------------- | -------------------------------- |
| 03  | Read Holding Registers | `modbus_read_holding_registers()` |
| 04  | Read Input Registers   | `modbus_read_input_registers()`   |

## Alarm Rules

| Condition                 | Severity | Alarm Type   |
| ------------------------- | -------- | ------------ |
| `temperature < 0 or > 60` | CRITICAL | TEMPERATURE  |
| `humidity > 95`           | WARNING  | HUMIDITY     |
| `battery_mv < 3000`       | WARNING  | LOW_BATTERY  |

## License

MIT License, Copyright 2026 HallowedEngine
