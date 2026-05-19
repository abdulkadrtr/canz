# CANZ — CAN Log Compressor

**Version:** 1.0.0

This repository contains the C implementation of the CANZ data compression architecture, introduced in the white paper.
CANZ achieves compression ratios as low as 3.70% of the original data size at high block sizes (2 million messages).

[White Paper EN](https://github.com/user-attachments/files/28011359/white_paper_en.pdf)

[White Paper TR](https://github.com/user-attachments/files/28011365/white_paper_tr.pdf)

[Youtube Introduction Video](https://www.youtube.com/watch?v=WSdfRWKCbgg)

CANZ is a lossless C library that highly compresses CAN log files in candump format. It supports both file-based and real-time CAN bus usage. Please refer to the white paper for detailed performance analyses.

| Directory | Description |
|-----------|-------------|
| `canz/` | Core compression algorithm implementation |
| `canz_app/` | Contains compression and decompression application for log files. |
| `canz_realtime/` | Contains real-time compression application that listens to CAN bus data. |
| `helpers`| Canz schema file generator |

## CANZ Performance Analysis Summary

| Block    | ZSTD Lvl. | Ratio | Size (%) | CPU (s) | RAM (MB) | Compression (MB/s) | Decompression (MB/s) |
|----------|-----------|-------|----------|---------|----------|--------------------|----------------------|
| 10,000   | 1         | 18.97x | 5.27%    | 5.04    | 3.24     | 94.23              | 23.31                |
| 10,000   | 3         | 19.50x | 5.13%    | 4.73    | 3.57     | 100.97             | 23.22                |
| 10,000   | 9         | 20.47x | 4.88%    | 6.91    | 4.00     | 66.85              | 22.42                |
| 10,000   | 19      | 21.44x | 4.66%    | 41.32   | 5.35     | 10.69              | 18.23                |
| 40,000   | 1         | 20.78x | 4.81%    | 4.97    | 6.04     | 97.16              | 22.87                |
| 40,000   | 3         | 21.41x | 4.67%    | 5.21    | 6.73     | 92.44              | 23.63                |
| 40,000   | 9         | 22.32x | 4.48%    | 5.70    | 12.24    | 80.82              | 24.70                |
| 40,000   | 19      | 23.64x | 4.23%    | 25.96   | 14.66    | 17.06              | 24.44                |
| 100,000  | 1         | 21.78x | 4.59%    | 5.49    | 12.65    | 84.08              | 24.36                |
| 100,000  | 3         | 22.35x | 4.47%    | 6.52    | 13.52    | 72.87              | 24.51                |
| 100,000  | 9         | 23.36x | 4.28%    | 6.94    | 19.47    | 65.95              | 24.21                |
| 100,000  | 19      | 24.74x | 4.04%    | 26.80   | 45.58    | 17.68              | 25.21                |
| 200,000  | 1         | 22.37x | 4.47%    | 5.92    | 22.28    | 79.06              | 25.04                |
| 200,000  | 3         | 22.87x | 4.37%    | 6.10    | 23.16    | 76.71              | 25.32                |
| 200,000  | 9         | 23.97x | 4.17%    | 7.85    | 28.53    | 60.14              | 24.41                |
| 200,000  | 19      | 25.38x | 3.94%    | 23.19   | 71.12    | 20.22              | 24.86                |

---

## Quick Start

First, clone the repository and build the core compression library:

```bash
git clone <repo_url>
cd canz
make
```

---

### File-Based Compression (`canz_app`)

To build the file-based compression application:

```bash
cd ../canz_app
make
```

`canz_app` uses CAN log files in `candump` format as input. A typical log entry looks like the following:

```text
(1685616630.304980) can0 394#D3230000D3439B91
(1685616630.304981) can0 47F#200000000000
(1685616630.304982) can0 340#0400000470008911
(1685616630.304983) can0 381#80E03F0000A74B05
```

After building, place a `candump` formatted `data.log` file inside the `canz_app` directory and run:

```bash
./canz_app
```

This command compresses `data.log` and generates an `output.canz` file.

To restore the compressed data back to its original form:

```bash
./canz_app -d
```

This command decompresses `output.canz` and reconstructs the original CAN log data in a lossless manner.

#### Configuration File

Compression parameters and runtime options are managed through the configuration file. For detailed information, refer to the **Configuration** section in the documentation.

#### Schema File

To run compression on your own CAN data, you must first create a custom `schema.txt` file. This schema should be specifically generated according to the CAN IDs and payload structures of your dataset.

For detailed information about schema generation and format, refer to the **Schema** section in the documentation.

You can use the applications in the `helpers` folder to automatically generate the schema file.

---

### Real-Time Compression (`canz_realtime`)

`canz_realtime` listens to a live CAN interface and compresses incoming CAN frames in real time. For architectural details and real-time design considerations, refer to the white paper.

For quick testing, a Linux `vcan` virtual CAN interface can be used.

First, create and enable the virtual CAN interface:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Then replay a CAN log onto the virtual bus:

```bash
canplayer -I can_data.log vcan0=can0
```

Finally, build and run the real-time compression application:

```bash
cd ../canz_realtime
make
./canz_realtime
```

This starts listening on the `vcan0` interface and performs real-time compression on incoming CAN traffic.

## Table of Contents

1. [Installation](#installation)
2. [Config File](#config-file)
3. [Schema File](#schema-file)
4. [CLI Usage](#cli-usage)
5. [API Reference](#api-reference)
   - [Error Codes](#error-codes)
   - [Config](#config)
   - [Schema](#schema)
   - [Context](#context)
   - [Compress / Decompress](#compress--decompress)
   - [Stats](#stats)
   - [Parser](#parser)
   - [Stream](#stream)
6. [Usage Examples](#usage-examples)

---

## Installation

### Requirements

- GCC ≥ 7 or Clang ≥ 5
- libzstd-dev

```bash
sudo apt install libzstd-dev
```

### Build

```bash
make all
```

### Integration

```bash
gcc main.c -I/path/to/canz/include -L/path/to/canz/build/lib -lcanz -lzstd -o program
```

Can be used with the public header:

```c
#include "canz.h"
```

---

## Config File

```ini
[files]
log_file          = input.log
compressed_file   = output.canz
decompressed_file = decompressed.log
schema_file       = schema.txt

[compression]
compression_level  = 19
messages_per_block = 50000
```

| Key | Default | Description |
|---|---|---|
| `log_file` | `input.log` | Source candump log file |
| `compressed_file` | `output.canz` | Compressed output |
| `decompressed_file` | `decompressed.log` | Decompressed output |
| `schema_file` | `schema.txt` | Channel and ID definition file |
| `compression_level` | `19` | ZSTD level (1–22) |
| `messages_per_block` | `50000` | Number of messages per block |

> **For real-time usage**, `compression_level = 3` is recommended. Please refer to the white paper for detailed performance reports.

---

## Schema File

Defines which channels and CAN IDs will be processed. Messages not found in the schema are skipped and not included in compression.

```
[CHANNELS]
vcan0
vcan1

[IDS]
1A0 8
2B0 4
3FF 8
```

- `[CHANNELS]` — One channel name per line.
- `[IDS]` — ID-DLC pairs in `<hex_id> <dlc>` format.

Schema information is embedded into the `.canz` file during compression; no external schema is needed for decompression.

You can use the applications in the helpers folder to automatically generate the schema file.

---

## CLI Usage

```bash
# Compress
canz -c canz.conf
canz -c canz.conf --stats

# Decompress
canz -d canz.conf
canz -d canz.conf --stats

# File override
canz -c -i input.log -o output.canz -s schema.txt
canz -c -l 3          # Fast compression

# Config and schema info
canz -c --info
```

| Option | Description |
|---|---|
| `-c [conf]` | Compress |
| `-d [conf]` | Decompress |
| `-i <file>` | Input log file override |
| `-o <file>` | Output .canz file override |
| `-s <file>` | Schema file override |
| `-l <1-22>` | ZSTD compression level override |
| `--stats` | Show statistics after operation |
| `--info` | Show config and schema info, then exit |

---

## API Reference

### Error Codes

| Code | Value | Description |
|---|---|---|
| `CANZ_OK` | `0` | Success |
| `CANZ_ERR_IO` | `-1` | File read/write error |
| `CANZ_ERR_PARSE` | `-2` | Log line parsing error |
| `CANZ_ERR_SCHEMA` | `-3` | Schema not found or invalid |
| `CANZ_ERR_MEMORY` | `-4` | Insufficient memory |
| `CANZ_ERR_CONFIG` | `-5` | Config file error |
| `CANZ_ERR_COMPRESS` | `-6` | ZSTD compression error |
| `CANZ_ERR_CORRUPT` | `-7` | Corrupt or invalid .canz file |

---

### Config

```c
void canz_config_default(canz_config_t *cfg);
int  canz_config_load(const char *path, canz_config_t *cfg);
void canz_config_print(const canz_config_t *cfg);
```

#### `canz_config_default`
Fills `cfg` with built-in default values.

#### `canz_config_load`
Reads the config file in INI format. Undefined keys retain their default values.  
**Returns:** `CANZ_OK` or `CANZ_ERR_CONFIG`

#### `canz_config_print`
Writes active config values to stdout.

---

### Schema

```c
int  canz_schema_load(const canz_config_t *cfg, canz_schema_t *schema);
void canz_schema_print(const canz_schema_t *schema);
int  canz_schema_resolve(const canz_schema_t *schema,
                         const char *channel, uint32_t id,
                         int *out_channel_idx, int *out_schema_idx);
```

#### `canz_schema_load`
Loads channel and ID definitions from `cfg->schema_file`.  
**Returns:** `CANZ_OK` or `CANZ_ERR_SCHEMA`

#### `canz_schema_print`
Writes schema contents to stdout.

#### `canz_schema_resolve`
Searches for a channel name and CAN ID in the schema, returns their indices. Used in the Stream API in the producer thread to resolve each message type once.

```c
int ch_idx, s_idx;
canz_schema_resolve(&schema, "vcan0", 0x1A0, &ch_idx, &s_idx);
```

**Returns:** `CANZ_OK`, or `CANZ_ERR_SCHEMA` if not found

---

### Context

```c
canz_ctx_t *canz_ctx_create(const canz_config_t *cfg);
void        canz_ctx_destroy(canz_ctx_t *ctx);
void        canz_set_progress_cb(canz_ctx_t *ctx, canz_progress_cb_t cb, void *userdata);
const char *canz_last_error(const canz_ctx_t *ctx);
int         canz_last_errcode(const canz_ctx_t *ctx);
```

#### `canz_ctx_create`
Creates a library context using the config.  
**Returns:** New context pointer, or `NULL` on error

#### `canz_ctx_destroy`
Releases all resources.

#### `canz_set_progress_cb`
Defines the progress callback. Called after each block is completed.

```c
typedef void (*canz_progress_cb_t)(
    size_t messages_done,
    size_t bytes_written,
    void  *userdata
);
```

#### `canz_last_error`
Returns a human-readable description of the last error.

#### `canz_last_errcode`
Returns the last error code.

---

### Compress / Decompress

```c
int canz_compress(canz_ctx_t *ctx);
int canz_decompress(canz_ctx_t *ctx);
```

#### `canz_compress`
Compresses `cfg->log_file` → `cfg->compressed_file`. The schema is read from `cfg->schema_file` and embedded into the `.canz` file.  
**Returns:** `CANZ_OK` or error code

#### `canz_decompress`
Decompresses `cfg->compressed_file` → `cfg->decompressed_file`. No external schema is needed; it is read from within the file.  
**Returns:** `CANZ_OK` or error code

---

### Stats

```c
int  canz_get_stats(const canz_ctx_t *ctx, canz_stats_t *stats);
void canz_stats_print(const canz_stats_t *stats);
```

#### `canz_get_stats`
Fills the statistics of the last operation. Returns `CANZ_ERR_CONFIG` if no operation has been performed yet.

```c
typedef struct {
    size_t   messages_total;
    size_t   messages_dropped;
    size_t   bytes_input;
    size_t   bytes_output;
    double   ratio;
    uint32_t num_blocks;
} canz_stats_t;
```

#### `canz_stats_print`
Writes statistics in a formatted manner to stdout.

---

### Parser

```c
int canz_parse_line(const char *line, canz_msg_t *out);
```

Parses a single candump line. Can be used for raw log processing or testing.

```
(1234567890.123456) vcan0 1A0#DEADBEEF01020304
```

```c
typedef struct {
    uint64_t ts_us;
    char     channel[16];
    uint32_t id;
    uint8_t  payload[64];
    uint8_t  dlc;
} canz_msg_t;
```

Single-character suffixes at the end of the line (such as `...2C0\n`) are automatically ignored.  
**Returns:** `1` on success, `0` on parse error

---

### Stream

For real-time usage where messages are pushed directly instead of reading from a file. Ping-pong buffer and thread management are handled on the application side; the library is only responsible for compression.

```c
canz_stream_t *canz_stream_open(const char *path,
                                const canz_schema_t *schema,
                                int comp_level);

int  canz_stream_flush_block(canz_stream_t *s,
                             const canz_stream_msg_t *msgs,
                             int count);

int  canz_stream_close(canz_stream_t *s);

const char *canz_stream_last_error(const canz_stream_t *s);

int  canz_stream_get_stats(const canz_stream_t *s, canz_stats_t *out);
```

#### `canz_stream_open`
Opens a new `.canz` stream file and writes the header.  
**Returns:** Stream handle, or `NULL` on error

#### `canz_stream_flush_block`
Compresses the given message array as a single block and writes it to the file. Called on the consumer side of the ping-pong buffer. **Not** thread-safe; must be called only from the consumer thread.  
**Returns:** `CANZ_OK` or error code

#### `canz_stream_close`
Closes the stream, safely finalizes the file, and releases all resources.

#### `canz_stream_last_error`
Returns the description of the last error.

#### `canz_stream_get_stats`
Returns current statistics. Can also be called before `canz_stream_close()`.

#### `canz_stream_msg_t`

```c
typedef struct {
    uint64_t ts_us;
    uint32_t id;
    uint16_t schema_idx;
    uint8_t  channel_idx;
    uint8_t  dlc;
    uint8_t  payload[64];
} canz_stream_msg_t;
```

`channel_idx` and `schema_idx` values must have been previously resolved using `canz_schema_resolve()`.

---

## Usage Examples

### File Compression

```c
#include "canz.h"

int main(void) {
    canz_config_t cfg;
    canz_config_load("canz.conf", &cfg);

    canz_ctx_t *ctx = canz_ctx_create(&cfg);

    if (canz_compress(ctx) == CANZ_OK) {
        canz_stats_t stats;
        canz_get_stats(ctx, &stats);
        canz_stats_print(&stats);
    } else {
        fprintf(stderr, "Error: %s\n", canz_last_error(ctx));
    }

    canz_ctx_destroy(ctx);
    return 0;
}
```

### Real-Time Stream (Ping-Pong Buffer)

```c
#include "canz.h"

#define BUFFER_SIZE 50000

static canz_stream_msg_t buffer[2][BUFFER_SIZE];

/* Producer: for each message received from the CAN bus */
void on_can_message(canz_schema_t *schema, const char *iface,
                    uint32_t id, uint8_t dlc, uint8_t *data,
                    uint64_t ts_us, canz_stream_msg_t *slot)
{
    int ch_idx, s_idx;
    if (canz_schema_resolve(schema, iface, id, &ch_idx, &s_idx) != CANZ_OK)
        return; /* not in schema, skip */

    slot->ts_us       = ts_us;
    slot->id          = id;
    slot->dlc         = dlc;
    slot->channel_idx = (uint8_t)ch_idx;
    slot->schema_idx  = (uint16_t)s_idx;
    memcpy(slot->payload, data, dlc);
}

/* Consumer: single line when buffer is full */
void on_buffer_full(canz_stream_t *stream, int buf_idx, int count) {
    canz_stream_flush_block(stream, buffer[buf_idx], count);
}

int main(void) {
    canz_config_t cfg;
    canz_config_load("canz_rt.conf", &cfg);

    canz_schema_t schema;
    canz_schema_load(&cfg, &schema);

    canz_stream_t *stream = canz_stream_open(
        cfg.compressed_file, &schema, cfg.compression_level);

    /* ... start producer/consumer threads ... */

    canz_stats_t stats;
    canz_stream_get_stats(stream, &stats);
    canz_stats_print(&stats);

    canz_stream_close(stream);
    return 0;
}
```
