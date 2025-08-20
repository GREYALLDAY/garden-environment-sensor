from datetime import datetime

input_file = 'raw_sensorlog.csv'
output_file = 'filtered_data_log.csv'
cutoff = datetime(2025, 6, 27)

with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
    for line in infile:
        try:
            timestamp_str = line.split(',')[0]
            timestamp = datetime.fromisoformat(timestamp_str)
            if timestamp.date() >= cutoff.date():
                outfile.write(line)
        except Exception as e:
            print(f"Skipping line due to error: {e}")
