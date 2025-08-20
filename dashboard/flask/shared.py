import json
import os
from flask import jsonify
from settings import CONFIG_FILE

latest_data = None

def api_response(status="ok", message=None, data=None, http_status=200):
    resp = {"status": status}
    if message: resp["message"] = message
    if data is not None: resp["data"] = data
    return jsonify(resp), http_status

def load_config():
    if not os.path.isfile(CONFIG_FILE):
        return {}
    with open(CONFIG_FILE, "r") as f:
        config = json.load(f)

    if "mqtt_homeassistant" in config:
        try:
            config["mqtt_homeassistant"]["port"] = int(config["mqtt_homeassistant"].get("port", 1883))
        except ValueError:
            config["mqtt_homeassistant"]["port"] = 1883

    if "mqtt" in config:
        try:
            config["mqtt"]["port"] = int(config["mqtt"].get("port", 1883))
        except ValueError:
            config["mqtt"]["port"] = 1883

    if "sensors" in config:
        try:
            config["sensors"]["polling_interval"] = int(config["sensors"].get("polling_interval", 300000))
        except ValueError:
            config["sensors"]["polling_interval"] = 300000

    return config

def save_config(data):
    with open(CONFIG_FILE, "w") as f:
        json.dump(data, f, indent=2)
