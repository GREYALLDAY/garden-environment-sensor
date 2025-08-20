from flask import Flask, jsonify, request, render_template, Blueprint, current_app as app
from ntfy_handler import send_ntfy_message
from mqtt_handler import publish_mqtt
from sensor_utils import format_sensor_data, load_log_data, write_csv_log, get_today_logfile, get_latest_logfile
from settings import RAW_LOG_FILE, CONFIG_FILE
from shared import latest_data, api_response, load_config, save_config
import logging
from datetime import datetime, timezone
import os, json, tempfile
import pandas as pd

routes = Blueprint('routes', __name__)
logger = logging.getLogger('dashboard')


# [GET] /dashboard ---------------------------------------------------------
@routes.route("/dashboard")
def dashboard():
    log_file = get_latest_logfile()
    if not os.path.exists(log_file):
        return "Sensor log not found", 404
    
    df = pd.read_csv(log_file)
    df["timestamp"] = pd.to_datetime(df["timestamp"])
    return render_template("dashboard.html", year=datetime.now().year)


# [GET] /api/sensor  --------------------------------------------------------
@routes.route("/api/sensor", methods=["GET"])
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
@routes.route("/api/sensor", methods=["POST"])
def post_sensor_data():
    try:
        if not request.is_json:
            return api_response("error", "Content-Type must be application/json", 400)

        data = request.get_json(silent=True)
        if not data:
            return api_response("error", "Invalid or missing JSON payload", 400)

        required = ("temp_f", "humidity", "lux", "moisture")

        def num(field):
            v = data.get(field, None)
            # treat None/"" as invalid
            if v is None or (isinstance(v, str) and v.strip() == ""):
                raise ValueError(f"{field} is missing or null")
            try:
                return float(v)
            except (ValueError, TypeError):
                raise ValueError(f"{field} must be numeric")

        try:
            latest_data = {
                "timestamp": datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"),
                "temp_f":  num("temp_f"),
                "humidity": num("humidity"),
                "lux":      num("lux"),
                "moisture": num("moisture"),
            }
        except ValueError as ve:
            logger.error(f"[API] /api/sensor validation error: {ve}; payload={data}")
            return api_response("error", str(ve), 400)

        # side-effects should not crash the request
        try:
            publish_mqtt(latest_data)
        except Exception as e:
            logger.error(f"[MQTT] publish failed: {e}")

        try:
            send_ntfy_message(format_sensor_data(latest_data))
        except Exception as e:
            logger.error(f"[NTFY] send failed: {e}")

        try:
            write_csv_log(latest_data)
        except Exception as e:
            logger.error(f"[CSV] write failed: {e}")
            return api_response("error", "Failed to write log", 500)

        return api_response("ok", data={"received": True})
    except Exception as e:
        logger.exception("[API] /api/sensor unhandled")
        return api_response("error", "internal error", 500)


# ---------------------------------------------------------------------------



# [GET|POST] /api/config  ---------------------------------------------------
@routes.route("/api/config", methods=["GET", "POST"])
def config_handler():
    if request.method == "GET":
        return api_response(data=load_config())

    try:
        new_cfg = request.get_json(silent=True)
        if not new_cfg:
            logger.error("[API] /api/config: No JSON body found")
            return api_response("error", "No JSON Found", http_status=400)

        # Schema: sleep in seconds, plus other config keys
        validators = {
            "sleep":        lambda v: (int(v), 1, 6000),  # 1s to 6000s
            "ssid":         lambda v: (str(v), None, None),
            "password":     lambda v: (str(v), None, None),
            "mqtt_broker":  lambda v: (str(v), None, None),
            "mqtt_port":    lambda v: (int(v), 1, 65535),
        }

        validated = {}

        # Backward-compat: accept polling_interval and convert → sleep (seconds)
        if "polling_interval" in new_cfg and "sleep" not in new_cfg:
            try:
                val = int(new_cfg["polling_interval"])
                if val > 6000:  # probably ms
                    val //= 1000
                val = max(1, min(6000, val))
                validated["sleep"] = val
            except Exception:
                pass

        # Validate all known keys
        for key, fn in validators.items():
            if key in new_cfg:
                try:
                    val, lo, hi = fn(new_cfg[key])
                    if lo is not None and val < lo:
                        return api_response("error", f"{key} must be >= {lo}", 400)
                    if hi is not None and val > hi:
                        return api_response("error", f"{key} must be <= {hi}", 400)
                    validated[key] = val
                except (ValueError, TypeError):
                    return api_response("error", f"Invalid value for {key}", 400)

        # Load current config, update with validated values
        config = load_config()
        config.update(validated)

        # Bump version if anything changed
        if validated:
            old_ver = int(config.get("config_version", 0))
            config["config_version"] = old_ver + 1
            from datetime import datetime, timezone
            config["updated_at"] = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

        # Save (atomic if helper is in place)
        save_config(config)

        return api_response("ok", data=config)

    except Exception as e:
        logger.error(f"[API] Exception in /api/config: {e}")
        return api_response("error", str(e), http_status=500)

# ---------------------------------------------------------------------------

# /api/status 
@routes.route("/api/status")
def status():
    return api_response(message="API is running!")

# /api/history  -------------------------------------------------------------
@routes.route("/api/history")
def get_history():



    filter_range = request.args.get("filter_range", "24h")
    day_param = request.args.get("day")  # optional

    log_path = RAW_LOG_FILE
    try:
        size = os.path.getsize(log_path)
    except Exception:
        size = -1
    app.logger.info(f"[API] /history using this logfile: {log_path} ({size} bytes), filter_range={filter_range}, day={day_param}")



    data, error = load_log_data(filter_range=filter_range, day=day_param)
    if error:
        logger.error(f"[API] /api/history error: {error}")
        return api_response("error", error, http_status=404)

    return api_response(data=data)


# ---------------------------------------------------------------------------
