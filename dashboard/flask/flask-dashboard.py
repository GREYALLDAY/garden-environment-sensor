from flask import Flask, jsonify, request, render_template
from datetime import datetime
import os
import csv

app = Flask(
    __name__,
    template_folder="../templates",
    static_folder="../static"
    )
app.config["TEMPLATES_AUTO_RELOAD"] = True

# In-memory cache of last reading, set logfile location
latest_data = None
log_file = os.path.join(os.path.dirname(__file__), "sensor_log.csv")

# Route: HTML dashboard
@app.route("/dashboard")
def dashboard():
    current_year = datetime.now().year
    return render_template("dashboard.html", year=current_year)

# GET API status
@app.route("/api/status", methods=["GET"])
def status():
    return jsonify({"message": "Flask server is working!"})

# GET latest sensor data
@app.route("/api/sensor", methods=["GET"])
def get_sensor_data():
    return jsonify(latest_data)

# POST sensor data
@app.route("/api/sensor", methods=["POST"])
def post_sensor_data():
    global latest_data
    data = request.get_json()

    # Validate and store
    try:
        latest_data = {
            "temp_f": float(data["temp_f"]),
            "humidity": float(data["humidity"]),
            "lux": float(data["lux"]),
            "moisture": float(data["moisture"])
        }

        file_exists = os.path.isfile(log_file)
        with open(log_file, "a", newline="") as f:
            writer = csv.writer(f)
            if not file_exists:
                writer.writerow(["timestamp", "temp_f", "humidity", "lux", "moisture"])
            timestamp = datetime.now().strftime("%Y-%m-%d %I:%M:%S %p")
            writer.writerow([
                timestamp,
                round(latest_data["temp_f"], 2),
                round(latest_data["humidity"], 2),
                round(latest_data["lux"], 1),
                round(latest_data["moisture"], 1)
            ])
        return jsonify({"status": "ok", "received": latest_data}), 201

    except Exception as e:
        return jsonify({"error": str(e)}), 400

# GET historical data
@app.route("/api/history", methods=["GET"])
def get_history():
    data = []
    try:
        with open(log_file, newline="") as f:
            reader = csv.DictReader(f)
            for row in reader:
                try:
                    dt = datetime.strptime(row["timestamp"], "%Y-%m-%d %I:%M:%S %p")
                    formatted = dt.strftime("%I:%M:%S %p %m/%-d")
                    data.append({
                        "timestamp": formatted,
                        "temp_f": float(row["temp_f"]),
                        "humidity": float(row["humidity"]),
                        "lux": float(row["lux"]),
                        "moisture": float(row.get("moisture", 0))
                    })
                except (ValueError, KeyError):
                    continue
        return jsonify(data)
    except FileNotFoundError:
        return jsonify([])

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000, debug=True)