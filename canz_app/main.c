
#include <stdio.h>
#include <string.h>
#include "canz.h"

static void on_progress(size_t msgs, size_t bytes, void *ud) {
    (void)ud;
    printf("\r  [%zu msg | %.2f MB]", msgs, bytes / 1048576.0);
    fflush(stdout);
}

int main(int argc, char **argv) {

    const char *conf_path = "canz.conf"; /* varsayılan */
    int decompress_mode   = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            decompress_mode = 1;
        } else {
            conf_path = argv[i];
        }
    }

    canz_config_t cfg;
    if (canz_config_load(conf_path, &cfg) != CANZ_OK) {
        fprintf(stderr, "Config could not be opened, using defaults.\n");
        canz_config_default(&cfg);
    }

    canz_config_print(&cfg);


    canz_ctx_t *ctx = canz_ctx_create(&cfg);
    if (!ctx) {
        fprintf(stderr, "Context creation failed!\n");
        return 1;
    }

    canz_set_progress_cb(ctx, on_progress, NULL);


    int rc;
    if (decompress_mode) {
        printf("[APP] Opening file for decompression...\n");
        rc = canz_decompress(ctx);
    } else {
        printf("[APP] Compressing...\n");
        rc = canz_compress(ctx);
    }

    printf("\n");


    if (rc != CANZ_OK) {
        fprintf(stderr, "[ERROR] %s\n", canz_last_error(ctx));
        canz_ctx_destroy(ctx);
        return 1;
    }

    canz_stats_t stats;
    if (canz_get_stats(ctx, &stats) == CANZ_OK)
        canz_stats_print(&stats);

    canz_ctx_destroy(ctx);
    return 0;
}
