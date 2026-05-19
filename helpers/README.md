# CANZ Schema Generator Tools

This folder contains helper tools for automatically generating `schema.txt` files for the CANZ library.

## Contents

- [real_time_schema_generator.py](#1-real-time-schema-generator) - Collect data from live CAN interface
- [static_file_schema_generator.py](#2-static-file-schema-generator) - Generate schema from candump log file

---

## What is schema.txt?

The CANZ library requires a schema file to compress CAN messages. This file contains:

```
[CHANNELS]
can0
vcan0

[IDS]
07F 8
130 8
140 8
...
```

- **CHANNELS**: CAN bus channel names (e.g., `can0`, `vcan0`)
- **IDS**: CAN message IDs (hex) and DLC (Data Length Code) values

---

## In-Code Configuration

Both scripts contain configuration variables that can be modified:

### real_time_schema_generator.py

```python
CAN_INTERFACE = "vcan0"      # CAN interface to listen
BUSTYPE = "socketcan"        # socketcan for Linux
LISTEN_DURATION = 10         # Listen duration in seconds
OUTPUT_FILE = "schema.txt"   # Output file name
```

### static_file_schema_generator.py

```python
INPUT_LOG_FILE = "data.log"    # Candump log file to read
OUTPUT_FILE = "schema.txt"     # Output file name
TIME_LIMIT = 10.0              # How many seconds to read (0 = all)
```

You can modify these values directly in the script or override them with command line arguments.
