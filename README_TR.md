# CANZ — CAN Log Compressor

**Version:** 1.0.0  

Bu depo, teknik belgede tanıtılan CANZ veri sıkıştırma mimarisinin C dilindeki uygulamasını içermektedir. CANZ, 2 milyon mesaja kadar blok boyutlarında %96,3'e varan boyut küçültme (veriyi orijinal boyutunun %3,70'ine kadar sıkıştırma) başarısı elde eder.

[Teknik Rapor EN](https://github.com/user-attachments/files/28011359/white_paper_en.pdf)

[Teknik Rapor TR](https://github.com/user-attachments/files/28011365/white_paper_tr.pdf)

CANZ, candump formatındaki CAN log dosyalarını yüksek oranda sıkıştıran, kayıpsız (lossless) bir C kütüphanesidir. Hem dosya tabanlı hem de gerçek zamanlı CAN hattı kullanımını destekler. Ayrıntılı performans analizleri için teknik raporu inceleyin.

| Dizin | Açıklama |
|-------|----------|
| `canz/` | Çekirdek sıkıştırma algoritması implementasyonu |
| `canz_app/` | Log dosyaları için sıkıştırma ve açma uygulamasını içerir. |
| `canz_realtime/` | Can hattı verisini gerçek zamanlı dinleyen sıkıştırma uygulamasını içerir. |

## CANZ Performans Analizi Özeti

| Blok     | ZSTD Sev. | Oran  | Boyut (%) | CPU (s) | RAM (MB) | Sıkıştırma (MB/s) | Açma (MB/s) |
|----------|-----------|-------|-----------|---------|----------|-------------------|-------------|
| 10.000   | 1         | 18.97x | 5.27%     | 5.04    | 3.24     | 94.23             | 23.31       |
| 10.000   | 3         | 19.50x | 5.13%     | 4.73    | 3.57     | 100.97            | 23.22       |
| 10.000   | 9         | 20.47x | 4.88%     | 6.91    | 4.00     | 66.85             | 22.42       |
| 10.000   | 19        | 21.44x | 4.66%     | 41.32   | 5.35     | 10.69             | 18.23       |
| 40.000   | 1         | 20.78x | 4.81%     | 4.97    | 6.04     | 97.16             | 22.87       |
| 40.000   | 3         | 21.41x | 4.67%     | 5.21    | 6.73     | 92.44             | 23.63       |
| 40.000   | 9         | 22.32x | 4.48%     | 5.70    | 12.24    | 80.82             | 24.70       |
| 40.000   | 19        | 23.64x | 4.23%     | 25.96   | 14.66    | 17.06             | 24.44       |
| 100.000  | 1         | 21.78x | 4.59%     | 5.49    | 12.65    | 84.08             | 24.36       |
| 100.000  | 3         | 22.35x | 4.47%     | 6.52    | 13.52    | 72.87             | 24.51       |
| 100.000  | 9         | 23.36x | 4.28%     | 6.94    | 19.47    | 65.95             | 24.21       |
| 100.000  | 19        | 24.74x | 4.04%     | 26.80   | 45.58    | 17.68             | 25.21       |
| 200.000  | 1         | 22.37x | 4.47%     | 5.92    | 22.28    | 79.06             | 25.04       |
| 200.000  | 3         | 22.87x | 4.37%     | 6.10    | 23.16    | 76.71             | 25.32       |
| 200.000  | 9         | 23.97x | 4.17%     | 7.85    | 28.53    | 60.14             | 24.41       |
| 200.000  | 19        | 25.38x | 3.94%     | 23.19   | 71.12    | 20.22             | 24.86       |

---

## Hızlı Başlangıç

İlk olarak depoyu klonlayın ve çekirdek kütüphaneyi derleyin:

```bash
git clone <repo_url>
cd canz
make
```

---

### Dosya Tabanlı Sıkıştırma (`canz_app`)

Dosya tabanlı sıkıştırma uygulamasını derlemek için:

```bash
cd ../canz_app
make
```

`canz_app`, giriş olarak `candump` formatındaki CAN log dosyalarını kullanır. Tipik bir log verisi aşağıdaki gibi görünmektedir:

```text
(1685616630.304980) can0 394#D3230000D3439B91
(1685616630.304981) can0 47F#200000000000
(1685616630.304982) can0 340#0400000470008911
(1685616630.304983) can0 381#80E03F0000A74B05
```


Derleme tamamlandıktan sonra `canz_app` dizini içerisine `candump` formatında bir `data.log` dosyası yerleştirin ve aşağıdaki komutu çalıştırın:

```bash
./canz_app
```

Bu işlem `data.log` dosyasını sıkıştırarak `output.canz` çıktısını üretir.

Sıkıştırılmış veriyi tekrar orijinal haline dönüştürmek için:

```bash
./canz_app -d
```

Bu komut `output.canz` dosyasını açarak log verisini kayıpsız (lossless) biçimde geri oluşturur.

#### Yapılandırma Dosyası

Sıkıştırma parametreleri ve çalışma seçenekleri `config` dosyası üzerinden yönetilmektedir. Ayrıntılı açıklamalar için dokümandaki **Config Dosyası** bölümünü inceleyin.

#### Schema Dosyası

Kendi CAN veriniz üzerinde sıkıştırma işlemi gerçekleştirmek için öncelikle verinize özel bir `schema.txt` dosyası oluşturmanız gerekir. Bu dosya CAN ID yapısı ve veri alanı özelliklerine göre özelleştirilmelidir.

Schema yapısı ve oluşturma süreci hakkında ayrıntılı bilgi için dokümandaki **Schema Dosyası** bölümünü inceleyin.

---

### Gerçek Zamanlı Sıkıştırma (`canz_realtime`)

`canz_realtime`, CAN hattını gerçek zamanlı dinleyerek akan CAN mesajlarını çevrim içi olarak sıkıştırır. Mimari detaylar ve gerçek zamanlı tasarım yaklaşımı için teknik raporu inceleyin.

Hızlı test için Linux `vcan` sanal CAN arayüzü kullanılabilir.

Öncelikle sanal CAN arayüzünü oluşturun:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Ardından mevcut bir CAN log dosyasını hatta yayınlayın:

```bash
canplayer -I can_data.log vcan0=can0
```

Sonrasında gerçek zamanlı sıkıştırma uygulamasını derleyin ve çalıştırın:

```bash
cd ../canz_realtime
make
./canz_realtime
```

Bu işlem ile `vcan0` üzerinden akan CAN mesajları gerçek zamanlı olarak dinlenir ve sıkıştırma işlemi başlatılır.

## İçindekiler

1. [Kurulum](#kurulum)
2. [Config Dosyası](#config-dosyası)
3. [Schema Dosyası](#schema-dosyası)
4. [CLI Kullanımı](#cli-kullanımı)
5. [API Referansı](#api-referansı)
   - [Hata Kodları](#hata-kodları)
   - [Config](#config)
   - [Schema](#schema)
   - [Context](#context)
   - [Compress / Decompress](#compress--decompress)
   - [Stats](#stats)
   - [Parser](#parser)
   - [Stream](#stream)
6. [Kullanım Örnekleri](#kullanım-örnekleri)

---

## Kurulum

### Gereksinimler

- GCC ≥ 7 veya Clang ≥ 5
- libzstd-dev

```bash
sudo apt install libzstd-dev
```

### Derleme

```bash
make all
```

### Entegrasyon

```bash
gcc main.c -I/path/to/canz/include -L/path/to/canz/build/lib -lcanz -lzstd -o program
```

Public header ile kullanım sağlanabilir:

```c
#include "canz.h"
```

---

## Config Dosyası

```ini
[files]
log_file          = input.log
compressed_file   = output.canz
decompressed_file = decompressed.log
schema_file       = schema.txt

[compression]
compression_level  = 19
messages_per_block = 50000
```

| Anahtar | Varsayılan | Açıklama |
|---|---|---|
| `log_file` | `input.log` | Kaynak candump log dosyası |
| `compressed_file` | `output.canz` | Sıkıştırılmış çıktı |
| `decompressed_file` | `decompressed.log` | Açılmış çıktı |
| `schema_file` | `schema.txt` | Kanal ve ID tanım dosyası |
| `compression_level` | `19` | ZSTD seviyesi (1–22) |
| `messages_per_block` | `50000` | Blok başına mesaj sayısı |

> **Gerçek zamanlı kullanım için** `compression_level = 3` önerilir. Ayrıntılı performans raporu için teknik raporu inceleyin.

---

## Schema Dosyası

Hangi kanalların ve CAN ID'lerinin işleneceğini tanımlar. Schema'da bulunmayan mesajlar atlanır ve sıkıştırmaya dahil edilmez.

```
[CHANNELS]
vcan0
vcan1

[IDS]
1A0 8
2B0 4
3FF 8
```

- `[CHANNELS]` — Satır başına bir kanal adı.
- `[IDS]` — `<hex_id> <dlc>` formatında ID-DLC çiftleri.

Schema bilgisi sıkıştırma yapılırken `.canz` dosyasına gömülür; açma işleminde dış schema gerekmez.

---

## CLI Kullanımı

```bash
# Sıkıştır
canz -c canz.conf
canz -c canz.conf --stats

# Aç
canz -d canz.conf
canz -d canz.conf --stats

# Dosya override
canz -c -i input.log -o output.canz -s schema.txt
canz -c -l 3          # Hızlı sıkıştırma

# Config ve schema bilgisi
canz -c --info
```

| Seçenek | Açıklama |
|---|---|
| `-c [conf]` | Sıkıştır |
| `-d [conf]` | Aç |
| `-i <dosya>` | Giriş log dosyası override |
| `-o <dosya>` | Çıktı .canz dosyası override |
| `-s <dosya>` | Schema dosyası override |
| `-l <1-22>` | ZSTD sıkıştırma seviyesi override |
| `--stats` | İşlem sonunda istatistik göster |
| `--info` | Config ve schema bilgisini göster, çık |

---

## API Referansı

### Hata Kodları

| Kod | Değer | Açıklama |
|---|---|---|
| `CANZ_OK` | `0` | Başarılı |
| `CANZ_ERR_IO` | `-1` | Dosya okuma/yazma hatası |
| `CANZ_ERR_PARSE` | `-2` | Log satırı ayrıştırma hatası |
| `CANZ_ERR_SCHEMA` | `-3` | Schema bulunamadı veya hatalı |
| `CANZ_ERR_MEMORY` | `-4` | Bellek yetersiz |
| `CANZ_ERR_CONFIG` | `-5` | Config dosyası hatası |
| `CANZ_ERR_COMPRESS` | `-6` | ZSTD sıkıştırma hatası |
| `CANZ_ERR_CORRUPT` | `-7` | Bozuk veya geçersiz .canz dosyası |

---

### Config

```c
void canz_config_default(canz_config_t *cfg);
int  canz_config_load(const char *path, canz_config_t *cfg);
void canz_config_print(const canz_config_t *cfg);
```

#### `canz_config_default`
`cfg`'yi yerleşik varsayılan değerlerle doldurur.

#### `canz_config_load`
INI formatındaki config dosyasını okur. Tanımlanmayan anahtarlar varsayılan değerde kalır.  
**Dönüş:** `CANZ_OK` veya `CANZ_ERR_CONFIG`

#### `canz_config_print`
Aktif config değerlerini stdout'a yazar.

---

### Schema

```c
int  canz_schema_load(const canz_config_t *cfg, canz_schema_t *schema);
void canz_schema_print(const canz_schema_t *schema);
int  canz_schema_resolve(const canz_schema_t *schema,
                         const char *channel, uint32_t id,
                         int *out_channel_idx, int *out_schema_idx);
```

#### `canz_schema_load`
`cfg->schema_file` dosyasından kanal ve ID tanımlarını yükler.  
**Dönüş:** `CANZ_OK` veya `CANZ_ERR_SCHEMA`

#### `canz_schema_print`
Schema içeriğini stdout'a yazar.

#### `canz_schema_resolve`
Kanal adı ve CAN ID'sini schema'da arar, indekslerini döner. Stream API'de producer thread'de her mesaj tipini bir kez çözmek için kullanılır.

```c
int ch_idx, s_idx;
canz_schema_resolve(&schema, "vcan0", 0x1A0, &ch_idx, &s_idx);
```

**Dönüş:** `CANZ_OK`, bulunamazsa `CANZ_ERR_SCHEMA`

---

### Context

```c
canz_ctx_t *canz_ctx_create(const canz_config_t *cfg);
void        canz_ctx_destroy(canz_ctx_t *ctx);
void        canz_set_progress_cb(canz_ctx_t *ctx, canz_progress_cb_t cb, void *userdata);
const char *canz_last_error(const canz_ctx_t *ctx);
int         canz_last_errcode(const canz_ctx_t *ctx);
```

#### `canz_ctx_create`
Config'i kullanarak kütüphane bağlamı oluşturur.  
**Dönüş:** Yeni bağlam pointer'ı, hata durumunda `NULL`

#### `canz_ctx_destroy`
Tüm kaynakları serbest bırakır.

#### `canz_set_progress_cb`
İlerleme callback'i tanımlar. Her blok tamamlandığında çağrılır.

```c
typedef void (*canz_progress_cb_t)(
    size_t messages_done,
    size_t bytes_written,
    void  *userdata
);
```

#### `canz_last_error`
Son hatanın okunabilir açıklamasını döner.

#### `canz_last_errcode`
Son hata kodunu döner.

---

### Compress / Decompress

```c
int canz_compress(canz_ctx_t *ctx);
int canz_decompress(canz_ctx_t *ctx);
```

#### `canz_compress`
`cfg->log_file` → `cfg->compressed_file` sıkıştırma yapar. Schema `cfg->schema_file`'dan okunur ve `.canz` dosyasına gömülür.  
**Dönüş:** `CANZ_OK` veya hata kodu

#### `canz_decompress`
`cfg->compressed_file` → `cfg->decompressed_file` açar. Dış schema gerekmez; dosya içinden okunur.  
**Dönüş:** `CANZ_OK` veya hata kodu

---

### Stats

```c
int  canz_get_stats(const canz_ctx_t *ctx, canz_stats_t *stats);
void canz_stats_print(const canz_stats_t *stats);
```

#### `canz_get_stats`
Son işlemin istatistiklerini doldurur. Henüz işlem yapılmadıysa `CANZ_ERR_CONFIG` döner.

```c
typedef struct {
    size_t   messages_total;
    size_t   messages_dropped;
    size_t   bytes_input;
    size_t   bytes_output;
    double   ratio;
    uint32_t num_blocks;
} canz_stats_t;
```

#### `canz_stats_print`
İstatistikleri biçimli olarak stdout'a yazar.

---

### Parser

```c
int canz_parse_line(const char *line, canz_msg_t *out);
```

Tek bir candump satırını ayrıştırır. Ham log işleme veya test amaçlı kullanılabilir.

```
(1234567890.123456) vcan0 1A0#DEADBEEF01020304
```

```c
typedef struct {
    uint64_t ts_us;
    char     channel[16];
    uint32_t id;
    uint8_t  payload[64];
    uint8_t  dlc;
} canz_msg_t;
```

Satır sonundaki tek karakter suffix'ler (`...2C0\n` gibi) otomatik olarak görmezden gelinir.  
**Dönüş:** `1` başarılı, `0` ayrıştırma hatası

---

### Stream

Dosya okumak yerine mesajları doğrudan iten gerçek zamanlı kullanım için. Ping-pong buffer ve thread yönetimi uygulama tarafında yapılır; kütüphane yalnızca sıkıştırma sorumluluğunu alır.

```c
canz_stream_t *canz_stream_open(const char *path,
                                const canz_schema_t *schema,
                                int comp_level);

int  canz_stream_flush_block(canz_stream_t *s,
                             const canz_stream_msg_t *msgs,
                             int count);

int  canz_stream_close(canz_stream_t *s);

const char *canz_stream_last_error(const canz_stream_t *s);

int  canz_stream_get_stats(const canz_stream_t *s, canz_stats_t *out);
```

#### `canz_stream_open`
Yeni bir `.canz` stream dosyası açar ve başlığı yazar.  
**Dönüş:** Stream handle, hata durumunda `NULL`

#### `canz_stream_flush_block`
Verilen mesaj dizisini tek blok olarak sıkıştırıp dosyaya yazar. Ping-pong buffer'ın consumer tarafında çağrılır. Thread-safe **değildir**; yalnızca consumer thread'den çağrılmalıdır.  
**Dönüş:** `CANZ_OK` veya hata kodu

#### `canz_stream_close`
Stream'i kapatır, dosyayı güvenle sonlandırır ve tüm kaynakları serbest bırakır.

#### `canz_stream_last_error`
Son hatanın açıklamasını döner.

#### `canz_stream_get_stats`
Anlık istatistikleri döner. `canz_stream_close()` öncesinde de çağrılabilir.

#### `canz_stream_msg_t`

```c
typedef struct {
    uint64_t ts_us;
    uint32_t id;
    uint16_t schema_idx;
    uint8_t  channel_idx;
    uint8_t  dlc;
    uint8_t  payload[64];
} canz_stream_msg_t;
```

`channel_idx` ve `schema_idx` değerleri `canz_schema_resolve()` ile önceden çözülmüş olmalıdır.

---

## Kullanım Örnekleri

### Dosya Sıkıştırma

```c
#include "canz.h"

int main(void) {
    canz_config_t cfg;
    canz_config_load("canz.conf", &cfg);

    canz_ctx_t *ctx = canz_ctx_create(&cfg);

    if (canz_compress(ctx) == CANZ_OK) {
        canz_stats_t stats;
        canz_get_stats(ctx, &stats);
        canz_stats_print(&stats);
    } else {
        fprintf(stderr, "Hata: %s\n", canz_last_error(ctx));
    }

    canz_ctx_destroy(ctx);
    return 0;
}
```

### Gerçek Zamanlı Stream (Ping-Pong Buffer)

```c
#include "canz.h"

#define BUFFER_SIZE 50000

static canz_stream_msg_t buffer[2][BUFFER_SIZE];

/* Producer: CAN hattından gelen her mesaj için */
void on_can_message(canz_schema_t *schema, const char *iface,
                    uint32_t id, uint8_t dlc, uint8_t *data,
                    uint64_t ts_us, canz_stream_msg_t *slot)
{
    int ch_idx, s_idx;
    if (canz_schema_resolve(schema, iface, id, &ch_idx, &s_idx) != CANZ_OK)
        return; /* schema'da yok, atla */

    slot->ts_us       = ts_us;
    slot->id          = id;
    slot->dlc         = dlc;
    slot->channel_idx = (uint8_t)ch_idx;
    slot->schema_idx  = (uint16_t)s_idx;
    memcpy(slot->payload, data, dlc);
}

/* Consumer: buffer dolunca tek satır */
void on_buffer_full(canz_stream_t *stream, int buf_idx, int count) {
    canz_stream_flush_block(stream, buffer[buf_idx], count);
}

int main(void) {
    canz_config_t cfg;
    canz_config_load("canz_rt.conf", &cfg);

    canz_schema_t schema;
    canz_schema_load(&cfg, &schema);

    canz_stream_t *stream = canz_stream_open(
        cfg.compressed_file, &schema, cfg.compression_level);

    /* ... producer/consumer thread'leri başlat ... */

    canz_stats_t stats;
    canz_stream_get_stats(stream, &stats);
    canz_stats_print(&stats);

    canz_stream_close(stream);
    return 0;
}
```
