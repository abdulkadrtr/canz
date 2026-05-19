#!/usr/bin/env python3
"""
Static File CAN Schema Generator
This script generates schema.txt from a candump log file.
"""

import re
import sys
from collections import defaultdict

# ============================================================
# CONFIGURATION
# ============================================================
INPUT_LOG_FILE = "data.log"    # Candump log file to read
OUTPUT_FILE = "schema.txt"     # Output file name
TIME_LIMIT = 10.0              # How many seconds to read (0 = all)

# ============================================================
# Candump Format Parser
# ============================================================
# Example format: (1234567890.123456) can0 123#0102030405060708
CANDUMP_PATTERN = re.compile(
    r'\((\d+\.\d+)\)\s+(\S+)\s+([0-9A-Fa-f]+)#([0-9A-Fa-f]*)'
)

def parse_candump_line(line):
    """
    Parses a line in candump format.
    
    Returns:
        (timestamp, channel, can_id, dlc) or None
    """
    match = CANDUMP_PATTERN.match(line.strip())
    if not match:
        return None
    
    timestamp = float(match.group(1))
    channel = match.group(2)
    can_id = int(match.group(3), 16)
    data = match.group(4)
    dlc = len(data) // 2  # Each byte is 2 hex characters
    
    return timestamp, channel, can_id, dlc


# ============================================================
# Main Function
# ============================================================
def generate_schema_from_file():
    """
    Generates schema.txt from a candump log file.
    """
    print(f"[INFO] Input File: {INPUT_LOG_FILE}")
    print(f"[INFO] Time Limit: {TIME_LIMIT} seconds" if TIME_LIMIT > 0 else "[INFO] Time Limit: Entire file")
    print(f"[INFO] Output File: {OUTPUT_FILE}")
    print("-" * 60)
    
    # Open file
    try:
        with open(INPUT_LOG_FILE, 'r') as f:
            lines = f.readlines()
        print(f"[OK] {len(lines)} lines read.")
    except FileNotFoundError:
        print(f"[ERROR] File not found: {INPUT_LOG_FILE}")
        print("\nSolution:")
        print(f"  1. Check the file path")
        print(f"  2. Create a log with candump:")
        print(f"     candump can0 -l")
        return False
    except Exception as e:
        print(f"[ERROR] Failed to read file: {e}")
        return False
    
    # Data collection
    channels = set()
    id_dlc_map = defaultdict(set)  # ID -> {DLC values}
    message_count = 0
    start_timestamp = None
    
    print(f"[PROCESSING] Analyzing log file...")
    
    for line_num, line in enumerate(lines, 1):
        parsed = parse_candump_line(line)
        
        if parsed is None:
            continue  # Invalid line, skip
        
        timestamp, channel, can_id, dlc = parsed
        
        # Record first message timestamp
        if start_timestamp is None:
            start_timestamp = timestamp
        
        # Time limit check
        elapsed = timestamp - start_timestamp
        if TIME_LIMIT > 0 and elapsed > TIME_LIMIT:
            print(f"[INFO] Time limit ({TIME_LIMIT}s) reached, stopping.")
            break
        
        # Collect data
        channels.add(channel)
        id_dlc_map[can_id].add(dlc)
        message_count += 1
        
        # Progress indicator
        if message_count % 1000 == 0:
            print(f"\r  Messages: {message_count} | "
                  f"Unique IDs: {len(id_dlc_map)} | "
                  f"Elapsed: {elapsed:.1f}s", end="", flush=True)
    
    print()  # New line
    
    # Display results
    print("-" * 60)
    print(f"[RESULT] Total {message_count} messages processed")
    print(f"[RESULT] {len(channels)} channel(s) found")
    print(f"[RESULT] {len(id_dlc_map)} unique CAN ID(s) found")
    
    if len(id_dlc_map) == 0:
        print("[WARNING] No valid CAN messages found!")
        print("          Check the log file format.")
        print("          Expected format: (timestamp) channel ID#DATA")
        return False
    
    # Create schema file
    try:
        with open(OUTPUT_FILE, 'w') as f:
            # CHANNELS section
            f.write("[CHANNELS]\n")
            for channel in sorted(channels):
                f.write(f"{channel}\n")
            f.write("\n")
            
            # IDS section
            f.write("[IDS]\n")
            sorted_ids = sorted(id_dlc_map.keys())
            for idx, can_id in enumerate(sorted_ids):
                # If multiple DLC values exist for one ID, use the most common one
                dlc_values = id_dlc_map[can_id]
                if len(dlc_values) > 1:
                    print(f"[WARNING] Multiple DLC values found for ID 0x{can_id:03X}: {dlc_values}")
                    print(f"          Using maximum DLC value ({max(dlc_values)}).")
                
                dlc = max(dlc_values)  # Use maximum DLC
                # Don't add newline after last ID
                if idx < len(sorted_ids) - 1:
                    f.write(f"{can_id:03X} {dlc}\n")
                else:
                    f.write(f"{can_id:03X} {dlc}")
        
        print(f"[OK] Schema file created: {OUTPUT_FILE}")
        
        # Display file contents (first 20 IDs)
        print("\n" + "=" * 60)
        print("GENERATED SCHEMA (First 20 IDs):")
        print("=" * 60)
        with open(OUTPUT_FILE, 'r') as f:
            lines = f.readlines()
            # Show CHANNELS and IDS headers
            in_ids = False
            id_count = 0
            for line in lines:
                if line.strip() == "[IDS]":
                    in_ids = True
                    print(line, end="")
                elif in_ids and line.strip() and not line.startswith("["):
                    if id_count < 20:
                        print(line, end="")
                        id_count += 1
                    elif id_count == 20:
                        print(f"... (total {len(id_dlc_map)} IDs)")
                        break
                else:
                    print(line, end="")
        print("=" * 60)
        
        return True
        
    except Exception as e:
        print(f"[ERROR] Failed to write file: {e}")
        return False


# ============================================================
# Script Entry Point
# ============================================================
if __name__ == "__main__":
    print("=" * 60)
    print("  Static File CAN Schema Generator")
    print("=" * 60)
    print()
    
    # Command line arguments (optional)
    if len(sys.argv) > 1:
        INPUT_LOG_FILE = sys.argv[1]
    if len(sys.argv) > 2:
        TIME_LIMIT = float(sys.argv[2])
    if len(sys.argv) > 3:
        OUTPUT_FILE = sys.argv[3]
    
    success = generate_schema_from_file()
    
    if success:
        print("\n[SUCCESS] Schema file is ready!")
        sys.exit(0)
    else:
        print("\n[FAILED] Could not generate schema.")
        sys.exit(1)
