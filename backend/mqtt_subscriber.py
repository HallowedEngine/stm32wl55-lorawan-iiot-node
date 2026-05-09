"""MQTT subscriber for ChirpStack v4 LoRaWAN uplink messages.

The subscriber listens to ``application/{app_id}/device/{dev_eui}/event/up``,
validates incoming JSON payloads, writes telemetry to InfluxDB, and evaluates
alarm rules for every valid uplink.
"""

from __future__ import annotations

import json
import logging
import re
import signal
import sys
import time
from typing import Any

import paho.mqtt.client as mqtt

from alarm_engine import AlarmEngine
from config import MQTT, MQTTConfig
from influxdb_writer import InfluxDBWriter, TelemetryRecord


LOGGER = logging.getLogger(__name__)
TOPIC_RE = re.compile(
    r"^application/(?P<app_id>[^/]+)/device/(?P<dev_eui>[^/]+)/event/up$"
)


class PayloadValidationError(ValueError):
    """Raised when an MQTT payload misses required fields or has invalid types."""


class MQTTTelemetrySubscriber:
    """Subscribe to ChirpStack MQTT uplinks and process valid telemetry."""

    def __init__(self, config: MQTTConfig = MQTT) -> None:
        self._config = config
        self._writer = InfluxDBWriter()
        self._alarm_engine = AlarmEngine(self._writer)
        self._client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self._stopping = False

        if config.username:
            self._client.username_pw_set(config.username, config.password)

        self._client.reconnect_delay_set(
            min_delay=config.reconnect_delay_min_s,
            max_delay=config.reconnect_delay_max_s,
        )
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message

    def run_forever(self) -> None:
        """Connect to MQTT and process messages until the process is stopped."""

        self._install_signal_handlers()

        while not self._stopping:
            try:
                LOGGER.info(
                    "Connecting to MQTT broker %s:%s",
                    self._config.host,
                    self._config.port,
                )
                self._client.connect(
                    self._config.host,
                    self._config.port,
                    keepalive=self._config.keepalive,
                )
                self._client.loop_forever(retry_first_connection=True)
            except KeyboardInterrupt:
                self.stop()
            except Exception as exc:
                LOGGER.exception("MQTT connection loop failed: %s", exc)
                time.sleep(self._config.reconnect_delay_min_s)

        self._writer.close()

    def stop(self) -> None:
        """Stop MQTT processing and close network resources."""

        self._stopping = True
        self._client.disconnect()

    def _install_signal_handlers(self) -> None:
        def handle_stop(_signum: int, _frame: object) -> None:
            LOGGER.info("Shutdown requested")
            self.stop()

        signal.signal(signal.SIGINT, handle_stop)
        signal.signal(signal.SIGTERM, handle_stop)

    def _on_connect(
        self,
        client: mqtt.Client,
        _userdata: object,
        _flags: mqtt.ConnectFlags,
        reason_code: mqtt.ReasonCode,
        _properties: mqtt.Properties | None,
    ) -> None:
        if reason_code.is_failure:
            LOGGER.error("MQTT connection failed: %s", reason_code)
            return

        LOGGER.info("Connected to MQTT broker, subscribing to %s", self._config.topic)
        client.subscribe(self._config.topic)

    def _on_disconnect(
        self,
        _client: mqtt.Client,
        _userdata: object,
        _disconnect_flags: mqtt.DisconnectFlags,
        reason_code: mqtt.ReasonCode,
        _properties: mqtt.Properties | None,
    ) -> None:
        if self._stopping:
            LOGGER.info("Disconnected from MQTT broker")
            return
        LOGGER.warning("MQTT disconnected, client will retry: %s", reason_code)

    def _on_message(
        self,
        _client: mqtt.Client,
        _userdata: object,
        message: mqtt.MQTTMessage,
    ) -> None:
        try:
            telemetry = parse_telemetry_message(message.topic, message.payload)
            self._writer.write_telemetry(telemetry)
            self._alarm_engine.evaluate(telemetry)
            LOGGER.info(
                "Telemetry written dev_eui=%s app_id=%s f_cnt=%s",
                telemetry.dev_eui,
                telemetry.app_id,
                telemetry.f_cnt,
            )
        except PayloadValidationError as exc:
            LOGGER.warning("Invalid MQTT payload on topic %s: %s", message.topic, exc)
        except Exception:
            LOGGER.exception("Failed to process MQTT message on topic %s", message.topic)


def parse_telemetry_message(topic: str, payload: bytes) -> TelemetryRecord:
    """Parse and validate a ChirpStack MQTT message into a telemetry record."""

    topic_match = TOPIC_RE.match(topic)
    if not topic_match:
        raise PayloadValidationError(f"topic does not match expected format: {topic}")

    try:
        decoded = payload.decode("utf-8")
        raw = json.loads(decoded)
    except UnicodeDecodeError as exc:
        raise PayloadValidationError("payload is not valid UTF-8") from exc
    except json.JSONDecodeError as exc:
        raise PayloadValidationError("payload is not valid JSON") from exc

    if not isinstance(raw, dict):
        raise PayloadValidationError("payload root must be a JSON object")

    data = _require_dict(raw, "data")
    rx_info = _require_list(raw, "rxInfo")
    if not rx_info or not isinstance(rx_info[0], dict):
        raise PayloadValidationError("rxInfo must contain at least one object")

    topic_app_id = topic_match.group("app_id")
    topic_dev_eui = topic_match.group("dev_eui")
    payload_dev_eui = _require_str(raw, "devEui")
    if payload_dev_eui != topic_dev_eui:
        raise PayloadValidationError(
            f"devEui mismatch between topic ({topic_dev_eui}) and payload ({payload_dev_eui})"
        )

    return TelemetryRecord(
        dev_eui=payload_dev_eui,
        app_id=topic_app_id,
        temperature=_require_number(data, "temperature"),
        humidity=_require_number(data, "humidity"),
        battery_mv=_require_int(data, "battery_mv"),
        uptime_s=_require_int(data, "uptime_s"),
        rssi=_require_number(rx_info[0], "rssi"),
        snr=_require_number(rx_info[0], "snr"),
        f_cnt=_optional_int(raw, "fCnt"),
    )


def _require_dict(source: dict[str, Any], key: str) -> dict[str, Any]:
    value = _require_key(source, key)
    if not isinstance(value, dict):
        raise PayloadValidationError(f"{key} must be an object")
    return value


def _require_list(source: dict[str, Any], key: str) -> list[Any]:
    value = _require_key(source, key)
    if not isinstance(value, list):
        raise PayloadValidationError(f"{key} must be a list")
    return value


def _require_str(source: dict[str, Any], key: str) -> str:
    value = _require_key(source, key)
    if not isinstance(value, str) or not value:
        raise PayloadValidationError(f"{key} must be a non-empty string")
    return value


def _require_number(source: dict[str, Any], key: str) -> float:
    value = _require_key(source, key)
    if isinstance(value, bool) or not isinstance(value, int | float):
        raise PayloadValidationError(f"{key} must be a number")
    return float(value)


def _require_int(source: dict[str, Any], key: str) -> int:
    value = _require_key(source, key)
    if isinstance(value, bool) or not isinstance(value, int):
        raise PayloadValidationError(f"{key} must be an integer")
    return value


def _optional_int(source: dict[str, Any], key: str) -> int | None:
    value = source.get(key)
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, int):
        raise PayloadValidationError(f"{key} must be an integer when present")
    return value


def _require_key(source: dict[str, Any], key: str) -> Any:
    if key not in source:
        raise PayloadValidationError(f"missing required field: {key}")
    return source[key]


def main() -> int:
    """Start the MQTT telemetry subscriber."""

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )
    subscriber = MQTTTelemetrySubscriber()
    subscriber.run_forever()
    return 0


if __name__ == "__main__":
    sys.exit(main())
