#include "canz_internal.h"

int canz_schema_load(const canz_config_t *cfg, canz_schema_t *schema) {
    schema->num_channels = 0;
    schema->num_ids      = 0;

    FILE *f = fopen(cfg->schema_file, "r");
    if (!f) {
        fprintf(stderr, "[canz] Schema file could not be opened: %s\n", cfg->schema_file);
        return CANZ_ERR_SCHEMA;
    }

    char line[256];
    int  mode = 0; /* 1=CHANNELS, 2=IDS */

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;

        if (strcmp(line, "[CHANNELS]") == 0) { mode = 1; continue; }
        if (strcmp(line, "[IDS]")      == 0) { mode = 2; continue; }

        if (mode == 1 && schema->num_channels < CANZ_MAX_CHANNELS) {
            strncpy(schema->channels[schema->num_channels].name, line, 15);
            schema->channels[schema->num_channels].name[15] = '\0';
            schema->num_channels++;
        } else if (mode == 2 && schema->num_ids < CANZ_MAX_IDS) {
            uint32_t id; int dlc;
            if (sscanf(line, "%x %d", &id, &dlc) == 2) {
                schema->ids[schema->num_ids].id  = id;
                schema->ids[schema->num_ids].dlc = (uint8_t)dlc;
                schema->num_ids++;
            }
        }
    }

    fclose(f);
    return CANZ_OK;
}

void canz_schema_print(const canz_schema_t *schema) {
    printf("=== CANZ Schema ===\n");
    printf("  Channels (%d):\n", schema->num_channels);
    for (int i = 0; i < schema->num_channels; i++)
        printf("    [%d] %s\n", i, schema->channels[i].name);
    printf("  IDs (%d):\n", schema->num_ids);
    for (int i = 0; i < schema->num_ids; i++)
        printf("    0x%03X  dlc=%d\n", schema->ids[i].id, schema->ids[i].dlc);
    printf("===================\n");
}
