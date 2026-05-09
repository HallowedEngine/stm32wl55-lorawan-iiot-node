"""Application configuration for the LoRaWAN MQTT to InfluxDB pipeline.

This module centralizes MQTT, InfluxDB, and alarm settings. Values are read
from environment variables first and fall back to development-friendly defaults.
"""

from __future__ import annotations

import os
from dataclasses import dataclass


def _get_int(name: str, default: int) -> int:
    value = os.getenv(name)
    if value is None:
        return default
    try:
        return int(value)
    except ValueError:
        return default


def _get_float(name: str, default: float) -> float:
    value = os.getenv(name)
    if value is None:
        return default
    try:
        return float(value)
    except ValueError:
        return default


@dataclass(frozen=True)
class MQTTConfig:
    """MQTT broker settings used by the subscriber."""

    host: str = os.getenv("MQTT_HOST", "localhost")
    port: int = _get_int("MQTT_PORT", 1883)
    topic: str = os.getenv("MQTT_TOPIC", "application/+/device/+/event/up")
    username: str | None = os.getenv("MQTT_USERNAME")
    password: str | None = os.getenv("MQTT_PASSWORD")
    keepalive: int = _get_int("MQTT_KEEPALIVE", 60)
    reconnect_delay_min_s: int = _get_int("MQTT_RECONNECT_DELAY_MIN_S", 1)
    reconnect_delay_max_s: int = _get_int("MQTT_RECONNECT_DELAY_MAX_S", 60)


@dataclass(frozen=True)
class InfluxDBConfig:
    """InfluxDB 2.x settings used by telemetry and alarm writers."""

    url: str = os.getenv("INFLUXDB_URL", "http://localhost:8086")
    token: str = os.getenv("INFLUXDB_TOKEN", "dev-token")
    org: str = os.getenv("INFLUXDB_ORG", "dev-org")
    bucket: str = os.getenv("INFLUXDB_BUCKET", "iiot")
    timeout_ms: int = _get_int("INFLUXDB_TIMEOUT_MS", 10_000)
    retries: int = _get_int("INFLUXDB_RETRIES", 3)
    retry_delay_s: float = _get_float("INFLUXDB_RETRY_DELAY_S", 2.0)


@dataclass(frozen=True)
class AlarmThresholds:
    """Threshold values for telemetry alarm evaluation."""

    temperature_low_critical: float = _get_float("ALARM_TEMP_LOW_CRITICAL", 0.0)
    temperature_high_critical: float = _get_float("ALARM_TEMP_HIGH_CRITICAL", 60.0)
    humidity_high_warning: float = _get_float("ALARM_HUMIDITY_HIGH_WARNING", 95.0)
    battery_low_mv: int = _get_int("ALARM_BATTERY_LOW_MV", 3000)


MQTT = MQTTConfig()
INFLUXDB = InfluxDBConfig()
ALARMS = AlarmThresholds()
