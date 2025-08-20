import pandas as pd

raw = pd.read_csv("logs/raw_sensorlog.csv", parse_dates=["timestamp"])
veml = pd.read_csv("logs/veml-debug.csv", parse_dates=["timestamp"])

# Merge on timestamp (within 1s tolerance)
merged = pd.merge_asof(
    raw.sort_values("timestamp"),
    veml.sort_values("timestamp"),
    on="timestamp",
    tolerance=pd.Timedelta("1s"),
    direction="nearest"
)

merged.dropna(inplace=True)
merged.to_csv("logs/merged_for_training.csv", index=False)
