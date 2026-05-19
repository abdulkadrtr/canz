#include "canz_internal.h"
#include "canz_block.h"

struct canz_stream {
    FILE          *fout;
    canz_schema_t  schema;
    int            comp_level;

    /* İstatistikler */
    size_t   total_msgs;
    size_t   total_blocks;
    size_t   bytes_written;

    /* Hata durumu */
    char errbuf[512];
    int  errcode;
};

canz_stream_t *canz_stream_open(const char          *path,
                                const canz_schema_t *schema,
                                int                  comp_level)
{
    if (!path || !schema) return NULL;

    canz_stream_t *s = (canz_stream_t *)calloc(1, sizeof(canz_stream_t));
    if (!s) return NULL;

    s->fout       = fopen(path, "wb");
    s->comp_level = (comp_level >= 1 && comp_level <= 22) ? comp_level : 19;
    s->schema     = *schema;

    if (!s->fout) {
        snprintf(s->errbuf, sizeof(s->errbuf),
                 "Output file could not be opened: %s", path);
        s->errcode = CANZ_ERR_IO;
        free(s);
        return NULL;
    }

    int hdr = canz_write_file_header(s->fout, schema);
    if (hdr < 0) {
        snprintf(s->errbuf, sizeof(s->errbuf), "Failed to write file header");
        s->errcode = CANZ_ERR_IO;
        fclose(s->fout);
        free(s);
        return NULL;
    }

    s->bytes_written = (size_t)hdr;

    printf("[canz_stream] Opened: %s  (level=%d)\n", path, s->comp_level);
    return s;
}

int canz_stream_flush_block(canz_stream_t           *s,
                            const canz_stream_msg_t *msgs,
                            int                      count)
{
    if (!s || !msgs || count <= 0) return CANZ_ERR_IO;

    size_t block_bytes = 0;
    int rc = canz_block_flush(s->fout, msgs, count,
                              &s->schema, s->comp_level,
                              &block_bytes);
    if (rc != CANZ_OK) {
        snprintf(s->errbuf, sizeof(s->errbuf),
                 "Block compression error (block #%zu)", s->total_blocks);
        s->errcode = rc;
        return rc;
    }

    fflush(s->fout);

    s->total_msgs    += (size_t)count;
    s->total_blocks  += 1;
    s->bytes_written += block_bytes;

    printf("[canz_stream] Block #%zu written: %d messages | %.2f KB\n",
           s->total_blocks, count, block_bytes / 1024.0);

    return CANZ_OK;
}

int canz_stream_close(canz_stream_t *s) {
    if (!s) return CANZ_ERR_IO;

    int rc = CANZ_OK;

    if (s->fout) {
        fflush(s->fout);
        fclose(s->fout);
        s->fout = NULL;
    }

    printf("[canz_stream] Closed. Total: %zu messages, %zu blocks, %.2f MB\n",
           s->total_msgs,
           s->total_blocks,
           s->bytes_written / 1048576.0);

    rc = s->errcode;
    free(s);
    return rc;
}

const char *canz_stream_last_error(const canz_stream_t *s) {
    if (!s) return "NULL stream";
    return s->errbuf[0] ? s->errbuf : "(no error)";
}

int canz_stream_get_stats(const canz_stream_t *s, canz_stats_t *out) {
    if (!s || !out) return CANZ_ERR_IO;

    out->messages_total   = s->total_msgs;
    out->messages_dropped = 0;      
    out->bytes_input      = 0;     
    out->bytes_output     = s->bytes_written;
    out->ratio            = 0.0;
    out->num_blocks       = (uint32_t)s->total_blocks;
    return CANZ_OK;
}


int canz_schema_resolve(const canz_schema_t *schema,
                        const char          *channel,
                        uint32_t             id,
                        int                 *out_channel_idx,
                        int                 *out_schema_idx)
{
    int ch_idx = -1;
    for (int i = 0; i < schema->num_channels; i++) {
        if (strcmp(schema->channels[i].name, channel) == 0) {
            ch_idx = i;
            break;
        }
    }

    int s_idx = -1;
    for (int i = 0; i < schema->num_ids; i++) {
        if (schema->ids[i].id == id) {
            s_idx = i;
            break;
        }
    }

    if (ch_idx == -1 || s_idx == -1) return CANZ_ERR_SCHEMA;

    if (out_channel_idx) *out_channel_idx = ch_idx;
    if (out_schema_idx)  *out_schema_idx  = s_idx;
    return CANZ_OK;
}
