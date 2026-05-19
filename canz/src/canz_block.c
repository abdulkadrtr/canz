#include "canz_block.h"

int canz_write_file_header(FILE *out, const canz_schema_t *schema) {
    int written = 0;

    if (fwrite("CANZ", 1, 4, out) != 4) return -1;
    uint8_t ver = 1;
    if (fwrite(&ver, 1, 1, out) != 1) return -1;
    written += 5;

    uint8_t nch = (uint8_t)schema->num_channels;
    fwrite(&nch, 1, 1, out);
    written += 1;
    for (int i = 0; i < schema->num_channels; i++) {
        char nbuf[16] = {0};
        strncpy(nbuf, schema->channels[i].name, 15);
        fwrite(nbuf, 1, 16, out);
        written += 16;
    }

    uint16_t nids = (uint16_t)schema->num_ids;
    fwrite(&nids, 2, 1, out);
    written += 2;
    for (int i = 0; i < schema->num_ids; i++) {
        fwrite(&schema->ids[i].id,  4, 1, out);
        fwrite(&schema->ids[i].dlc, 1, 1, out);
        written += 5;
    }

    return written;
}

int canz_block_flush(FILE                    *out,
                     const canz_stream_msg_t *msgs,
                     int                      count,
                     const canz_schema_t     *schema,
                     int                      comp_level,
                     size_t                  *bytes_out)
{
    canz_buf_t buf;
    canz_buf_init(&buf, 4 * 1024 * 1024);

    uint8_t vbuf[10];

    int local_to_schema[CANZ_MAX_IDS];
    int schema_to_local[CANZ_MAX_IDS];
    memset(schema_to_local, -1, sizeof(schema_to_local));
    int num_groups = 0;

    for (int i = 0; i < count; i++) {
        int si = msgs[i].schema_idx;
        if (schema_to_local[si] == -1) {
            schema_to_local[si]         = num_groups;
            local_to_schema[num_groups] = si;
            num_groups++;
        }
    }

    canz_buf_write16(&buf, (uint16_t)num_groups);
    canz_buf_write32(&buf, (uint32_t)count);

    for (int i = 0; i < count; i++) {
        int len = canz_varint_enc_u(
                      (uint64_t)schema_to_local[msgs[i].schema_idx], vbuf);
        canz_buf_append(&buf, vbuf, len);
    }

    canz_buf_write64(&buf, msgs[0].ts_us);
    uint64_t prev_ts = msgs[0].ts_us;
    for (int i = 1; i < count; i++) {
        int len = canz_varint_enc_s((int64_t)(msgs[i].ts_us - prev_ts), vbuf);
        canz_buf_append(&buf, vbuf, len);
        prev_ts = msgs[i].ts_us;
    }

    const canz_stream_msg_t **gmsgs = malloc(sizeof(canz_stream_msg_t*) * count);
    if (!gmsgs) { canz_buf_free(&buf); return CANZ_ERR_MEMORY; }

    for (int g = 0; g < num_groups; g++) {
        int     si    = local_to_schema[g];
        uint32_t cid   = schema->ids[si].id;
        uint8_t  c_dlc = schema->ids[si].dlc;

        int gcnt = 0;
        for (int i = 0; i < count; i++)
            if (msgs[i].schema_idx == si)
                gmsgs[gcnt++] = &msgs[i];

        canz_buf_write32(&buf, cid);
        canz_buf_write32(&buf, (uint32_t)gcnt);

        /* İlk mesaj: kanal indeksi + tam payload */
        canz_buf_append(&buf, &gmsgs[0]->channel_idx, 1);
        canz_buf_append(&buf,  gmsgs[0]->payload, c_dlc);

        if (gcnt > 1) {
            /* Kalan mesajların kanal indeksleri */
            for (int i = 1; i < gcnt; i++)
                canz_buf_append(&buf, &gmsgs[i]->channel_idx, 1);

            /* Payload delta: column-major (byte bazında, mesaj bazında delta) */
            for (int b = 0; b < c_dlc; b++) {
                uint8_t prev = gmsgs[0]->payload[b];
                for (int i = 1; i < gcnt; i++) {
                    int delta = (int)gmsgs[i]->payload[b] - (int)prev;
                    int len = canz_varint_enc_s((int64_t)delta, vbuf);
                    canz_buf_append(&buf, vbuf, len);
                    prev = gmsgs[i]->payload[b];
                }
            }
        }
    }
    free(gmsgs);

    size_t  bound = ZSTD_compressBound(buf.size);
    uint8_t *cbuf = malloc(bound);
    if (!cbuf) { canz_buf_free(&buf); return CANZ_ERR_MEMORY; }

    size_t csize = ZSTD_compress(cbuf, bound, buf.data, buf.size, comp_level);
    if (ZSTD_isError(csize)) {
        free(cbuf); canz_buf_free(&buf);
        return CANZ_ERR_COMPRESS;
    }

    uint32_t csize32 = (uint32_t)csize;
    fwrite(&csize32, 4, 1, out);
    fwrite(cbuf, 1, csize, out);
    if (bytes_out) *bytes_out += 4 + csize;

    free(cbuf);
    canz_buf_free(&buf);
    return CANZ_OK;
}
