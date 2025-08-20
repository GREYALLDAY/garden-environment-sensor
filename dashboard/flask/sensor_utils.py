import logging
import os, csv
from datetime import datetime, timedelta, timezone
from settings import LOG_DIR, RAW_LOG_FILE

logger = logging.getLogger(__name__)


def validate_sensor_data(data):
    required_keys = ["temp_f", "humidity", "lux", "moisture"]
    for key in required_keys:
        if key not in data:
            logger.warning(f"[sensor] Missing key: {key}")
            return False
    return True

def format_sensor_data(data):
    return (
        f"🌡 Temp: {data['temp_f']}°F\n"
        f"💧 Humidity: {data['humidity']}%\n"
        f"☀️ Lux: {data['lux']}\n"
        f"🌱 Moisture: {data['moisture']}"
    )

def get_today_logfile():
    today = datetime.now().strftime("%Y-%m-%d")
    return os.path.join(LOG_DIR, f"raw_sensorlog_{today}.csv")

def get_latest_logfile():
    files = [
        f for f in os.listdir(LOG_DIR)
        if f.startswith("raw_sensorlog_") and f.endswith(".csv")
    ]
    if not files:
        return None
    for f in sorted(files, reverse=True):
        path = os.path.join(LOG_DIR, f)
        try:
            if os.path.getsize(path) > 0:
                return path
        except OSError:
            continue
    # fall back to most recent even if empty
    return os.path.join(LOG_DIR, sorted(files, reverse=True)[0])

def write_csv_log(data):
    os.makedirs(LOG_DIR, exist_ok=True)
    path = RAW_LOG_FILE
    is_new = not os.path.exists(path) or os.path.getsize(path) == 0

    with open(path, "a", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["timestamp","temp_f","humidity","lux","moisture"])
        if is_new:
            w.writeheader()
        w.writerow({
            "timestamp": data["timestamp"],  # ideally "YYYY-MM-DDTHH:MM:SSZ"
            "temp_f": round(data["temp_f"], 2),
            "humidity": round(data["humidity"], 2),
            "lux": round(data["lux"], 1),
            "moisture": round(data["moisture"], 1),
        })



def _read_csv_dicts(path):
    rows = []
    if not path or not os.path.exists(path) or os.path.getsize(path) == 0:
        return rows
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for r in reader:
            rows.append(r)
    return rows


def load_log_data(filter_range="24h", day=None):
    path = RAW_LOG_FILE
    if not os.path.exists(path) or os.path.getsize(path) == 0:
        return [], None

    data = []
    try:
        with open(path, newline="", encoding="utf-8-sig") as f:
            reader = csv.DictReader(f)
            for row in reader:
                ts = row.get("timestamp", "")
                # normalize timestamp -> ISO-UTC string
                try:
                    if ts.endswith("Z"):
                        # already UTC
                        pass
                    else:
                        # try parse local/naive -> keep as naive ISO to unblock UI
                        datetime.fromisoformat(ts)  # validate format
                    data.append({
                        "timestamp": ts if ts.endswith("Z") else ts,
                        "display_time": ts,  # front-end can format later
                        "temp_f": float(row["temp_f"]),
                        "humidity": float(row["humidity"]),
                        "lux": float(row["lux"]),
                        "moisture": float(row.get("moisture", 0)),
                    })
                except Exception:
                    continue
        # sort by timestamp string is fine if they’re ISO-ish
        data.sort(key=lambda x: x["timestamp"])
        return data, None
    except Exception as e:
        return [], str(e)
