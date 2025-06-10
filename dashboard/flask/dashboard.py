from flask_mqtt import Mqtt
from flask import Flask, jsonify, request, render_template
from datetime import datetime
import os
import csv
import json
import logging
import pandas as pd
from logging.handlers import RotatingFileHandler


# instantiate flask app
app = Flask(__name__, template_folder="../templates", static_folder="../static")
app.config["TEMPLATES_AUTO_RELOAD"] = True
os.makedirs('logs', exist_ok=True)

# configure SERVER log handlers, one for console, one for logfile which will be saved in ../flask/logs
formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.INFO)
console_handler.setFormatter(formatter)
handler = RotatingFileHandler('logs/sensor-log.log', maxBytes=1000000, backupCount=5)
handler.setLevel(logging.INFO)
handler.setFormatter(formatter)
app.logger.addHandler(console_handler)
app.logger.addHandler(handler)

# function to publish readings to mqtt
def publish_mqtt(data):
    try:
        mqtt.publish("garden/sensors", json.dumps(data), retain=True)
    except Exception as e:
        print("error publishing to MQTT", e)

# configure MQTT broker
app.config['MQTT_BROKER_URL'] = '<IP_ADDRESS>' # change <IP_ADDRESS> to your brokers 
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = ''
app.config['MQTT_PASSWORD'] = ''
app.config['MQTT_KEEPALIVE'] = 60
app.config['MQTT_TLS_ENABLED'] = False
mqtt = Mqtt(app)

# set SENSOR logfile location to /logs
latest_data = None
log_file = os.path.join(os.path.dirname(__file__), "logs/raw_sensorlog.csv")

# dashboard
@app.route("/dashboard")
def dashboard():
    df = pd.read_csv("raw_sensorlog.csv")
    df["timestamp"] = pd.to_datetime(df["timestamp"])
    current_year = datetime.now().year
    return render_template("dashboard.html", year=current_year)

# [POST] sensor data
@app.route("/api/sensor", methods=["POST"])
def post_sensor_data():
    global latest_data
    data = request.get_json()
    if not data:
            return jsonify({"error": "Invalid or missing JSON payload"}), 400
    required_keys = ["timestamp", "temp_f", "humidity", "lux", "moisture"]
    if not all(k in data for k in required_keys):
        return jsonify({"error": "Missing required headers!"}), 400

# validate data before logging, publish to MQTT
    try:
        latest_data = {
            "timestamp": datetime.now().isoformat(),
            "temp_f": float(data["temp_f"]),
            "humidity": float(data["humidity"]),
            "lux": float(data["lux"]),
            "moisture": float(data["moisture"])
        }
        publish_mqtt(latest_data)
        print("Published to MQTT:", json.dumps(latest_data))

        file_exists = os.path.isfile(log_file)
        write_header = not file_exists or os.stat(log_file).st_size == 0 # Makes sure file and headers exist. if file is empty, it will add headers
        
        with open(log_file, "a", newline="") as f:
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
        app.logger.info(f"Sensor POST received: {request.json}")
        print("writing to log file.") # extra print statement, can be removed
        return jsonify({"status": "ok", "received": latest_data}), 201
    except Exception as e:
        print("Error during POST handling", e)
        return jsonify({"error": str(e)}), 400

# [GET] API status
@app.route("/api/status", methods=["GET"])
def status():
    return jsonify({"message": "API is working!"})

# [GET] latest sensor data
@app.route("/api/sensor", methods=["GET"])
def get_sensor_data():
    if latest_data is None:
        return jsonify({"message": "No data received yet"}), 204
    return jsonify(latest_data)
    
# [GET] historical data from raw_sensorlog.csv
@app.route("/api/history", methods=["GET"])
def get_history():
    data = []
    try:
        with open(log_file, newline="") as f:
            reader = csv.DictReader(f)
            for row in reader:
                try:
                    timestamp = row["timestamp"].replace("Z", "") # clean up UTC timestamp for readability
                    dt = datetime.fromisoformat(timestamp) # converts time to ISO format
                    display_time = dt.strftime("%-m/%-d/%y %I:%M %p") # converts UTC into more readable format, stores as display_time

                    data.append({
                    "timestamp": dt.isoformat(),
                    "display_time": display_time,
                    "temp_f": float(row["temp_f"]),
                    "humidity": float(row["humidity"]),
                    "lux": float(row["lux"]),
                    "moisture": float(row.get("moisture", 0))
                })       
                except (ValueError, KeyError) as e:
                    print(f"Skipping row due to error: {e}, Row: {row}")
                    continue
        data.sort(key=lambda x: x["timestamp"])
        return jsonify(data)
    except FileNotFoundError:
        return jsonify([])

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000, debug=True)