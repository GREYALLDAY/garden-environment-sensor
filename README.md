# Garden Environment Sensor

An ESP32-based sensor system to remotely monitor temperature, humidity, light levels, and soil moisture in your garden or greenhouse.


---
> ⚠️ This project is intended for local network use. If exposing it to the public internet, consider adding authentication and HTTPS.


 ## What's New in v1.1.2
- FreeRTOS conversion: firmware now uses parallel tasks instead of loop()
- Deep Sleep support: drastically improved battery life
- ESP32-S3 support
- VEML7700 light sensor added
- Better OTA logic and memory leak fixes


## Features

### FreeRTOS + Deep Sleep Support
- Fully refactored codebase to use FreeRTOS tasks
- ESP now wakes, reads sensors, sends data, and returns to deep sleep
- Major power savings for solar or battery powered setups

### Multi Board Support
- Now supports ESP32-S3!

### Lightweight API
- `POST /api/sensor` — accepts JSON sensor payloads  
- `GET /api/status` — returns basic system status  
- `GET /api/history` — returns historical data from `raw_sensorlog.csv`

### OTA Update Support
- Wirelessly update the firmware using PlatformIO or Arduino IDE
- Shows up as a wireless device, just select the board from the top dropdown and upload your changes

### Wireless Monitoring via Web Dashboard
- Flask/Gunicorn or FastAPI backend
- Displays real-time sensor data and historical charts
- Supports trend tracking, average calculations, and more

### Frontend
- Dashboard built using Jinja2, run with Gunicorn
- Displays a clean, interactive chart powered by [Chart.js](https://www.chartjs.org/)
- Dual Y-axis chart
- Customizable with basic HTML + CSS in `/templates/dashboard.html` and `/static/style.css`

### Live or Periodic Updates
- Real-time updates via HTTP POST or MQTT subscription
- Periodic logging to `raw_sensorlog.csv`
- Utility script `data-cleaner.py` included to clean and validate data, saved as `cleaned_sensorlog.csv`

### Multi-Network Support
- Automatically connects to known Wi-Fi networks
- Easily configurable for multiple locations

### MQTT / Home Assistant Integration
- `flask-mqtt` used to send sensor information to `garden/sensors`
- Easily integrate with Home Assistant to create graphs using `mini-graph-card`
 

---

## Installation & Setup

This guide is assuming you have basic coding/folder structure/terminal knowledge, are familiar with VScode, and know how to wire I2C components to microcontrollers. This is not a full tutorial designed for beginners, however if you have any questions at all please don't hesitate to reach out, and I'll do my best to help you get up and running.

### Requirements

- ESP32 Dev Board (S3 now supported!)
- Raspberry Pi Zero W (or any device to run Flask/FastAPI server)  
- VSCode with PlatformIO extension installed  

#### Supported Sensors:
- HDC1080 – I2C, Temperature & Humidity (Adafruit)
- OPT3001 – I2C, Precision light sensor (TI)
- VEML7700 - I2C, Precision light sensor (Vishay)  
- VH400 - Analog soil moisture sensor (Vegetronix) 

---

### Uploading ESP32 Firmware

1. Open VSCode and install the **PlatformIO** extension if not already installed.
2. Navigate to `esp32-firmware/src/main.cpp` and update the Wi-Fi credentials:
   - Edit the Wi-Fi SSID and password definitions at the top and in the fallback section.

3. Plug in your ESP32 board and confirm the COM port in PlatformIO.

4. Open a terminal and navigate to the firmware folder:

```bash
cd esp32-firmware
```

5. Build the code:

```bash
platformio run
```

6. Upload the firmware:

```bash
platformio run --target upload
```

7. Open the serial monitor to get the device's IP address:

```bash
platformio device monitor --baud 115200
```

If you see unreadable characters, confirm the baud rate or press the reset button on the ESP32.

---

## Running the Web Server

Navigate to the `/dashboard` directory. You'll find two folders:

- `/flask` — lightweight and ideal for devices like the RPi Zero W  
- `/fastapi` — async, scalable, and better for higher performance systems

### Flask with Gunicorn (Basic launch)
```bash
cd garden-dev/dashboard/flask
gunicorn -b 0.0.0.0:8000 dashboard:app
```
 > Note: `dashboard:app` tells Gunicorn to look for the Flask app object named `app` in the `dashboard.py` file.

### Flask with Gunicorn (Using gunicorn.conf.py)
```bash
cd garden-dev/dashboard/flask
gunicorn -c gunicorn.conf.py dashboard:app
```
### Configuring gunicorn.conf.py
In order to use the configuration file, you must first create a `gunicorn.conf.py` file in the same directory as`dashboard.py`. I've included a configuration file that will work with this set up out-of-the-box. Also I have included the full example file from Benoitc's [github repo](https://github.com/benoitc/gunicorn/blob/master/examples/example_config.py)
```python
# gunicorn.conf.py

# Logging
loglevel = 'debug'
capture_output = True
accesslog = 'logs/access.log'
errorlog = 'logs/error.log'

# Workers and performance
workers = 2
threads = 4
worker_class = 'gthread'  # or 'uvicorn.workers.UvicornWorker' if using FastAPI

# Binding
bind = '0.0.0.0:8000'

# Daemon (optional)
# daemon = True
```
- This will add debug level logging, create an `access.log` and `error.log` and store them in the `/logs` directory under `/dashboard/flask`.
- It also adds 2 workers and 4 threads to handle processes concurrently, as well as binds the webserver to 0.0.0.0 port 8000, so any IP address the hosting device uses will bind as the server IP, rather than hardcoding a server IP.
- This can be further utilized if you set a static IP and hostname for your server in /etc/hosts, then you can simply type http://server.local:8000/dashboard rather than the device IP address.
- There are additional options you can add, such as timeout, keep alive, hooks, add a SSL cert, etc. But for now this is plenty to get up and running.


### FastAPI
```bash
cd dashboard/fastapi
uvicorn fastapi:app --reload
```

Once the server is running, visit it in your browser:

```
http://<IP_ADDRESS>:8000/dashboard
```

Replace `<IP_ADDRESS>` with the IP of the device hosting the server.

---

## Troubleshooting

- Ensure both ESP32 and server device are on the same network/subnet
- Restart the server if you modify Flask/FastAPI code.
- Verify sensor wiring and I2C addresses if values are not appearing.
- Press EN button on ESP to reboot after flashing or if not getting any readings after flashing.

---

## Customizing the Dashboard UI

The web interface is built with simple HTML, CSS, and Chart.js. You'll find these files in the appropriate server folder:

```
dashboard/
├── flask/
│   ├── templates/
│   │   └── dashboard.html       # Main HTML template
│   ├── static/
│   │   ├── style.css            # CSS styling
│   │   └── chart.min.js         # Chart.js library
│   ├── logs/
│   │   ├── access.log           # Logs Flask access requests
│   │   ├── error.log            # Logs Flask error messages
│   │   ├── raw_sensorlog.csv    # Main sensor log, stores raw sensor data
│   │   └── cleaned_sensorlog.csv# Cleaned and validated sensor data
│   └── utils/
│       └── data-cleaner.py      # Cleans raw CSV logs into standardized format

  
```

To modify the UI:
- **Edit `dashboard.html`** to change layout, add labels, or display more data.
- **Update `style.css`** to customize colors, fonts, spacing, etc.
- **Replace or extend `chart.min.js`** logic in the script section to add more chart types or interactions.

Feel free to rework the template to match your branding or expand it into a full progressive web app (PWA) down the line.

---

## License

MIT License — open source, modifiable, and free to use.
