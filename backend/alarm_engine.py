"""Alarm evaluation for validated LoRaWAN telemetry.

The alarm engine applies configured thresholds and writes every triggered alarm
to InfluxDB while also emitting a clear console log message.
"""

from __future__ import annotations

import logging
from collections.abc import Iterable

from config import ALARMS, AlarmThresholds
from influxdb_writer import AlarmRecord, InfluxDBWriter, TelemetryRecord


LOGGER = logging.getLogger(__name__)


class AlarmEngine:
    """Evaluate telemetry records and persist triggered alarm events."""

    def __init__(
        self,
        writer: InfluxDBWriter,
        thresholds: AlarmThresholds = ALARMS,
    ) -> None:
        self._writer = writer
        self._thresholds = thresholds

    def evaluate(self, telemetry: TelemetryRecord) -> list[AlarmRecord]:
        """Evaluate telemetry and write any triggered alarms."""

        alarms = list(self._build_alarms(telemetry))
        for alarm in alarms:
            LOGGER.warning(
                "ALARM %s severity=%s dev_eui=%s app_id=%s value=%s threshold=%s message=%s",
                alarm.alarm_type,
                alarm.severity,
                alarm.dev_eui,
                alarm.app_id,
                alarm.value,
                alarm.threshold,
                alarm.message,
            )
            try:
                self._writer.write_alarm(alarm)
            except Exception:
                LOGGER.exception("Failed to write alarm to InfluxDB: %s", alarm)

        return alarms

    def _build_alarms(self, telemetry: TelemetryRecord) -> Iterable[AlarmRecord]:
        if telemetry.temperature < self._thresholds.temperature_low_critical:
            yield AlarmRecord(
                dev_eui=telemetry.dev_eui,
                app_id=telemetry.app_id,
                alarm_type="TEMPERATURE",
                severity="CRITICAL",
                message="Temperature is below critical low threshold",
                value=telemetry.temperature,
                threshold=self._thresholds.temperature_low_critical,
            )

        if telemetry.temperature > self._thresholds.temperature_high_critical:
            yield AlarmRecord(
                dev_eui=telemetry.dev_eui,
                app_id=telemetry.app_id,
                alarm_type="TEMPERATURE",
                severity="CRITICAL",
                message="Temperature is above critical high threshold",
                value=telemetry.temperature,
                threshold=self._thresholds.temperature_high_critical,
            )

        if telemetry.humidity > self._thresholds.humidity_high_warning:
            yield AlarmRecord(
                dev_eui=telemetry.dev_eui,
                app_id=telemetry.app_id,
                alarm_type="HUMIDITY",
                severity="WARNING",
                message="Humidity is above warning threshold",
                value=telemetry.humidity,
                threshold=self._thresholds.humidity_high_warning,
            )

        if telemetry.battery_mv < self._thresholds.battery_low_mv:
            yield AlarmRecord(
                dev_eui=telemetry.dev_eui,
                app_id=telemetry.app_id,
                alarm_type="LOW_BATTERY",
                severity="WARNING",
                message="Battery voltage is below low battery threshold",
                value=float(telemetry.battery_mv),
                threshold=float(self._thresholds.battery_low_mv),
            )
