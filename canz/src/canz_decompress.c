#include "canz_internal.h"

#define FREAD_OR_FAIL(ptr, sz, n, f, ctx, label)         \
    do {                                                  \
        if (fread((ptr),(sz),(n),(f)) != (size_t)(n)) {   \
            CTX_ERR(ctx, CANZ_ERR_IO, "Beklenmeyen EOF"); \
            goto label;                                   \
        }                                                 \
    } while(0)

int canz_decompress(canz_ctx_t *ctx) {
    printf("[canz] Decompressing: %s\n", ctx->cfg.compressed_file);

    FILE *fin = fopen(ctx->cfg.compressed_file, "rb");
    if (!fin) {
        CTX_ERR(ctx, CANZ_ERR_IO, "File could not be opened: %s", ctx->cfg.compressed_file);
        return CANZ_ERR_IO;
    }

    FILE *fout = fopen(ctx->cfg.decompressed_file, "w");
    if (!fout) {
        fclose(fin);
        CTX_ERR(ctx, CANZ_ERR_IO, "Output file could not be opened: %s", ctx->cfg.decompressed_file);
        return CANZ_ERR_IO;
    }

    int rc = CANZ_OK;

    /* ---- Read file header ---- */
    char magic[4];
    FREAD_OR_FAIL(magic, 1, 4, fin, ctx, cleanup);
    if (memcmp(magic, "CANZ", 4) != 0) {
        CTX_ERR(ctx, CANZ_ERR_CORRUPT, "Magic bytes incorrect, this is not a .canz file");
        rc = CANZ_ERR_CORRUPT; goto cleanup;
    }

    uint8_t version;
    FREAD_OR_FAIL(&version, 1, 1, fin, ctx, cleanup);
    if (version != 1) {
        CTX_ERR(ctx, CANZ_ERR_CORRUPT, "Unsupported format version: %d", version);
        rc = CANZ_ERR_CORRUPT; goto cleanup;
    }

    /* Channel table */
    uint8_t num_channels;
    FREAD_OR_FAIL(&num_channels, 1, 1, fin, ctx, cleanup);
    canz_channel_schema_t channels[CANZ_MAX_CHANNELS];
    for (int i = 0; i < num_channels; i++) {
        char buf[16] = {0};
        FREAD_OR_FAIL(buf, 1, 16, fin, ctx, cleanup);
        strncpy(channels[i].name, buf, 15);
        channels[i].name[15] = '\0';
    }

    /* ID table */
    uint16_t num_ids;
    FREAD_OR_FAIL(&num_ids, 2, 1, fin, ctx, cleanup);
    canz_id_schema_t ids[CANZ_MAX_IDS];
    for (int i = 0; i < num_ids; i++) {
        FREAD_OR_FAIL(&ids[i].id,  4, 1, fin, ctx, cleanup);
        FREAD_OR_FAIL(&ids[i].dlc, 1, 1, fin, ctx, cleanup);
    }

    /* ---- Process blocks ---- */
    size_t total_msgs = 0;
    size_t bytes_read = (size_t)ftell(fin);
    uint32_t block_idx = 0;
    int first_msg = 1;

    uint32_t comp_size;
    while (fread(&comp_size, 4, 1, fin) == 1) {
        uint8_t *comp_data = malloc(comp_size);
        if (!comp_data) { rc = CANZ_ERR_MEMORY; goto cleanup; }
        FREAD_OR_FAIL(comp_data, 1, comp_size, fin, ctx, cleanup_comp);

        unsigned long long dec_cap = ZSTD_getFrameContentSize(comp_data, comp_size);
        if (dec_cap == ZSTD_CONTENTSIZE_ERROR || dec_cap == ZSTD_CONTENTSIZE_UNKNOWN) {
            CTX_ERR(ctx, CANZ_ERR_CORRUPT, "Block %u: ZSTD frame size could not be read", block_idx);
            free(comp_data); rc = CANZ_ERR_CORRUPT; goto cleanup;
        }

        uint8_t *dec = malloc(dec_cap);
        if (!dec) { free(comp_data); rc = CANZ_ERR_MEMORY; goto cleanup; }

        size_t result = ZSTD_decompress(dec, dec_cap, comp_data, comp_size);
        free(comp_data);
        if (ZSTD_isError(result)) {
            CTX_ERR(ctx, CANZ_ERR_CORRUPT, "Block %u: ZSTD error: %s",
                    block_idx, ZSTD_getErrorName(result));
            free(dec); rc = CANZ_ERR_CORRUPT; goto cleanup;
        }

        size_t bof = 0;
        uint16_t num_groups = *(uint16_t*)(dec + bof); bof += 2;
        uint32_t msg_count  = *(uint32_t*)(dec + bof); bof += 4;

        /* Order sequence */
        uint32_t *order = malloc(msg_count * sizeof(uint32_t));
        if (!order) { free(dec); rc = CANZ_ERR_MEMORY; goto cleanup; }
        for (uint32_t i = 0; i < msg_count; i++) {
            uint64_t v; canz_varint_dec_u(dec, &bof, &v);
            order[i] = (uint32_t)v;
        }

        /* Global timestamps */
        uint64_t *gts = malloc(msg_count * sizeof(uint64_t));
        if (!gts) { free(order); free(dec); rc = CANZ_ERR_MEMORY; goto cleanup; }
        gts[0] = *(uint64_t*)(dec + bof); bof += 8;
        for (uint32_t i = 1; i < msg_count; i++) {
            int64_t delta; canz_varint_dec_s(dec, &bof, &delta);
            gts[i] = gts[i-1] + (uint64_t)delta;
        }

        /* Per-group buffers */
        typedef struct { uint32_t id; uint8_t dlc; uint8_t ch; uint8_t pay[CANZ_MAX_DLC]; } _gmsg_t;
        _gmsg_t **gbufs = malloc(num_groups * sizeof(_gmsg_t*));
        uint32_t *gcnts = malloc(num_groups * sizeof(uint32_t));
        uint32_t *ghead = calloc(num_groups, sizeof(uint32_t));
        if (!gbufs || !gcnts || !ghead) {
            free(gbufs); free(gcnts); free(ghead);
            free(gts); free(order); free(dec);
            rc = CANZ_ERR_MEMORY; goto cleanup;
        }

        for (int g = 0; g < num_groups; g++) {
            uint32_t cid    = *(uint32_t*)(dec + bof); bof += 4;
            uint32_t g_cnt  = *(uint32_t*)(dec + bof); bof += 4;
            gcnts[g]        = g_cnt;

            uint8_t dlc = 0;
            for (int i = 0; i < num_ids; i++)
                if (ids[i].id == cid) { dlc = ids[i].dlc; break; }

            _gmsg_t *gb = malloc(g_cnt * sizeof(_gmsg_t));
            if (!gb) { rc = CANZ_ERR_MEMORY; goto cleanup; /* simplified */ }
            gbufs[g] = gb;

            gb[0].id  = cid;
            gb[0].dlc = dlc;
            gb[0].ch  = dec[bof++];
            memcpy(gb[0].pay, dec + bof, dlc); bof += dlc;

            if (g_cnt > 1) {
                for (uint32_t i = 1; i < g_cnt; i++) {
                    gb[i].id = cid; gb[i].dlc = dlc;
                    gb[i].ch = dec[bof++];
                }
                for (int b = 0; b < dlc; b++) {
                    uint8_t prev = gb[0].pay[b];
                    for (uint32_t i = 1; i < g_cnt; i++) {
                        int64_t delta; canz_varint_dec_s(dec, &bof, &delta);
                        prev = (uint8_t)((int)prev + (int)delta);
                        gb[i].pay[b] = prev;
                    }
                }
            }
        }

        /* Reconstruct original order and write */
        for (uint32_t i = 0; i < msg_count; i++) {
            uint32_t  g    = order[i];
            _gmsg_t  *m    = &gbufs[g][ghead[g]++];

            char hex[CANZ_MAX_DLC * 2 + 1];
            for (int b = 0; b < m->dlc; b++)
                sprintf(hex + b*2, "%02X", m->pay[b]);
            hex[m->dlc * 2] = '\0';

            if (!first_msg)
                fputc('\n', fout);

            fprintf(fout, "(%llu.%06llu) %s %03X#%s",
                    (unsigned long long)(gts[i] / 1000000),
                    (unsigned long long)(gts[i] % 1000000),
                    channels[m->ch].name,
                    m->id, hex);

            first_msg = 0;
        }

        total_msgs += msg_count;
        bytes_read  += 4 + comp_size;
        block_idx++;

        for (int g = 0; g < num_groups; g++) free(gbufs[g]);
        free(gbufs); free(gcnts); free(ghead);
        free(gts); free(order); free(dec);

        if (ctx->progress_cb)
            ctx->progress_cb(total_msgs, bytes_read, ctx->progress_userdata);

        continue;

cleanup_comp:
        free(comp_data);
        goto cleanup;
    }

    /* ---- Stats ---- */
    {
        FILE *tmp = fopen(ctx->cfg.compressed_file, "rb");
        size_t comp_bytes = 0;
        if (tmp) { fseek(tmp, 0, SEEK_END); comp_bytes = (size_t)ftell(tmp); fclose(tmp); }

        FILE *tmp2 = fopen(ctx->cfg.decompressed_file, "r");
        size_t decomp_bytes = 0;
        if (tmp2) { fseek(tmp2, 0, SEEK_END); decomp_bytes = (size_t)ftell(tmp2); fclose(tmp2); }

        ctx->stats.messages_total   = total_msgs;
        ctx->stats.messages_dropped = 0;
        ctx->stats.bytes_input      = comp_bytes;
        ctx->stats.bytes_output     = decomp_bytes;
        ctx->stats.ratio            = comp_bytes > 0 ? (double)decomp_bytes / (double)comp_bytes : 0;
        ctx->stats.num_blocks       = block_idx;
        ctx->stats_valid            = 1;
    }

    printf("[canz] Decompression completed -> %s (%zu messages, %u blocks)\n",
           ctx->cfg.decompressed_file, total_msgs, block_idx);

cleanup:
    fclose(fin);
    fclose(fout);
    return rc;
}
