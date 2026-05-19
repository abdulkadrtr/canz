/*
 * canz - CAN Log Compressor CLI
 *
 * Usage:
 *   canz [OPTIONS]
 *
 * Options:
 *   -c [CONF]    Compress using config file (default: canz.conf)
 *   -d [CONF]    Decompress using config file (default: canz.conf)
 *   -i <file>    Override: input log file
 *   -o <file>    Override: output .canz file
 *   -s <file>    Override: schema file
 *   -l <1-22>    Override: ZSTD compression level
 *   --stats      Print stats after operation
 *   --info       Print config and schema, then exit
 *   -h           Show this help
 */

#include "../include/canz.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(const char *argv0) {
    printf(
        "Usage: %s [OPTIONS]\n\n"
        "Modes:\n"
        "  -c [conf]    Compress (default: canz.conf)\n"
        "  -d [conf]    Decompress (default: canz.conf)\n\n"
        "Override options:\n"
        "  -i <file>    Input log file\n"
        "  -o <file>    Output .canz file\n"
        "  -s <file>    Schema file\n"
        "  -l <1-22>    ZSTD compression level\n\n"
        "Other:\n"
        "  --stats      Show statistics after operation\n"
        "  --info       Show config and schema info, then exit\n"
        "  -h           Show this help message\n",
        argv0
    );
}

/* ---- Simple progress bar ---- */
static void progress_cb(size_t msgs, size_t bytes, void *ud) {
    (void)ud;
    printf("\r  Processing: %zu messages | %.1f MB written  ",
           msgs, bytes / 1048576.0);
    fflush(stdout);
}

int main(int argc, char **argv) {
    if (argc < 2) { print_help(argv[0]); return 1; }

    int   mode       = 0;        /* 1=compress, 2=decompress */
    int   show_stats = 0;
    int   show_info  = 0;
    char  conf_path[CANZ_MAX_PATH] = "canz.conf";

    /* Overrides (empty = not set) */
    char ov_input[CANZ_MAX_PATH]  = "";
    char ov_output[CANZ_MAX_PATH] = "";
    char ov_schema[CANZ_MAX_PATH] = "";
    int  ov_level = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]); return 0;
        } else if (strcmp(argv[i], "--stats") == 0) {
            show_stats = 1;
        } else if (strcmp(argv[i], "--info") == 0) {
            show_info = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            mode = 1;
            if (i + 1 < argc && argv[i+1][0] != '-')
                strncpy(conf_path, argv[++i], CANZ_MAX_PATH - 1);
        } else if (strcmp(argv[i], "-d") == 0) {
            mode = 2;
            if (i + 1 < argc && argv[i+1][0] != '-')
                strncpy(conf_path, argv[++i], CANZ_MAX_PATH - 1);
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            strncpy(ov_input,  argv[++i], CANZ_MAX_PATH - 1);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            strncpy(ov_output, argv[++i], CANZ_MAX_PATH - 1);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            strncpy(ov_schema, argv[++i], CANZ_MAX_PATH - 1);
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            ov_level = atoi(argv[++i]);
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_help(argv[0]); return 1;
        }
    }

    if (mode == 0) { print_help(argv[0]); return 1; }

    /* Load config */
    canz_config_t cfg;
    int rc = canz_config_load(conf_path, &cfg);
    if (rc != CANZ_OK) {
        fprintf(stderr, "[canz] Config loading error, using defaults.\n");
        canz_config_default(&cfg);
    }

    /* Apply overrides */
    if (ov_input[0])  strncpy(cfg.log_file,        ov_input,  CANZ_MAX_PATH - 1);
    if (ov_output[0]) strncpy(cfg.compressed_file,  ov_output, CANZ_MAX_PATH - 1);
    if (ov_schema[0]) strncpy(cfg.schema_file,      ov_schema, CANZ_MAX_PATH - 1);
    if (ov_level > 0) cfg.compression_level = ov_level;

    /* --info: print and exit */
    if (show_info) {
        canz_config_print(&cfg);
        canz_schema_t schema;
        if (canz_schema_load(&cfg, &schema) == CANZ_OK)
            canz_schema_print(&schema);
        return 0;
    }

    /* Create context and run */
    canz_ctx_t *ctx = canz_ctx_create(&cfg);
    if (!ctx) { fprintf(stderr, "Context creation failed!\n"); return 1; }

    canz_set_progress_cb(ctx, progress_cb, NULL);

    if (mode == 1) rc = canz_compress(ctx);
    else           rc = canz_decompress(ctx);

    printf("\n"); /* newline after progress bar */

    if (rc != CANZ_OK) {
        fprintf(stderr, "[ERROR] %s (code: %d)\n", canz_last_error(ctx), rc);
        canz_ctx_destroy(ctx);
        return 1;
    }

    if (show_stats) {
        canz_stats_t stats;
        if (canz_get_stats(ctx, &stats) == CANZ_OK)
            canz_stats_print(&stats);
    }

    canz_ctx_destroy(ctx);
    return 0;
}
