#include "canz_internal.h"
#include <ctype.h>

void canz_config_default(canz_config_t *cfg) {
    strncpy(cfg->log_file,          "input.log",        CANZ_MAX_PATH - 1);
    strncpy(cfg->compressed_file,   "output.canz",      CANZ_MAX_PATH - 1);
    strncpy(cfg->decompressed_file, "decompressed.log", CANZ_MAX_PATH - 1);
    strncpy(cfg->schema_file,       "schema.txt",       CANZ_MAX_PATH - 1);
    cfg->compression_level  = 19;
    cfg->messages_per_block = 2000000;
}

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

int canz_config_load(const char *path, canz_config_t *cfg) {
    canz_config_default(cfg);

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[canz] Config file could not be opened: %s\n", path);
        return CANZ_ERR_CONFIG;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        char *semi = strchr(line, ';');
        if (semi) *semi = '\0';
        line[strcspn(line, "\r\n")] = '\0';

        char *s = trim(line);
        if (*s == '\0' || *s == '[') continue;

        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';

        char *key = trim(s);
        char *val = trim(eq + 1);

        if      (strcmp(key, "log_file")          == 0) strncpy(cfg->log_file,          val, CANZ_MAX_PATH - 1);
        else if (strcmp(key, "compressed_file")   == 0) strncpy(cfg->compressed_file,   val, CANZ_MAX_PATH - 1);
        else if (strcmp(key, "decompressed_file") == 0) strncpy(cfg->decompressed_file, val, CANZ_MAX_PATH - 1);
        else if (strcmp(key, "schema_file")       == 0) strncpy(cfg->schema_file,       val, CANZ_MAX_PATH - 1);
        else if (strcmp(key, "compression_level") == 0) cfg->compression_level  = atoi(val);
        else if (strcmp(key, "messages_per_block")== 0) cfg->messages_per_block = atoi(val);
        else fprintf(stderr, "[canz] Unknown config key: '%s'\n", key);
    }

    fclose(f);
    return CANZ_OK;
}

void canz_config_print(const canz_config_t *cfg) {
    printf("=== CANZ Config ===\n");
    printf("  log_file          : %s\n", cfg->log_file);
    printf("  compressed_file   : %s\n", cfg->compressed_file);
    printf("  decompressed_file : %s\n", cfg->decompressed_file);
    printf("  schema_file       : %s\n", cfg->schema_file);
    printf("  compression_level : %d\n", cfg->compression_level);
    printf("  messages_per_block: %d\n", cfg->messages_per_block);
    printf("===================\n");
}
