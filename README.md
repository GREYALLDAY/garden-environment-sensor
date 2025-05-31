# Garden Environment Sensor

An ESP32-based sensor system to remotely monitor temperature, humidity, light levels, and soil moisture in your garden or greenhouse.


---

## Features

### Lightweight API
- `POST /api/sensor` — accepts JSON sensor payloads  
- `GET /api/status` — returns basic system status  
- `GET /api/history` — returns historical data from `sensor_log.csv`

### Wireless Monitoring via Web Dashboard
- Flask or FastAPI backend
- Displays real-time sensor data and historical charts
- Supports trend tracking, average calculations, and more

### Frontend
- Dashboard built using Jinja2 templating
- Charts powered by [Chart.js](https://www.chartjs.org/)
- Customizable with basic HTML + CSS in `/templates/dashboard.html` and `/static/style.css`


### Live or Periodic Updates
- Real-time updates via HTTP POST
- Periodic logging to `sensor_log.csv`
- All data is sanitized and validated before storage

### Multi-Network Support
- Automatically connects to known Wi-Fi networks
- Easily configurable for multiple locations

### MQTT / Home Assistant Integration
- Coming soon!!

---

## Installation & Setup

This guide is assuming you have basic coding/folder structure/terminal knowledge, are familiar with VScode, and know how to wire I2C components to microcontrollers. This is not a full tutorial designed for beginners, however if you have any questions at all please don't hesitate to reach out, and I'll do my best to help you get up and running.

### Requirements

- ESP32 Dev Board  
- Raspberry Pi Zero W (or any device to run Flask/FastAPI server)  
- VSCode with PlatformIO extension installed  

#### Sensors:
- HDC1080 – Temperature & Humidity  
- BH1750 – Light sensor  
- VH400 or any other analog moisture sensor – Soil Moisture  

---

### Uploading ESP32 Firmware

1. Open VSCode and install the **PlatformIO** extension if not already installed.
2. Navigate to `esp32-firmware/src/main.cpp` and update the Wi-Fi credentials:
   - Edit lines `19`, `20`, `103`, and `105` for two networks.
   - To support additional networks, uncomment and update lines `21`, `22`, `106–109`.

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

- `/flask-dashboard` — lightweight and ideal for devices like the RPi Zero W  
- `/fastapi` — async, scalable, and better for higher performance systems

### Flask
```bash
cd dashboard/flask-dashboard
python flaskserver.py
```

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

- Ensure both ESP32 and server device are on the same network.
- Restart the server if you modify Flask/FastAPI code.
- Verify sensor wiring and I2C addresses if values are not appearing.

---

## Roadmap

- [x] Multi-network support
- [x] Real-time and historical dashboard
- [ ] MQTT integration
- [ ] Home Assistant discovery support
- [ ] Offline SD card logging
- [ ] OTA updates

---

## Customizing the Dashboard UI

The web interface is built with simple HTML, CSS, and [Chart.js](https://www.chartjs.org/). You'll find these files in the appropriate server folder:

```
dashboard/
├── flask-dashboard/
│   ├── templates/
│   │   └── dashboard.html   ← Main HTML template
│   └── static/
│       └── style.css        ← Basic CSS styling
│       └── chart.min.js     ← Chart.js library
```

To modify the UI:
- **Edit `dashboard.html`** to change layout, add labels, or display more data.
- **Update `style.css`** to customize colors, fonts, spacing, etc.
- **Replace or extend `chart.min.js`** logic in the script section to add more chart types or interactions.

Feel free to rework the template to match your branding or expand it into a full progressive web app (PWA) down the line.

---

## Roadmap

- [x] Multi-network support
- [x] Real-time and historical dashboard
- [x] Chart.js integration for historical graphs
- [x] Simple HTML/CSS dashboard layout
- [ ] MQTT integration
- [ ] Home Assistant discovery support
- [ ] Offline SD card logging
- [ ] OTA updates
- [ ] Add theme toggle (dark/light mode)
- [ ] Make dashboard mobile responsive
- [ ] Add real-time chart updates without page refresh
- [ ] Switch to Vue/React frontend (maybe)
- [ ] Create separate admin/config interface

---

## License

MIT License — open source, modifiable, and free to use.
