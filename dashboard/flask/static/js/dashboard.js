// dashboard.js
// Modular, documented version of your dashboard logic

// ---- Global Constants ---- //
const API_URL = "/api/history";
const ZOOM_START_KEY = "sensorChartZoomStart";
const ZOOM_END_KEY = "sensorChartZoomEnd";

// ---- Utility Functions ---- //

/**
 * Creates a summary card with a value placeholder and sparkline chart.
 * @param {string} id - ID prefix for the card.
 * @param {string} label - The card label/title.
 * @param {string} color - Accent color for the card.
 * @returns {HTMLElement} The generated card element.
 */
function makeCard(id, label, color) {
  const el = document.createElement("div");
  el.className = "card";
  el.innerHTML = `
    <h4>${label}</h4>
    <div class="value" id="${id}-val">--</div>
    <canvas class="spark" id="${id}-spark"></canvas>`;
  el.style.setProperty("--accent", color);
  return el;
}

/**
 * Calculates the average `y` value of a dataset.
 * @param {Array} arr - Array of objects with `y` values.
 * @returns {string} Average rounded to 1 decimal place, or "--" if empty.
 */
function calcAvg(arr) {
  return arr.length ? (arr.reduce((sum, p) => sum + p.y, 0) / arr.length).toFixed(1) : "--";
}

/**
 * Converts API data into Chart.js-friendly datasets (x = timestamp, y = value).
 * @param {Array} data - Array of sensor readings.
 * @param {string} key - The key to map (e.g., "temp_f").
 */
function mapData(data, key) {
  return data.map((e) => ({ x: e.timestamp, y: e[key] }));
}

// ---- UI Update Functions ---- //

/**
 * Builds the summary cards dynamically and appends to the grid.
 * @param {HTMLElement} grid - The container for cards.
 * @param {CSSStyleDeclaration} styles - Computed CSS variables.
 */
function buildSummaryCards(grid, styles) {
  grid.append(
    makeCard("latest", "Last Reading", "#888888"),
    makeCard("avgTemp", "Avg Temp", styles.getPropertyValue("--temp").trim()),
    makeCard("avgHum", "Avg Hum", styles.getPropertyValue("--hum").trim()),
    makeCard("avgLux", "Avg Lux", styles.getPropertyValue("--lux").trim()),
    makeCard("avgMoist", "Avg Moist", styles.getPropertyValue("--moist").trim())
  );
}

/**
 * Updates the values inside the summary cards.
 * @param {Object} latest - Most recent reading.
 * @param {Object} datasets - Object containing arrays for each metric.
 */
function updateSummaryCardValues(latest, datasets) {
  document.getElementById("latest-val").textContent =
    `Temp: ${latest.temp_f}°F\nRH: ${latest.humidity}%\nLux: ${latest.lux}`;
  document.getElementById("avgTemp-val").textContent = `${calcAvg(datasets.temp)}°F`;
  document.getElementById("avgHum-val").textContent = `${calcAvg(datasets.hum)}%`;
  document.getElementById("avgLux-val").textContent = calcAvg(datasets.lux);
  document.getElementById("avgMoist-val").textContent = `${calcAvg(datasets.moist)}%`;
}

/**
 * Renders the sparkline charts for summary cards.
 * @param {Object} datasets - Object containing arrays for each metric.
 * @param {CSSStyleDeclaration} styles - Computed CSS variables for colors.
 */
function renderSparklines(datasets, styles) {
  const sparkCfg = (data, col) => ({
    type: "line",
    data: { datasets: [{ data, borderColor: col, tension: 0.3, fill: false, pointRadius: 0 }] },
    options: { responsive: false, plugins: { legend: { display: false } }, scales: { x: { display: false }, y: { display: false } } }
  });

  new Chart(document.getElementById("avgTemp-spark"), sparkCfg(datasets.temp.slice(-20), styles.getPropertyValue("--temp")));
  new Chart(document.getElementById("avgHum-spark"), sparkCfg(datasets.hum.slice(-20), styles.getPropertyValue("--hum")));
  new Chart(document.getElementById("avgLux-spark"), sparkCfg(datasets.lux.slice(-20), styles.getPropertyValue("--lux")));
  new Chart(document.getElementById("avgMoist-spark"), sparkCfg(datasets.moist.slice(-20), styles.getPropertyValue("--moist")));
}

/**
 * Initializes and returns the main Chart.js instance.
 * @param {Object} datasets - Object containing arrays for each metric.
 * @param {CSSStyleDeclaration} styles - Computed CSS variables for styling.
 */
function renderMainChart(datasets, styles) {
  const ctx = document.getElementById("sensorChart").getContext("2d");
  const savedStart = localStorage.getItem(ZOOM_START_KEY);
  const savedEnd = localStorage.getItem(ZOOM_END_KEY);

  return new Chart(ctx, {
    type: "line",
    data: {
      datasets: [
        { label: "Temp (°F)", data: datasets.temp, borderColor: styles.getPropertyValue("--temp").trim(), pointRadius: 0, tension: 0.25, yAxisID: "y1" },
        { label: "Humidity (%)", data: datasets.hum, borderColor: styles.getPropertyValue("--hum").trim(), pointRadius: 0, tension: 0.25, yAxisID: "y1" },
        { label: "Lux", data: datasets.lux, borderColor: styles.getPropertyValue("--lux").trim(), pointRadius: 0, tension: 0.25, yAxisID: "y2" },
        { label: "Moisture (%)", data: datasets.moist, borderColor: styles.getPropertyValue("--moist").trim(), pointRadius: 0, tension: 0.25, yAxisID: "y1" }
      ]
    },
    options: {
      interaction: { mode: "nearest", intersect: false },
      scales: {
        x: {
          type: "time",
          min: savedStart ? parseInt(savedStart) : undefined,
          max: savedEnd ? parseInt(savedEnd) : undefined,
          time: { tooltipFormat: "MMM d h:mm a" },
          ticks: { source: "data" },
          grid: { color: styles.getPropertyValue("--grid-line") }
        },
        y1: { beginAtZero: true, grid: { color: styles.getPropertyValue("--grid-line") } },
        y2: { beginAtZero: true, position: "right", grid: { drawOnChartArea: false } }
      },
      plugins: {
        legend: { labels: { color: styles.getPropertyValue("--txt-main") } },
        zoom: {
          pan: {
            enabled: true,
            mode: "x",
            modifierKey: "ctrl",
            onPanComplete: ({ chart }) => saveZoom(chart)
          },
          zoom: {
            wheel: { enabled: true, modifierKey: "ctrl", speed: 0.1 },
            pinch: { enabled: true },
            mode: "x",
            onZoomComplete: ({ chart }) => saveZoom(chart)
          }
        }
      }
    }
  });
}

/**
 * Persists zoom state to localStorage.
 * @param {Chart} chart - The Chart.js instance.
 */
function saveZoom(chart) {
  const xAxis = chart.scales.x;
  localStorage.setItem(ZOOM_START_KEY, xAxis.min);
  localStorage.setItem(ZOOM_END_KEY, xAxis.max);
}

/**
 * Sets up event listeners for the time range buttons and reset zoom.
 * @param {Chart} chart - The Chart.js instance.
 */
function setupChartControls(chart) {
  const ranges = {
    "1h": 60 * 60 * 1000,
    "1d": 24 * 60 * 60 * 1000,
    "1w": 7 * 24 * 60 * 60 * 1000
  };

  // Time range buttons
  document.querySelectorAll(".chart-controls button[data-range]").forEach((btn) => {
    btn.addEventListener("click", () => {
      const range = btn.dataset.range;
      const now = Date.now();

      if (range === "all") {
        chart.options.scales.x.min = undefined;
        chart.options.scales.x.max = undefined;
      } else {
        chart.options.scales.x.min = now - ranges[range];
        chart.options.scales.x.max = now;
      }

      localStorage.removeItem(ZOOM_START_KEY);
      localStorage.removeItem(ZOOM_END_KEY);
      chart.update("none");
    });
  });

  // Reset zoom button
  document.getElementById("resetZoom").addEventListener("click", () => {
    chart.resetZoom();
    chart.options.scales.x.min = undefined;
    chart.options.scales.x.max = undefined;
    localStorage.removeItem(ZOOM_START_KEY);
    localStorage.removeItem(ZOOM_END_KEY);
    chart.update("none");
  });
}

// ---- Data Fetch + Initialization ---- //

/**
 * Fetches historical sensor data, updates the UI, and renders charts.
 */
function initDashboard() {
  const grid = document.getElementById("cards");
  const styles = getComputedStyle(document.documentElement);

  buildSummaryCards(grid, styles);

  fetch(API_URL)
    .then((r) => r.json())
    .then((res) => {
      if (res.status !== "ok" || !Array.isArray(res.data) || res.data.length === 0) {
        document.getElementById("latest-val").textContent = "No Data Available";
        return;
      }

      const data = res.data;
      const datasets = {
        temp: mapData(data, "temp_f"),
        hum: mapData(data, "humidity"),
        lux: mapData(data, "lux"),
        moist: mapData(data, "moisture")
      };

      updateSummaryCardValues(data.at(-1), datasets);
      renderSparklines(datasets, styles);

      const chart = renderMainChart(datasets, styles);
      setupChartControls(chart);
    })
    .catch(() => {
      document.getElementById("latest-val").textContent = "Error Loading Data";
    });
}

// ---- Utility Function to Convert Data to CSV ---- //

/**
 * Converts an array of objects to a CSV string.
 * @param {Array} data - The data to convert.
 * @param {Array} columns - The column headers for the CSV.
 * @returns {string} The CSV formatted string.
 */
function convertToCSV(data, columns) {
  const header = columns.join(','); // Join columns with commas
  const rows = data.map(row => {
    return columns.map(col => row[col] || '').join(',');
  }).join('\n');

  return header + '\n' + rows; // Return CSV string with header and rows
}

/**
 * Triggers the download of the CSV file.
 * @param {string} csvContent - The CSV string content.
 * @param {string} filename - The name of the file.
 */
function downloadCSV(csvContent, filename) {
  const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
  const link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = filename;
  link.click();
}

// ---- Add Event Listener for Export CSV Button ---- //

/**
 * Exports the sensor data as a CSV file when the button is clicked.
 */
function setupExportCsvButton(data) {
  const button = document.getElementById("exportCsv");

  button.addEventListener("click", () => {
    // Define the columns we want to include in the CSV file
    const columns = ['timestamp', 'temp_f', 'humidity', 'lux', 'moisture'];

    // Convert the data into CSV format
    const csvContent = convertToCSV(data, columns);

    // Trigger the download with a filename
    const filename = 'sensor_data.csv';
    downloadCSV(csvContent, filename);
  });
}

// ---- Data Fetch + Initialization ---- //

/**
 * Fetches historical sensor data, updates the UI, and renders charts.
 */
function initDashboard() {
  const grid = document.getElementById("cards");
  const styles = getComputedStyle(document.documentElement);

  buildSummaryCards(grid, styles);

  fetch(API_URL)
    .then((r) => r.json())
    .then((res) => {
      if (res.status !== "ok" || !Array.isArray(res.data) || res.data.length === 0) {
        document.getElementById("latest-val").textContent = "No Data Available";
        return;
      }

      const data = res.data;
      const datasets = {
        temp: mapData(data, "temp_f"),
        hum: mapData(data, "humidity"),
        lux: mapData(data, "lux"),
        moist: mapData(data, "moisture")
      };

      updateSummaryCardValues(data.at(-1), datasets);
      renderSparklines(datasets, styles);

      const chart = renderMainChart(datasets, styles);
      setupChartControls(chart);

      // Setup the Export CSV button
      setupExportCsvButton(data);
    })
    .catch(() => {
      document.getElementById("latest-val").textContent = "Error Loading Data";
    });
}

// ---- Bootstrap ---- //
document.addEventListener("DOMContentLoaded", initDashboard);
