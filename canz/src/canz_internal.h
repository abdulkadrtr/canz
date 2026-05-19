#ifndef CANZ_INTERNAL_H
#define CANZ_INTERNAL_H

#include "canz.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zstd.h>

/* ---- ByteBuffer ---- */
typedef struct {
    uint8_t *data;
    size_t   size;
    size_t   cap;
} canz_buf_t;

void canz_buf_init   (canz_buf_t *b, size_t initial_cap);
void canz_buf_append (canz_buf_t *b, const void *data, size_t len);
void canz_buf_free   (canz_buf_t *b);
void canz_buf_write32(canz_buf_t *b, uint32_t v);
void canz_buf_write16(canz_buf_t *b, uint16_t v);
void canz_buf_write64(canz_buf_t *b, uint64_t v);

/* ---- Varint ---- */
int canz_varint_enc_u(uint64_t  v, uint8_t *out);
int canz_varint_enc_s(int64_t   v, uint8_t *out);
int canz_varint_dec_u(const uint8_t *data, size_t *off, uint64_t *out);
int canz_varint_dec_s(const uint8_t *data, size_t *off, int64_t  *out);

/* ---- Context (private definition) ---- */
struct canz_ctx {
    canz_config_t  cfg;
    canz_schema_t  schema;
    char           errbuf[512];
    int            errcode;
    canz_stats_t   stats;
    int            stats_valid;
    canz_progress_cb_t progress_cb;
    void              *progress_userdata;
};

/* ---- Helpers ---- */
#define CTX_ERR(ctx, code, ...)                        \
    do {                                               \
        (ctx)->errcode = (code);                       \
        snprintf((ctx)->errbuf, sizeof((ctx)->errbuf), \
                 __VA_ARGS__);                         \
    } while(0)

#endif /* CANZ_INTERNAL_H */
