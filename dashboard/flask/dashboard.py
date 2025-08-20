from flask_mqtt import Mqtt
from flask import Flask, jsonify
from datetime import datetime
import os
from dotenv import load_dotenv
# custom imports
from routes import routes
from shared import load_config, save_config
from ntfy_handler import send_ntfy_message
from mqtt_handler import publish_mqtt
from sensor_utils import validate_sensor_data, format_sensor_data
from settings import CONFIG_FILE, LOG_DIR
from logging_config import setup_loggers

# Load .env variables
load_dotenv()

# Initialize flask/websocket
app = Flask(__name__, template_folder="./templates", static_folder="./static")
app.register_blueprint(routes)
app.config["TEMPLATES_AUTO_RELOAD"] = True
os.makedirs(LOG_DIR, exist_ok=True)


# set up logging
setup_loggers(app)

## Local MQTT configuration
app.config.update(
    MQTT_BROKER_URL='localhost',
    MQTT_BROKER_PORT=1883,
    MQTT_USERNAME='',
    MQTT_PASSWORD='',
    MQTT_KEEPALIVE=60,
    MQTT_TLS_ENABLED=False
)
#mqtt = Mqtt(app)

## set global state
latest_data = None

## Set a standardized API response
def api_response(status="ok", message=None, data=None, http_status=200):
    resp = {"status": status}
    if message: resp["message"] = message
    if data is not None: resp["data"] = data
    return jsonify(resp), http_status


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000, debug=True,)
