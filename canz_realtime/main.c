/**
 *   ./canz_realtime              -> compress  (canz_rt.conf)
 *   ./canz_realtime myconf.conf  -> different config
 *   ./canz_realtime -d           -> decompress (canz_rt.conf compressed_file -> decompressed_file)
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/sockios.h>
#include <time.h>

#include "canz.h"

#define DEFAULT_CONF       "canz_rt.conf"
#define BUFFER_SIZE        50000          
#define SOCKET_TIMEOUT_US  100000

static canz_stream_msg_t g_buffer[2][BUFFER_SIZE];

static sem_t              g_sem_data_ready;
static sem_t              g_sem_space_avail;
static int                g_prod_buf = 0;
static int                g_cons_buf = 0;

static volatile sig_atomic_t g_running = 1;

static canz_stream_t  *g_stream = NULL;
static canz_schema_t   g_schema;

static canz_config_t   g_cfg;
static char            g_iface[16] = "vcan0";

/* ============================================================
 * Signal handler — Ctrl+C
 * ============================================================ */
static void handle_sigint(int sig) {
    (void)sig;
    g_running = 0;
    printf("\n[SİSTEM] Shutdown signal received.\n");
}

static void *producer_thread(void *arg) {
    (void)arg;

    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) { perror("[PRODUCER] Socket error."); return NULL; }

    struct ifreq ifr;
    strncpy(ifr.ifr_name, g_iface, IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("[PRODUCER] ioctl SIOCGIFINDEX");
        close(sock);
        g_buffer[g_prod_buf][0].id = 0xFFFFFFFF;
        sem_post(&g_sem_data_ready);
        return NULL;
    }

    int enable_fd = 1;
    setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
               &enable_fd, sizeof(enable_fd));

    struct timeval tv = { .tv_sec = 0, .tv_usec = SOCKET_TIMEOUT_US };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* Bind */
    struct sockaddr_can addr = {
        .can_family  = AF_CAN,
        .can_ifindex = ifr.ifr_ifindex
    };
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[PRODUCER] Bind error.");
        close(sock);
        g_buffer[g_prod_buf][0].id = 0xFFFFFFFF;
        sem_post(&g_sem_data_ready);
        return NULL;
    }

    printf("[PRODUCER] '%s' interface is listening...\n", g_iface);

    int default_ch_idx = 0;
    for (int i = 0; i < g_schema.num_channels; i++) {
        if (strcmp(g_schema.channels[i].name, g_iface) == 0) {
            default_ch_idx = i;
            break;
        }
    }

    int prod_count = 0;
    struct canfd_frame frame;

    while (g_running) {
        int nbytes = read(sock, &frame, sizeof(struct canfd_frame));

        if (nbytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                continue;
            perror("[PRODUCER] Read error.");
            break;
        }

        uint32_t real_id = frame.can_id & CAN_EFF_MASK;

        int ch_idx = default_ch_idx;
        int s_idx  = -1;
        if (canz_schema_resolve(&g_schema, g_iface, real_id,
                                &ch_idx, &s_idx) != CANZ_OK)
            continue;

        uint64_t ts_us;
        struct timeval ts_tv;
        if (ioctl(sock, SIOCGSTAMP, &ts_tv) == 0) {
            ts_us = (uint64_t)ts_tv.tv_sec * 1000000ULL + ts_tv.tv_usec;
        } else {
            struct timespec ts_spec;
            clock_gettime(CLOCK_REALTIME, &ts_spec);
            ts_us = (uint64_t)ts_spec.tv_sec * 1000000ULL
                  + (uint64_t)ts_spec.tv_nsec / 1000;
        }

        canz_stream_msg_t *m = &g_buffer[g_prod_buf][prod_count++];
        m->ts_us       = ts_us;
        m->id          = real_id;
        m->dlc         = frame.len;
        m->channel_idx = (uint8_t)ch_idx;
        m->schema_idx  = (uint16_t)s_idx;
        memcpy(m->payload, frame.data, frame.len);

        if (prod_count == BUFFER_SIZE) {
            int ret;
            do { ret = sem_wait(&g_sem_space_avail); }
            while (ret == -1 && errno == EINTR && g_running);

            if (!g_running) break;

            g_prod_buf  = 1 - g_prod_buf;
            prod_count  = 0;
            sem_post(&g_sem_data_ready);
        }
    }

    g_buffer[g_prod_buf][prod_count].id = 0xFFFFFFFF;
    sem_post(&g_sem_data_ready);

    close(sock);
    printf("[PRODUCER] Stopped.\n");
    return NULL;
}


static void *consumer_thread(void *arg) {
    (void)arg;

    while (1) {
        int ret;
        do { ret = sem_wait(&g_sem_data_ready); }
        while (ret == -1 && errno == EINTR);

        if (ret == -1) { perror("[CONSUMER] sem_wait"); break; }

        int count = 0;
        while (count < BUFFER_SIZE &&
               g_buffer[g_cons_buf][count].id != 0xFFFFFFFF)
            count++;

        int is_last = (g_buffer[g_cons_buf][count].id == 0xFFFFFFFF);

        if (count > 0) {
            int rc = canz_stream_flush_block(g_stream,
                                             g_buffer[g_cons_buf],
                                             count);
            if (rc != CANZ_OK)
                fprintf(stderr, "[CONSUMER] Error: %s\n",
                        canz_stream_last_error(g_stream));
        }

        if (is_last) break;

        g_cons_buf = 1 - g_cons_buf;
        sem_post(&g_sem_space_avail);
    }

    printf("[CONSUMER] Stopped.\n");
    return NULL;
}

static void do_decompress(void) {
    canz_ctx_t *ctx = canz_ctx_create(&g_cfg);
    if (!ctx) { fprintf(stderr, "Context creation failed!\n"); return; }

    int rc = canz_decompress(ctx);
    if (rc != CANZ_OK) {
        fprintf(stderr, "[ERROR] %s\n", canz_last_error(ctx));
    } else {
        canz_stats_t s;
        if (canz_get_stats(ctx, &s) == CANZ_OK) canz_stats_print(&s);
    }

    canz_ctx_destroy(ctx);
}


int main(int argc, char **argv) {
    const char *conf_path     = DEFAULT_CONF;
    int         decompress    = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0)       decompress = 1;
        else if (strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [-d] [conf]\n", argv[0]);
            printf("  (conf default: %s)\n", DEFAULT_CONF);
            return 0;
        } else {
            conf_path = argv[i];
        }
    }

    if (canz_config_load(conf_path, &g_cfg) != CANZ_OK) {
        fprintf(stderr, "[WARNING] '%s' could not be opened, using defaults.\n",
                conf_path);
        canz_config_default(&g_cfg);
    }
    canz_config_print(&g_cfg);

    if (decompress) { do_decompress(); return 0; }

    if (canz_schema_load(&g_cfg, &g_schema) != CANZ_OK) {
        fprintf(stderr, "[ERROR] Failed to load schema: %s\n", g_cfg.schema_file);
        return 1;
    }
    canz_schema_print(&g_schema);

    if (g_schema.num_channels > 0)
        strncpy(g_iface, g_schema.channels[0].name, sizeof(g_iface) - 1);

    g_stream = canz_stream_open(g_cfg.compressed_file,
                                &g_schema,
                                g_cfg.compression_level);
    if (!g_stream) {
        fprintf(stderr, "[ERROR] Failed to open stream: %s\n", g_cfg.compressed_file);
        return 1;
    }

    struct sigaction sa = { .sa_handler = handle_sigint };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sem_init(&g_sem_data_ready,  0, 0);
    sem_init(&g_sem_space_avail, 0, 1);

    pthread_t prod_tid, cons_tid;
    pthread_create(&prod_tid, NULL, producer_thread, NULL);
    pthread_create(&cons_tid, NULL, consumer_thread, NULL);

    pthread_join(prod_tid, NULL);
    pthread_join(cons_tid, NULL);

    sem_destroy(&g_sem_data_ready);
    sem_destroy(&g_sem_space_avail);

    canz_stats_t stats;
    if (canz_stream_get_stats(g_stream, &stats) == CANZ_OK)
        canz_stats_print(&stats);

    canz_stream_close(g_stream);
    printf("[SYSTEM] System shutdown complete.\n");
    return 0;
}
