#include "canz_internal.h"

#define IS_HEX(c) (((c) >= '0' && (c) <= '9') || \
                   ((c) >= 'A' && (c) <= 'F') || \
                   ((c) >= 'a' && (c) <= 'f'))
#define HEX_VAL(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0')      : \
                    ((c) >= 'A' && (c) <= 'F') ? ((c) - 'A' + 10) : \
                                                 ((c) - 'a' + 10))

int canz_parse_line(const char *line, canz_msg_t *out) {
    const char *p = line;

    /* 1. Timestamp: (sec.usec) */
    if (*p != '(') return 0;
    p++;

    uint64_t sec = 0;
    while (*p >= '0' && *p <= '9') { sec = sec * 10 + (*p - '0'); p++; }
    if (*p != '.') return 0;
    p++;

    uint64_t usec = 0;
    while (*p >= '0' && *p <= '9') { usec = usec * 10 + (*p - '0'); p++; }
    if (*p != ')') return 0;
    p++;

    out->ts_us = sec * 1000000ULL + usec;

    /* 2. Channel */
    while (*p == ' ' || *p == '\t') p++;
    int ch_len = 0;
    while (*p && *p != ' ' && *p != '\t' && ch_len < 15)
        out->channel[ch_len++] = *p++;
    out->channel[ch_len] = '\0';

    /* 3. CAN ID (hex) */
    while (*p == ' ' || *p == '\t') p++;
    uint32_t id = 0;
    while (*p != '#' && *p) {
        char c = *p++;
        uint8_t v;
        if      (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else return 0;
        id = (id << 4) | v;
    }
    if (*p != '#') return 0;
    p++;
    out->id = id;

    out->dlc = 0;
    while (out->dlc < CANZ_MAX_DLC) {
        /* Bosluk/tab atla */
        while (*p == ' ' || *p == '\t') p++;

        char c1 = *p;
        /* Satir sonu veya gecersiz karakter -> dur */
        if (!c1 || c1 == '\n' || c1 == '\r' || !IS_HEX(c1)) break;

        char c2 = *(p + 1);
        /* Ikinci karakter gecersiz (tek suffix) -> dur */
        if (!c2 || c2 == '\n' || c2 == '\r' || !IS_HEX(c2)) break;

        p += 2;
        out->payload[out->dlc++] = (uint8_t)((HEX_VAL(c1) << 4) | HEX_VAL(c2));
    }

    return 1;
}

#undef IS_HEX
#undef HEX_VAL
