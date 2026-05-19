#ifndef CANZ_H
#define CANZ_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * CANZ - CAN Log Compressor Library
 * Version: 1.0.0
 * ============================================================ */

/* ---- Limits ---- */
#define CANZ_MAX_CHANNELS   32
#define CANZ_MAX_IDS        4096
#define CANZ_MAX_PATH       512
#define CANZ_MAX_DLC        64

/* ---- Return codes ---- */
#define CANZ_OK             0
#define CANZ_ERR_IO        -1
#define CANZ_ERR_PARSE     -2
#define CANZ_ERR_SCHEMA    -3
#define CANZ_ERR_MEMORY    -4
#define CANZ_ERR_CONFIG    -5
#define CANZ_ERR_COMPRESS  -6
#define CANZ_ERR_CORRUPT   -7

/* ============================================================
 * CONFIG
 * ============================================================ */

typedef struct {
    char log_file[CANZ_MAX_PATH];
    char compressed_file[CANZ_MAX_PATH];
    char decompressed_file[CANZ_MAX_PATH];
    char schema_file[CANZ_MAX_PATH];
    int  compression_level;      /* 1-22, default 19 */
    int  messages_per_block;     /* default 2,000,000 */
} canz_config_t;

/**
 * Fill config with built-in defaults.
 */
void canz_config_default(canz_config_t *cfg);

/**
 * Load config from an INI-style .conf file.
 * Returns CANZ_OK or CANZ_ERR_CONFIG.
 */
int canz_config_load(const char *path, canz_config_t *cfg);

/**
 * Print the active config to stdout (for debugging).
 */
void canz_config_print(const canz_config_t *cfg);

/* ============================================================
 * SCHEMA
 * ============================================================ */

typedef struct {
    uint32_t id;
    uint8_t  dlc;
} canz_id_schema_t;

typedef struct {
    char name[16];
} canz_channel_schema_t;

typedef struct {
    canz_channel_schema_t channels[CANZ_MAX_CHANNELS];
    int                   num_channels;
    canz_id_schema_t      ids[CANZ_MAX_IDS];
    int                   num_ids;
} canz_schema_t;

/**
 * Load schema from the file specified in cfg->schema_file.
 * Returns CANZ_OK or CANZ_ERR_SCHEMA.
 */
int canz_schema_load(const canz_config_t *cfg, canz_schema_t *schema);

/**
 * Print schema summary to stdout.
 */
void canz_schema_print(const canz_schema_t *schema);

/* ============================================================
 * CONTEXT
 * ============================================================ */

typedef struct canz_ctx canz_ctx_t;

/**
 * Progress callback: called periodically during compress/decompress.
 *   messages_done  - number of CAN messages processed so far
 *   bytes_written  - bytes written to output so far
 *   userdata       - pointer passed to canz_set_progress_cb()
 */
typedef void (*canz_progress_cb_t)(size_t messages_done,
                                   size_t bytes_written,
                                   void  *userdata);

/**
 * Create a library context using the given config.
 * Returns NULL on allocation failure.
 */
canz_ctx_t *canz_ctx_create(const canz_config_t *cfg);

/**
 * Free all resources held by ctx.
 */
void canz_ctx_destroy(canz_ctx_t *ctx);

/**
 * Attach a progress callback (optional, call before compress/decompress).
 */
void canz_set_progress_cb(canz_ctx_t *ctx,
                          canz_progress_cb_t cb,
                          void *userdata);

/**
 * Returns a human-readable description of the last error.
 */
const char *canz_last_error(const canz_ctx_t *ctx);

/**
 * Returns the last numeric error code.
 */
int canz_last_errcode(const canz_ctx_t *ctx);

/* ============================================================
 * CORE OPERATIONS
 * ============================================================ */

/**
 * Compress cfg->log_file → cfg->compressed_file.
 * Schema is loaded from cfg->schema_file.
 *
 * Returns CANZ_OK on success, negative error code on failure.
 */
int canz_compress(canz_ctx_t *ctx);

/**
 * Decompress cfg->compressed_file → cfg->decompressed_file.
 * Schema is embedded in the .canz file; no external schema needed.
 *
 * Returns CANZ_OK on success, negative error code on failure.
 */
int canz_decompress(canz_ctx_t *ctx);

/* ============================================================
 * STATS (available after compress / decompress)
 * ============================================================ */

typedef struct {
    size_t   messages_total;
    size_t   messages_dropped;  /* schema miss (compress only) */
    size_t   bytes_input;
    size_t   bytes_output;
    double   ratio;             /* bytes_input / bytes_output */
    uint32_t num_blocks;
} canz_stats_t;

/**
 * Fill *stats with counters from the last operation.
 * Returns CANZ_OK, or CANZ_ERR_CONFIG if no operation has run yet.
 */
int canz_get_stats(const canz_ctx_t *ctx, canz_stats_t *stats);

/**
 * Print a formatted stats summary to stdout.
 */
void canz_stats_print(const canz_stats_t *stats);

/* ============================================================
 * LOW-LEVEL PARSER (exposed for tools / testing)
 * ============================================================ */

typedef struct {
    uint64_t ts_us;
    char     channel[16];
    uint32_t id;
    uint8_t  payload[CANZ_MAX_DLC];
    uint8_t  dlc;
} canz_msg_t;

/**
 * Parse a single text line in candump format:
 *   (sec.usec) CHANNEL ID#PAYLOAD
 *
 * Returns 1 on success, 0 on parse failure.
 */
int canz_parse_line(const char *line, canz_msg_t *out);

#ifdef __cplusplus
}
#endif

#endif /* CANZ_H */

typedef struct {
    uint64_t ts_us;                  
    uint32_t id;                     
    uint16_t schema_idx;             
    uint8_t  channel_idx;           
    uint8_t  dlc;                    
    uint8_t  payload[CANZ_MAX_DLC];
} canz_stream_msg_t;

typedef struct canz_stream canz_stream_t;


canz_stream_t *canz_stream_open(const char       *path,
                                const canz_schema_t *schema,
                                int               comp_level);


int canz_stream_flush_block(canz_stream_t           *s,
                            const canz_stream_msg_t *msgs,
                            int                      count);

int canz_stream_close(canz_stream_t *s);

const char *canz_stream_last_error(const canz_stream_t *s);

int canz_stream_get_stats(const canz_stream_t *s, canz_stats_t *out);

int canz_schema_resolve(const canz_schema_t *schema,
                        const char          *channel,
                        uint32_t             id,
                        int                 *out_channel_idx,
                        int                 *out_schema_idx);
