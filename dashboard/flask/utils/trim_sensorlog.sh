#!/bin/bash

# Script to trim raw_sensorlog.csv to keep only entries after July 2, 2025
# while retaining headers and removing blank entries

INPUT_FILE="raw_sensorlog.csv"
OUTPUT_FILE="trimmed_sensorlog.csv"
BACKUP_FILE="raw_sensorlog.csv.backup"

# Check if input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: $INPUT_FILE not found!"
    exit 1
fi

# Create backup
echo "Creating backup: $BACKUP_FILE"
cp "$INPUT_FILE" "$BACKUP_FILE"

# Process the file
echo "Processing $INPUT_FILE..."

# Extract header
head -1 "$INPUT_FILE" > "$OUTPUT_FILE"

# Filter entries after July 2, 2025 (2025-07-02) and remove blank lines
# The awk command:
# - Skips the header line (NR > 1)
# - Checks if the line is not empty (NF > 0)
# - Extracts the date from timestamp and compares it to 2025-07-02
awk -F',' '
NR > 1 && NF > 0 {
    # Extract date from timestamp (first 10 characters: YYYY-MM-DD)
    date_str = substr($1, 1, 10)
    if (date_str > "2025-07-02") {
        print $0
    }
}' "$INPUT_FILE" >> "$OUTPUT_FILE"

# Count lines
original_lines=$(wc -l < "$INPUT_FILE")
trimmed_lines=$(wc -l < "$OUTPUT_FILE")
data_lines=$((trimmed_lines - 1))

echo "Original file: $original_lines lines"
echo "Trimmed file: $trimmed_lines lines (including header)"
echo "Data entries kept: $data_lines"

echo "Trimmed file saved as: $OUTPUT_FILE"
echo "Original file backed up as: $BACKUP_FILE"

# Show first few lines of output
echo ""
echo "First 10 lines of trimmed file:"
head -10 "$OUTPUT_FILE"

# Show date range
echo ""
echo "Date range in trimmed file:"
echo "First entry: $(tail -n +2 "$OUTPUT_FILE" | head -1 | cut -d',' -f1)"
echo "Last entry: $(tail -1 "$OUTPUT_FILE" | cut -d',' -f1)"
