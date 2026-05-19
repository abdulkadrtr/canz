#include "canz_internal.h"

void canz_buf_init(canz_buf_t *b, size_t initial_cap) {
    b->data = (uint8_t*)malloc(initial_cap);
    b->size = 0;
    b->cap  = initial_cap;
}

void canz_buf_append(canz_buf_t *b, const void *data, size_t len) {
    if (b->size + len > b->cap) {
        b->cap  = (b->size + len) * 2;
        b->data = (uint8_t*)realloc(b->data, b->cap);
    }
    memcpy(b->data + b->size, data, len);
    b->size += len;
}

void canz_buf_free(canz_buf_t *b) {
    free(b->data);
    b->data = NULL;
    b->size = b->cap = 0;
}

void canz_buf_write32(canz_buf_t *b, uint32_t v) { canz_buf_append(b, &v, 4); }
void canz_buf_write16(canz_buf_t *b, uint16_t v) { canz_buf_append(b, &v, 2); }
void canz_buf_write64(canz_buf_t *b, uint64_t v) { canz_buf_append(b, &v, 8); }

int canz_varint_enc_u(uint64_t v, uint8_t *out) {
    int len = 0;
    do {
        out[len] = (v & 0x7F) | (v > 0x7F ? 0x80 : 0);
        v >>= 7;
        len++;
    } while (v);
    return len;
}

int canz_varint_enc_s(int64_t v, uint8_t *out) {
    uint64_t uv = ((uint64_t)v << 1) ^ ((uint64_t)(v >> 63));
    return canz_varint_enc_u(uv, out);
}

int canz_varint_dec_u(const uint8_t *data, size_t *off, uint64_t *out) {
    uint64_t val = 0;
    int shift = 0;
    for (;;) {
        uint8_t b = data[(*off)++];
        val |= (uint64_t)(b & 0x7F) << shift;
        shift += 7;
        if (!(b & 0x80)) break;
    }
    *out = val;
    return 0;
}

int canz_varint_dec_s(const uint8_t *data, size_t *off, int64_t *out) {
    uint64_t uv;
    canz_varint_dec_u(data, off, &uv);
    *out = (int64_t)((uv >> 1) ^ -(int64_t)(uv & 1));
    return 0;
}
