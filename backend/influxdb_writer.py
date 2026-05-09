"""InfluxDB 2.x writer for LoRaWAN telemetry and alarm events.

The writer stores parsed sensor data in the ``iiot_telemetry`` measurement and
alarm records in the ``iiot_alarm`` measurement, retrying transient write errors.
"""

from __future__ import annotations

import logging
import time
from dataclasses import dataclass
from typing import Any

from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

from config import INFLUXDB, InfluxDBConfig


LOGGER = logging.getLogger(__name__)


@dataclass(frozen=True)
class TelemetryRecord:
    """Validated telemetry values ready to be written to InfluxDB."""

    dev_eui: str
    app_id: str
    temperature: float
    humidity: float
    battery_mv: int
    uptime_s: int
    rssi: float
    snr: float
    f_cnt: int | None = None


@dataclass(frozen=True)
class AlarmRecord:
    """Alarm values ready to be written to InfluxDB."""

    dev_eui: str
    app_id: str
    alarm_type: str
    severity: str
    message: str
    value: float
    threshold: float


class InfluxDBWriter:
    """Small wrapper around the InfluxDB client with retry-aware writes."""

    def __init__(self, config: InfluxDBConfig = INFLUXDB) -> None:
        self._config = config
        self._client = InfluxDBClient(
            url=config.url,
            token=config.token,
            org=config.org,
            timeout=config.timeout_ms,
        )
        self._write_api = self._client.write_api(write_options=SYNCHRONOUS)

    def write_telemetry(self, telemetry: TelemetryRecord) -> None:
        """Write one telemetry record to the ``iiot_telemetry`` measurement."""

        point = (
            Point("iiot_telemetry")
            .tag("dev_eui", telemetry.dev_eui)
            .tag("app_id", telemetry.app_id)
            .field("temperature", telemetry.temperature)
            .field("humidity", telemetry.humidity)
            .field("battery_mv", telemetry.battery_mv)
            .field("uptime_s", telemetry.uptime_s)
            .field("rssi", telemetry.rssi)
            .field("snr", telemetry.snr)
        )
        if telemetry.f_cnt is not None:
            point.field("f_cnt", telemetry.f_cnt)

        self._write_with_retry(point)

    def write_alarm(self, alarm: AlarmRecord) -> None:
        """Write one alarm event to the ``iiot_alarm`` measurement."""

        point = (
            Point("iiot_alarm")
            .tag("dev_eui", alarm.dev_eui)
            .tag("app_id", alarm.app_id)
            .tag("alarm_type", alarm.alarm_type)
            .tag("severity", alarm.severity)
            .field("message", alarm.message)
            .field("value", alarm.value)
            .field("threshold", alarm.threshold)
        )
        self._write_with_retry(point)

    def close(self) -> None:
        """Close the underlying InfluxDB client connection."""

        self._client.close()

    def _write_with_retry(self, point: Point) -> None:
        attempts = max(1, self._config.retries)
        last_error: Exception | None = None

        for attempt in range(1, attempts + 1):
            try:
                self._write_api.write(
                    bucket=self._config.bucket,
                    org=self._config.org,
                    record=point,
                    write_precision=WritePrecision.NS,  # type: ignore[arg-type]
                )
                return
            except Exception as exc:
                last_error = exc
                LOGGER.warning(
                    "InfluxDB write failed on attempt %s/%s: %s",
                    attempt,
                    attempts,
                    exc,
                )
                if attempt < attempts:
                    time.sleep(self._config.retry_delay_s)

        raise RuntimeError("InfluxDB write failed after retries") from last_error


def telemetry_from_dict(data: dict[str, Any]) -> TelemetryRecord:
    """Build a typed telemetry record from a validated dictionary."""

    return TelemetryRecord(
        dev_eui=str(data["dev_eui"]),
        app_id=str(data["app_id"]),
        temperature=float(data["temperature"]),
        humidity=float(data["humidity"]),
        battery_mv=int(data["battery_mv"]),
        uptime_s=int(data["uptime_s"]),
        rssi=float(data["rssi"]),
        snr=float(data["snr"]),
        f_cnt=int(data["f_cnt"]) if data.get("f_cnt") is not None else None,
    )
