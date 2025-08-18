from flask_mqtt import Mqtt
from flask import Flask, jsonify, request, render_template
from datetime import datetime
import os
import csv
import json
import logging
import pandas as pd
from logging.handlers import RotatingFileHandler
import subprocess

CONFIG_FILE = "config.json"
LOG_DIR = "logs"
RAW_LOG_FILE = os.path.join(LOG_DIR, "raw_sensorlog.csv")
VEML_LOG_FILE = os.path.join(LOG_DIR, "veml-debug.csv")

# Initialize Flask app
app = Flask(__name__, template_folder="../templates", static_folder="../static")
app.config["TEMPLATES_AUTO_RELOAD"] = True
os.makedirs(LOG_DIR, exist_ok=True)

# Load and save config
def load_config():
    if not os.path.isfile(CONFIG_FILE):
        return {}
    with open(CONFIG_FILE, "r") as f:
        return json.load(f)

def save_config(data):
    with open(CONFIG_FILE, "w") as f:
        json.dump(data, f, indent=2)

# Logging configuration
formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.INFO)
console_handler.setFormatter(formatter)

log_handler = RotatingFileHandler(os.path.join(LOG_DIR, 'sensor-log.log'), maxBytes=5_000_000, backupCount=5)
log_handler.setLevel(logging.INFO)
log_handler.setFormatter(formatter)

app.logger.addHandler(console_handler)
app.logger.addHandler(log_handler)

veml_logger = logging.getLogger("vemlLogger")
veml_logger.setLevel(logging.DEBUG)
veml_handler = RotatingFileHandler(VEML_LOG_FILE, maxBytes=500_000, backupCount=3)
veml_handler.setFormatter(logging.Formatter('%(message)s'))

if not veml_logger.handlers:
    veml_logger.addHandler(veml_handler)
    

# MQTT configuration
app.config.update(
    MQTT_BROKER_URL='localhost',
    MQTT_BROKER_PORT=1883,
    MQTT_USERNAME='',
    MQTT_PASSWORD='',
    MQTT_KEEPALIVE=60,
    MQTT_TLS_ENABLED=False
)
mqtt = Mqtt(app)

# NFTY configuration
def send_ntfy_message(topic, message, title="Garden Sensor-ntfy", priority="default"):
    subprocess.run([
        "ntfy", "publish", topic,
        "--priority", priority,
        "--title", title,
        message
    ])

# Global state
latest_data = None

# Publish to MQTT
def publish_mqtt(data):
    try:
        mqtt.publish("garden/sensors", json.dumps(data), retain=True)
    except Exception as e:
        print("Error publishing to MQTT:", e)


def api_response(status="ok", message=None, data=None, http_status=200):
    resp = {"status": status}
    if message: resp["message"] = message
    if data is not None: resp["data"] = data
    return jsonify(resp), http_status
# ---------------------------------------------------------------------------


# ============= ROUTES =============

@app.route("/dashboard")
def dashboard():
    if not os.path.exists(RAW_LOG_FILE):
        return "Sensor log not found", 404
    df = pd.read_csv(RAW_LOG_FILE)
    df["timestamp"] = pd.to_datetime(df["timestamp"])
    return render_template("dashboard.html", year=datetime.now().year)


# [GET] /api/sensor  --------------------------------------------------------
@app.route("/api/sensor", methods=["GET"])
def get_sensor_data():
    if latest_data is None:
        return api_response("error", "No data received yet", http_status=200)

    try:
        dt = datetime.fromisoformat(latest_data["timestamp"].replace("Z", ""))
        formatted = {
            "timestamp": dt.isoformat(),
            "display_time": dt.strftime("%-m/%-d/%y %I:%M %p"),
            "temp_f": float(latest_data["temp_f"]),
            "humidity": float(latest_data["humidity"]),
            "lux": float(latest_data["lux"]),
            "moisture": float(latest_data.get("moisture", 0))
        }
        return api_response(data=formatted) 
    except (ValueError, KeyError) as e:
        return api_response("error", str(e), http_status=500)
# ---------------------------------------------------------------------------


# [POST] /api/sensor  -------------------------------------------------------
@app.route("/api/sensor", methods=["POST"])
def post_sensor_data():
    global latest_data
    data = request.get_json()
    if not data:
        return api_response("error", "Invalid or missing JSON payload", http_status=400)

    required = {"timestamp", "temp_f", "humidity", "lux", "moisture"}
    if not required.issubset(data):
        return api_response("error", "Missing required headers!", http_status=400)

    try:
        latest_data = {
            "timestamp": datetime.now().isoformat(),
            "temp_f": float(data["temp_f"]),
            "humidity": float(data["humidity"]),
            "lux": float(data["lux"]),
            "moisture": float(data["moisture"]),
        }

        publish_mqtt(data)
        print("Published to MQTT:", json.dumps(latest_data))
        
        
        file_exists = os.path.isfile(RAW_LOG_FILE)
        write_header = not file_exists or os.stat(RAW_LOG_FILE).st_size == 0

        with open(RAW_LOG_FILE, "a", newline="") as f:
            writer = csv.writer(f)
            if write_header:
                writer.writerow(["timestamp", "temp_f", "humidity", "lux", "moisture"])
            writer.writerow([
                latest_data["timestamp"],
                round(latest_data["temp_f"], 2),
                round(latest_data["humidity"], 2),
                round(latest_data["lux"], 1),
                round(latest_data["moisture"], 1)
            ])



        if not os.path.isfile(VEML_LOG_FILE) or os.stat(VEML_LOG_FILE).st_size == 0:
            with open(VEML_LOG_FILE, 'w') as f:
                f.write("timestamp,lux,raw_white,raw_als,integration_time\n")

        veml_data_row = (
            f"{latest_data['timestamp']},"
            f"{latest_data.get('lux', -1):.1f},"
            f"{data.get('raw_white', -1)},"
            f"{data.get('raw_als', -1)},"
            f"{data.get('integration_time', -1)}"
        )
        veml_logger.debug(veml_data_row)
        for handler in veml_logger.handlers:
            handler.flush()


        app.logger.info(f"Sensor POST received: {request.json}")
        print("Writing to log file.")
        print("[DEBUG] VEML data row: ", veml_data_row)
        return api_response(data={"received": latest_data}, http_status=201)
        
    except Exception as e:
        app.logger.error(f"Error during POST: {e}")
        return api_response("error", str(e), http_status=500)
# ---------------------------------------------------------------------------


# [GET|POST] /api/config  ---------------------------------------------------
@app.route("/api/config", methods=["GET", "POST"])
def config_handler():
    if request.method == "GET":
        return api_response(data=load_config())
    try:
        new_cfg = request.get_json()
        if not new_cfg:
            return api_response("error", "No JSON Found", http_status=400)
        valid_schema = {
            "polling_rate": int,
            "ssid": str,
            "password": str,
            "mqtt_broker": str,
            "mqtt_port": int
        }
        validated = {}
        for key, expected_type in valid_schema.items():
            if key in new_cfg:
                value = new_cfg[key]
                if not isinstance(value, expected_type):
                    if key == "polling_rate" and not (1000 <= value <= 600000):
                        return jsonify({"error": "polling_rate must be between 1000 and 600000"})
                validated[key] = value
        config = load_config()
        config.update(validated)
        save_config(config)
        return api_response("ok", data=config)
    except Exception as e:
        return api_response("error", str(e), http_status=500)
# ---------------------------------------------------------------------------


# /api/status stays simple
@app.route("/api/status")
def status():
    return api_response(message="API is running!")       # {"status":"ok","message":...}

# /api/history  -------------------------------------------------------------
@app.route("/api/history")
def get_history():
    try:
        with open(RAW_LOG_FILE, newline="") as f:
            reader = csv.DictReader(f)
            data = []
            for row in reader:
                try:
                    dt = datetime.fromisoformat(row["timestamp"].replace("Z", ""))
                    data.append({
                        "timestamp": dt.isoformat(),
                        "display_time": dt.strftime("%-m/%-d/%y %I:%M %p"),
                        "temp_f": float(row["temp_f"]),
                        "humidity": float(row["humidity"]),
                        "lux": float(row["lux"]),
                        "moisture": float(row.get("moisture", 0)),
                    })
                except (ValueError, KeyError):
                    continue
        data.sort(key=lambda x: x["timestamp"])
        return api_response(data=data)
    except FileNotFoundError:
        return api_response("error", "History log not found", http_status=404)
# ---------------------------------------------------------------------------


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000, debug=True)