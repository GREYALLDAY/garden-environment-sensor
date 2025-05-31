from datetime import datetime
from fastapi import FastAPI, Request  # type: ignore
from fastapi.responses import HTMLResponse  # type: ignore
from fastapi.staticfiles import StaticFiles  # type: ignore
from fastapi.templating import Jinja2Templates  # type: ignore
from pydantic import BaseModel  # type: ignore
from pathlib import Path
import csv
import os

app = FastAPI()

# Define base directory and log file path
BASE_DIR = Path(__file__).resolve().parent.parent
log_file = BASE_DIR / "sensor_log.csv"

# Mount static and templates folders
app.mount("/static", StaticFiles(directory=BASE_DIR / "static"), name="static")
templates = Jinja2Templates(directory=BASE_DIR / "templates")

class SensorData(BaseModel):
    temp_f: float
    humidity: float
    lux: float
    moisture: float

# In-memory cache of last reading
latest_data: SensorData | None = None

# GET dashboard
@app.get("/dashboard", response_class=HTMLResponse)
def dashboard(request: Request):
    current_year = datetime.now().year
    return templates.TemplateResponse("dashboard.html", {"request": request, "year": current_year})

# GET API Status
@app.get("/api/status")
def root():
    return {"message": "FastAPI is working!"}

# GET sensor data
@app.get("/api/sensor")
def read_sensor():
    global latest_data
    if latest_data is None:
        return {"status": "no data received yet"}
    return latest_data

# POST sensor data
@app.post("/api/sensor", status_code=201)
def receive_data(data: SensorData):
    global latest_data
    latest_data = data
    file_exists = log_file.exists()
    with log_file.open("a", newline="") as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(["timestamp", "temp_f", "humidity", "lux", "moisture"])
        timestamp = datetime.now().strftime("%Y-%m-%d %I:%M:%S %p")
        writer.writerow([
            timestamp,
            round(data.temp_f, 2),
            round(data.humidity, 2),
            round(data.lux, 1),
            round(data.moisture, 1)
        ])
    return {"status": "ok", "received": data}

# GET historical data
@app.get("/api/history")
def get_history():
    data = []
    if not log_file.exists():
        return data
    with log_file.open(newline="") as f:
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
            except (ValueError, KeyError) as e:
                print(f"Skipping bad row: {row} - {e}")
                continue
    return data
