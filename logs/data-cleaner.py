import pandas as pd
import os

# Check file exists
if not os.path.exists("raw_sensorlog.csv"):
    raise FileNotFoundError("raw_sensorlog.csv not found")

# Load raw CSV
df = pd.read_csv("raw_sensorlog.csv")

# Drop rows with repeated headers / junk data
df = df[df.applymap(lambda x: isinstance(x, (int, float)) or str(x).replace('.', '', 1).isdigit()).all(axis=1)]

# Reset index
df.reset_index(drop=True, inplace=True)

# Must have at least 5 columns: timestamp, temp_f, humidity, lux, moisture
if df.shape[1] < 5:
    raise ValueError("Expected at least 5 columns:timestamp, temp_f, humidity, lux, moisture.")

# Rename columns
df.columns = ['timestamp', 'temp_f', 'humidity', 'lux', 'moisture'] + [f'extra_{i}' for i in range(5, df.shape[1])]
df = df[['timestamp', 'temp_f', 'humidity', 'lux', 'moisture']]

# Convert timestamp to datetime 
df['timestamp'] = pd.to_datetime(df['timestamp'], errors='coerce')

# Convert other columns to numeric
for col in df.columns:
    if col != "timestamp":
        df[col] = pd.to_numeric(df[col], errors='coerce')

# Drop rows with NaN
df.dropna(inplace=True)

# Drop rows with 0 or negative in any column
df = df[(df[['temp_f', 'humidity', 'lux', 'moisture']]> 0).all(axis=1)]

# Save cleaned output
df.to_csv("clean_sensorlog.csv", index=False)
print("file saved as 'clean_sensorlog.csv'")