#include "canz_internal.h"

canz_ctx_t *canz_ctx_create(const canz_config_t *cfg) {
    canz_ctx_t *ctx = (canz_ctx_t*)calloc(1, sizeof(canz_ctx_t));
    if (!ctx) return NULL;
    ctx->cfg = *cfg;
    return ctx;
}

void canz_ctx_destroy(canz_ctx_t *ctx) {
    if (ctx) free(ctx);
}

void canz_set_progress_cb(canz_ctx_t *ctx,
                          canz_progress_cb_t cb,
                          void *userdata) {
    ctx->progress_cb       = cb;
    ctx->progress_userdata = userdata;
}

const char *canz_last_error(const canz_ctx_t *ctx) {
    if (!ctx) return "NULL context";
    return ctx->errbuf[0] ? ctx->errbuf : "(no error)";
}

int canz_last_errcode(const canz_ctx_t *ctx) {
    return ctx ? ctx->errcode : CANZ_ERR_MEMORY;
}

int canz_get_stats(const canz_ctx_t *ctx, canz_stats_t *stats) {
    if (!ctx->stats_valid) return CANZ_ERR_CONFIG;
    *stats = ctx->stats;
    return CANZ_OK;
}

void canz_stats_print(const canz_stats_t *s) {
    double compressed_percent = (s->bytes_output / (double)s->bytes_input) * 100.0;
    printf("=== CANZ Stats =================================\n");
    printf("  Total Messages       : %zu\n",    s->messages_total);
    printf("  Dropped Messages     : %zu\n",    s->messages_dropped);
    printf("  Input Size           : %.2f MB\n", s->bytes_input  / 1048576.0);
    printf("  Output Size          : %.2f MB\n", s->bytes_output / 1048576.0);
    printf("  Compression Ratio    : %.2fx\n",   s->ratio);
    printf("  Compressed Percentage: %.4f%%\n", compressed_percent);
    printf("  Block Count          : %u\n",      s->num_blocks);
    printf("================================================\n");
}
