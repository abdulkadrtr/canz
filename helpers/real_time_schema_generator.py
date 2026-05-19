#!/usr/bin/env python3
"""
Real-Time CAN Schema Generator
This script listens to a CAN interface and automatically generates a schema.txt file.
"""

import can
import time
import sys
from collections import defaultdict

# ============================================================
# CONFIGURATION
# ============================================================
CAN_INTERFACE = "vcan0"      # CAN interface to listen (can0, vcan0, vcan1, etc.)
BUSTYPE = "socketcan"        # socketcan for Linux
LISTEN_DURATION = 10         # Listen duration in seconds
OUTPUT_FILE = "schema.txt"   # Output file name

# ============================================================
# Main Function
# ============================================================
def generate_schema_from_realtime():
    """
    Generates schema.txt by listening to a CAN interface.
    """
    print(f"[INFO] CAN Interface: {CAN_INTERFACE}")
    print(f"[INFO] Listen Duration: {LISTEN_DURATION} seconds")
    print(f"[INFO] Output File: {OUTPUT_FILE}")
    print("-" * 60)
    
    # Connect to CAN bus
    try:
        bus = can.interface.Bus(channel=CAN_INTERFACE, bustype=BUSTYPE)
        print(f"[OK] Connected to '{CAN_INTERFACE}' interface.")
    except Exception as e:
        print(f"[ERROR] Failed to connect to CAN interface: {e}")
        print("\nSuggested solutions:")
        print("  1. Make sure the CAN interface is active:")
        print(f"     sudo ip link set {CAN_INTERFACE} up type can bitrate 500000")
        print("  2. For virtual CAN:")
        print(f"     sudo modprobe vcan")
        print(f"     sudo ip link add dev {CAN_INTERFACE} type vcan")
        print(f"     sudo ip link set up {CAN_INTERFACE}")
        print("  3. Make sure python-can library is installed:")
        print("     pip install python-can")
        return False
    
    # Data collection
    channels = set()
    id_dlc_map = defaultdict(set)  # ID -> {DLC values}
    message_count = 0
    
    print(f"[LISTENING] Collecting messages for {LISTEN_DURATION} seconds...")
    start_time = time.time()
    
    try:
        while (time.time() - start_time) < LISTEN_DURATION:
            msg = bus.recv(timeout=0.1)
            
            if msg is not None:
                # Channel information (usually interface name for socketcan)
                channel = CAN_INTERFACE
                channels.add(channel)
                
                # CAN ID (extended or standard)
                can_id = msg.arbitration_id
                
                # DLC (Data Length Code)
                dlc = msg.dlc
                
                # Map ID to DLC
                id_dlc_map[can_id].add(dlc)
                
                message_count += 1
                
                # Progress indicator
                if message_count % 100 == 0:
                    elapsed = time.time() - start_time
                    remaining = LISTEN_DURATION - elapsed
                    print(f"\r  Messages: {message_count} | "
                          f"Unique IDs: {len(id_dlc_map)} | "
                          f"Remaining: {remaining:.1f}s", end="", flush=True)
        
        print()  # New line
        
    except KeyboardInterrupt:
        print("\n[WARNING] Interrupted by user.")
    finally:
        bus.shutdown()
    
    # Display results
    print("-" * 60)
    print(f"[RESULT] Total {message_count} messages collected")
    print(f"[RESULT] {len(channels)} channel(s) found")
    print(f"[RESULT] {len(id_dlc_map)} unique CAN ID(s) found")
    
    if len(id_dlc_map) == 0:
        print("[WARNING] No CAN messages received!")
        print("          Make sure there is traffic on the CAN bus.")
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
                # Usually should be fixed, but check for safety
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
        
        # Display file contents
        print("\n" + "=" * 60)
        print("GENERATED SCHEMA:")
        print("=" * 60)
        with open(OUTPUT_FILE, 'r') as f:
            print(f.read())
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
    print("  Real-Time CAN Schema Generator")
    print("=" * 60)
    print()
    
    # Command line arguments (optional)
    if len(sys.argv) > 1:
        CAN_INTERFACE = sys.argv[1]
    if len(sys.argv) > 2:
        LISTEN_DURATION = int(sys.argv[2])
    if len(sys.argv) > 3:
        OUTPUT_FILE = sys.argv[3]
    
    success = generate_schema_from_realtime()
    
    if success:
        print("\n[SUCCESS] Schema file is ready!")
        sys.exit(0)
    else:
        print("\n[FAILED] Could not generate schema.")
        sys.exit(1)
