/*
 dci_parser.c
 Простой парсер ключевых частей картографического формата (ЦММ / ЦМР) по спецификации из
 "Правила цифрового описания картографической информации (Версия 2.0)".

 Компиляция:
   gcc -std=c99 -O2 dci_parser.c -o dci_parser

 Пример запуска:
   ./dci_parser -f data.dci -t 0x2000 -n 4096 -p 317.5 -o 500000,0 -r 0x8000 -m 1024 -e le
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

/* ---- настройки по умолчанию ---- */
#define DEFAULT_SQUARE_PIXELS 64   /* квадраты 64x64 пикселей (фиксировано в спецификации) */
#define DEFAULT_PIXEL_METERS 317.5 /* пример; замените на значение из паспорта */

/* ---- endianness ---- */
typedef enum { ENDIAN_LE = 0, ENDIAN_BE = 1 } endian_t;
static endian_t g_endian = ENDIAN_LE;

static uint8_t read_u8(FILE *f) {
    int c = fgetc(f);
    if (c == EOF) { perror("read_u8: EOF"); exit(1); }
    return (uint8_t)c;
}
static uint16_t read_u16(FILE *f) {
    uint8_t b0 = read_u8(f), b1 = read_u8(f);
    if (g_endian == ENDIAN_LE) return (uint16_t)(b0 | (b1<<8));
    else return (uint16_t)((b0<<8) | b1);
}
static uint32_t read_u32(FILE *f) {
    uint8_t b0 = read_u8(f), b1 = read_u8(f), b2 = read_u8(f), b3 = read_u8(f);
    if (g_endian == ENDIAN_LE) return (uint32_t)(b0 | (b1<<8) | (b2<<16) | (b3<<24));
    else return (uint32_t)((b0<<24) | (b1<<16) | (b2<<8) | b3);
}

/* ---- параметры запуска ---- */
typedef struct {
    const char *fn;
    uint64_t tdmo_offset;
    uint32_t tdmo_count;
    uint64_t cmr_tzbd_offset;
    uint32_t cmr_count;
    double pixel_m;
    double origin_x;
    double origin_y;
    int little_endian;
} params_t;

/* ---- usage ---- */
static void usage(const char *prog) {
    fprintf(stderr,
        "Использование: %s -f file [-t tdmo_offset] [-n tdmo_count] [-p pixel_m] [-o X0,Y0]\n"
        "          [-r tzbd_offset] [-m cmr_count] [-e le|be]\n\n"
        "Пример:\n"
        "  %s -f myfile.dci -t 0x2000 -n 4096 -p 317.5 -o 500000,0 -r 0x8000 -m 1024 -e le\n",
        prog, prog);
}

/* ---- чтение массива ТДМО ---- */
static uint32_t *read_tdmo(FILE *f, uint64_t offset, uint32_t count) {
    if (!offset || !count) return NULL;
    if (fseeko(f, (off_t)offset, SEEK_SET) != 0) { perror("fseeko tdmo"); exit(1); }
    uint32_t *arr = (uint32_t*)calloc(count, sizeof(uint32_t));
    if (!arr) { perror("calloc tdmo"); exit(1); }
    for (uint32_t i=0;i<count;i++) arr[i] = read_u32(f);
    return arr;
}

/* ---- парсинг блока ТМО ---- */
static void parse_tmo_block(FILE *f, uint64_t addr, uint32_t square_index,
                            double pixel_m, double origin_x, double origin_y,
                            uint32_t square_size_pixels) {
    if (addr == 0) return;
    if (fseeko(f, (off_t)addr, SEEK_SET) != 0) {
        fprintf(stderr,"seek to block 0x%08" PRIx64 " failed\n", (uint64_t)addr);
        return;
    }

    printf("=== Блок ТМО: addr=0x%08" PRIx64 " square_index=%u ===\n", (uint64_t)addr, square_index);

    while (1) {
        off_t pos = ftello(f);
        if (pos < 0) { perror("ftello"); exit(1); }

        uint32_t obj_id = read_u32(f);
        uint8_t obj_code = read_u8(f);
        uint8_t reserved = read_u8(f);
        uint8_t viz_flags = read_u8(f);
        uint8_t rep_count = read_u8(f);
        uint16_t next_obj_rel = read_u16(f);

        if (obj_id == 0 && obj_code == 0 && rep_count==0 && next_obj_rel==0) break;

        printf("Object ID=%" PRIu32 " code=%u reps=%u\n", obj_id, obj_code, rep_count);

        for (uint8_t r=0;r<rep_count;r++) {
            off_t rep_pos = ftello(f);
            uint8_t r_viz = read_u8(f);
            uint8_t r_loc = read_u8(f);
            uint16_t r_next_rel = read_u16(f);
            uint16_t prim_count = read_u16(f);
            printf("  Rep %u: viz=0x%02x loc=0x%02x prims=%u\n",
                   (unsigned)r, r_viz, r_loc, prim_count);

            for (uint16_t p=0;p<prim_count;p++) {
                uint8_t npoints = read_u8(f);
                uint8_t prim_loc = read_u8(f);
                if (npoints == 0) continue;
                if (npoints > 255) {
                    fprintf(stderr,"npoints too big (%u)\n", npoints);
                    exit(1);
                }
                printf("    Prim %u: npoints=%u loc=0x%02x\n", (unsigned)p, npoints, prim_loc);

                for (uint8_t pt=0; pt<npoints; pt++) {
                    uint8_t dx = read_u8(f);
                    uint8_t dy = read_u8(f);
                    double dx_pixels = ((double)dx) / 4.0;
                    double dy_pixels = ((double)dy) / 4.0;
                    uint32_t i = square_index; /* упрощённо */
                    uint32_t j = 0;
                    double pixel_x_global = (double)(square_size_pixels * i) + dx_pixels;
                    double pixel_y_global = (double)(square_size_pixels * j) + dy_pixels;
                    double X = origin_x + pixel_x_global * pixel_m;
                    double Y = origin_y + pixel_y_global * pixel_m;
                    printf("      pt%u: dx=%u dy=%u => X=%.3f Y=%.3f\n", pt, dx, dy, X, Y);
                }
            }
            if (r_next_rel != 0) {
                off_t next_pos = rep_pos + r_next_rel;
                if (fseeko(f, next_pos, SEEK_SET) != 0) { perror("fseeko rep next"); exit(1); }
            }
        }

        if (next_obj_rel == 0) break;
        else {
            off_t next_obj_pos = pos + next_obj_rel;
            if (fseeko(f, next_obj_pos, SEEK_SET) != 0) { perror("fseeko next_obj"); exit(1); }
        }
    }
    printf("=== Конец блока ТМО ===\n\n");
}

/* ---- структура ТЗБД ---- */
#pragma pack(push,1)
typedef struct {
    uint16_t max_height;
    uint8_t abs_min_neg;
    uint8_t divider_pow;
    uint32_t addr_block;
} tzbd_entry_t;
#pragma pack(pop)

/* ---- разбор ЦМР ТЗБД ---- */
static void parse_cmr_tzbd(FILE *f, uint64_t tzbd_offset, uint32_t count,
                           double pixel_m, uint32_t square_size_pixels,
                           const char *out_prefix) {
    if (!tzbd_offset || !count) return;
    if (fseeko(f, (off_t)tzbd_offset, SEEK_SET) != 0) { perror("fseeko tzbd"); exit(1); }

    char csvname[256];
    snprintf(csvname, sizeof(csvname), "%s_cmr_tzbd.csv", out_prefix ? out_prefix : "out");
    FILE *csv = fopen(csvname, "w");
    if (!csv) { perror("fopen csv"); exit(1); }
    fprintf(csv,"square_index,max_height,abs_min_neg,divider_pow,addr_block\n");

    for (uint32_t k=0;k<count;k++) {
        uint16_t maxh = read_u16(f);
        uint8_t absmin = read_u8(f);
        uint8_t dpow = read_u8(f);
        uint32_t addr = read_u32(f);
        fprintf(csv,"%u,%" PRIu16 ",%u,%u,0x%08" PRIx32 "\n", k, maxh, absmin, dpow, addr);

        if (addr != 0) {
            uint32_t cells = square_size_pixels * square_size_pixels;
            uint8_t *buf = (uint8_t*)malloc(cells);
            if (!buf) { perror("malloc cmr block"); exit(1); }
            if (fseeko(f, (off_t)addr, SEEK_SET) != 0) { perror("fseeko cmr block"); free(buf); continue; }
            fread(buf,1,cells,f);

            char blockname[256];
            snprintf(blockname, sizeof(blockname), "%s_cmr_block_%u.csv",
                     out_prefix ? out_prefix : "out", k);
            FILE *bcsv = fopen(blockname, "w");
            if (!bcsv) { perror("fopen block csv"); free(buf); continue; }
            fprintf(bcsv,"ix,iy,byte_value,height_meters\n");
            uint32_t cell_idx=0;
            for (uint32_t iy=0; iy<square_size_pixels; iy++) {
                for (uint32_t ix=0; ix<square_size_pixels; ix++) {
                    uint8_t s = buf[cell_idx++];
                    uint32_t D = 1u << dpow;
                    int H = (int)s * (int)D - (int)absmin;
                    fprintf(bcsv,"%u,%u,%u,%d\n", ix, iy, s, H);
                }
            }
            fclose(bcsv);
            free(buf);
        }
    }
    fclose(csv);
    printf("ЦМР ТЗБД экспортирован в %s (+ CSV для блоков)\n", csvname);
}

/* ---- MAIN ---- */
int main(int argc, char **argv) {
    params_t p;
    memset(&p,0,sizeof(p));
    p.pixel_m = DEFAULT_PIXEL_METERS;
    p.little_endian = 1;

    /* ручной парсер аргументов */
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i+1<argc) p.fn = argv[++i];
        else if (strcmp(argv[i], "-t") == 0 && i+1<argc) p.tdmo_offset = strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "-n") == 0 && i+1<argc) p.tdmo_count = strtoul(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "-p") == 0 && i+1<argc) p.pixel_m = atof(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i+1<argc) sscanf(argv[++i], "%lf,%lf", &p.origin_x, &p.origin_y);
        else if (strcmp(argv[i], "-r") == 0 && i+1<argc) p.cmr_tzbd_offset = strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "-m") == 0 && i+1<argc) p.cmr_count = strtoul(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "-e") == 0 && i+1<argc) {
            char *val = argv[++i];
            p.little_endian = (strcmp(val,"le")==0);
        } else {
            usage(argv[0]);
            return 1;
        }
    }
    if (!p.fn) { usage(argv[0]); return 1; }

    g_endian = p.little_endian ? ENDIAN_LE : ENDIAN_BE;

    FILE *f = fopen(p.fn, "rb");
    if (!f) { perror("fopen"); return 1; }

    printf("Файл: %s\n", p.fn);
    printf("Endian: %s\n", p.little_endian ? "little" : "big");
    printf("pixel_m=%.6f origin=(%.3f,%.3f)\n", p.pixel_m, p.origin_x, p.origin_y);

    if (p.tdmo_offset && p.tdmo_count) {
        uint32_t *tdmo = read_tdmo(f, p.tdmo_offset, p.tdmo_count);
        for (uint32_t k=0;k<p.tdmo_count;k++) {
            if (tdmo[k] != 0)
                parse_tmo_block(f, (uint64_t)tdmo[k], k,
                                p.pixel_m, p.origin_x, p.origin_y, DEFAULT_SQUARE_PIXELS);
        }
        free(tdmo);
    }

    if (p.cmr_tzbd_offset && p.cmr_count) {
        parse_cmr_tzbd(f, p.cmr_tzbd_offset, p.cmr_count,
                       p.pixel_m, DEFAULT_SQUARE_PIXELS, "dci_out");
    }

    fclose(f);
    printf("Готово.\n");
    return 0;
}
