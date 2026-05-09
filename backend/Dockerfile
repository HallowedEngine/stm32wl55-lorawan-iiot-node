FROM python:3.11-slim

RUN useradd --create-home --uid 1000 --shell /usr/sbin/nologin appuser

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

USER appuser

HEALTHCHECK --interval=30s --timeout=5s --retries=3 CMD python -c "import pathlib, sys; sys.exit(0 if 'mqtt_subscriber.py' in pathlib.Path('/proc/1/cmdline').read_text(errors='ignore') else 1)"

CMD ["python", "mqtt_subscriber.py"]
