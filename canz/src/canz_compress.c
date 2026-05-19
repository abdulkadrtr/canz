#include "canz_internal.h"
#include "canz_block.h"

int canz_compress(canz_ctx_t *ctx) {
    int rc = canz_schema_load(&ctx->cfg, &ctx->schema);
    if (rc != CANZ_OK) {
        CTX_ERR(ctx, CANZ_ERR_SCHEMA, "Schema could not be loaded: %s", ctx->cfg.schema_file);
        return rc;
    }

    printf("[canz] Compressing: %s -> %s\n",
           ctx->cfg.log_file, ctx->cfg.compressed_file);

    FILE *fin = fopen(ctx->cfg.log_file, "r");
    if (!fin) {
        CTX_ERR(ctx, CANZ_ERR_IO, "Log file could not be opened: %s", ctx->cfg.log_file);
        return CANZ_ERR_IO;
    }

    FILE *fout = fopen(ctx->cfg.compressed_file, "wb");
    if (!fout) {
        fclose(fin);
        CTX_ERR(ctx, CANZ_ERR_IO, "Output file could not be opened: %s", ctx->cfg.compressed_file);
        return CANZ_ERR_IO;
    }

    /* Dosya başlığı */
    int hdr = canz_write_file_header(fout, &ctx->schema);
    if (hdr < 0) {
        fclose(fin); fclose(fout);
        CTX_ERR(ctx, CANZ_ERR_IO, "Header writing error");
        return CANZ_ERR_IO;
    }

    /* Blok tamponu — canz_stream_msg_t kullanıyoruz artık */
    int bsz = ctx->cfg.messages_per_block;
    canz_stream_msg_t *block = malloc(sizeof(canz_stream_msg_t) * bsz);
    if (!block) {
        fclose(fin); fclose(fout);
        CTX_ERR(ctx, CANZ_ERR_MEMORY, "Block malloc error");
        return CANZ_ERR_MEMORY;
    }

    int      msg_count  = 0;
    size_t   total_msgs = 0;
    size_t   dropped    = 0;
    size_t   bytes_out  = (size_t)hdr;
    uint32_t num_blocks = 0;
    char     line[512];
    canz_msg_t m;

    while (1) {
        char *ok = fgets(line, sizeof(line), fin);

        if (ok) {
            if (!canz_parse_line(line, &m)) continue;

            int ch_idx = -1;
            for (int i = 0; i < ctx->schema.num_channels; i++)
                if (strcmp(ctx->schema.channels[i].name, m.channel) == 0)
                    { ch_idx = i; break; }

            int s_idx = -1;
            for (int i = 0; i < ctx->schema.num_ids; i++)
                if (ctx->schema.ids[i].id == m.id)
                    { s_idx = i; break; }

            if (ch_idx == -1 || s_idx == -1) { dropped++; continue; }

            canz_stream_msg_t *bm = &block[msg_count++];
            bm->ts_us       = m.ts_us;
            bm->id          = m.id;
            bm->dlc         = m.dlc;
            bm->channel_idx = (uint8_t)ch_idx;
            bm->schema_idx  = (uint16_t)s_idx;
            memcpy(bm->payload, m.payload, m.dlc);
            total_msgs++;
        }

        if (msg_count == bsz || (!ok && msg_count > 0)) {
            rc = canz_block_flush(fout, block, msg_count,
                                  &ctx->schema,
                                  ctx->cfg.compression_level,
                                  &bytes_out);
            if (rc != CANZ_OK) {
                free(block); fclose(fin); fclose(fout);
                CTX_ERR(ctx, rc, "Block compression error (block #%u)", num_blocks);
                return rc;
            }
            num_blocks++;
            msg_count = 0;

            if (ctx->progress_cb)
                ctx->progress_cb(total_msgs, bytes_out, ctx->progress_userdata);
        }

        if (!ok) break;
    }

    free(block);
    fclose(fin);
    fclose(fout);

    FILE *tmp = fopen(ctx->cfg.log_file, "r");
    size_t input_bytes = 0;
    if (tmp) { fseek(tmp, 0, SEEK_END); input_bytes = (size_t)ftell(tmp); fclose(tmp); }

    ctx->stats.messages_total   = total_msgs;
    ctx->stats.messages_dropped = dropped;
    ctx->stats.bytes_input      = input_bytes;
    ctx->stats.bytes_output     = bytes_out;
    ctx->stats.ratio = input_bytes > 0 ? (double)input_bytes / (double)bytes_out : 0;
    ctx->stats.num_blocks       = num_blocks;
    ctx->stats_valid            = 1;

    printf("[canz] Completed. %zu messages processed, %u blocks, "
           "%zu dropped. %.2fx compression.\n",
           total_msgs, num_blocks, dropped, ctx->stats.ratio);

    return CANZ_OK;
}
