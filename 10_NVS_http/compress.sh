#!/usr/bin/env bash

INPUT_FILE=$1
OUTPUT_FILE=$2

if [ -z "$INPUT_FILE" ] || [ -z "$OUTPUT_FILE" ]; then
  echo "Usage: $0 <input_file> <output_file>"
  exit 1
fi
sed -E ':a;N;$!ba; 
    # s/<!--.*-->//g;          # Remove html comments
    s/>\s+</></g;            # Delete spaces between tags
    s/>\s+/>/g;              # Delete spaces after >
    s/\s+</</g;              # Delete spaces before <
    s/\s+/ /g;               # Delete multiple spaces
    s/\n//g' "$INPUT_FILE" >"$OUTPUT_FILE"
