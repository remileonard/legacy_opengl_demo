/*
 * jpeg.c - small decoder for the baseline JPEG images of Certain Impact
 * (huffman coded, 8 bit, grey or YCbCr with any sampling factors).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jpeg.h"

typedef struct {
    unsigned char bits[17];
    unsigned char vals[256];
    /* canonical decoding tables */
    int mincode[17], maxcode[18], valptr[17];
} Huff;

typedef struct {
    int id, h, v, tq, td, ta;
    int bw, bh;              /* size in blocks */
    unsigned char *data;     /* bw * 8 x bh * 8 samples */
    int dc;
} Comp;

typedef struct {
    const unsigned char *p, *end;
    unsigned int buf;
    int nbits, marker;
    unsigned short q[4][64];
    Huff hdc[4], hac[4];
    Comp c[3];
    int nc, w, h, hmax, vmax, restart;
} Jpeg;

static const unsigned char zigzag[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

static int u16(const unsigned char *p) { return (p[0] << 8) | p[1]; }

static void build_huff(Huff *h)
{
    int code = 0, k = 0, i;
    for (i = 1; i <= 16; i++) {
        h->valptr[i] = k;
        h->mincode[i] = code;
        code += h->bits[i];
        k += h->bits[i];
        h->maxcode[i] = h->bits[i] ? code - 1 : -1;
        code <<= 1;
    }
    h->maxcode[17] = 0x7fffffff;
}

static int getbit(Jpeg *j)
{
    if (j->nbits == 0) {
        int b = 0;
        if (j->p < j->end && !j->marker) {
            b = *j->p++;
            if (b == 0xff) {
                int n = j->p < j->end ? *j->p : 0;
                if (n == 0)
                    j->p++;
                else {
                    j->marker = n;
                    j->p--;
                    b = 0;
                }
            }
        }
        j->buf = (unsigned int)b;
        j->nbits = 8;
    }
    j->nbits--;
    return (j->buf >> j->nbits) & 1;
}

static int getbits(Jpeg *j, int n)
{
    int v = 0;
    while (n--)
        v = (v << 1) | getbit(j);
    return v;
}

static int decode(Jpeg *j, Huff *h)
{
    int code = 0, i;
    for (i = 1; i <= 16; i++) {
        code = (code << 1) | getbit(j);
        if (h->maxcode[i] >= 0 && code <= h->maxcode[i])
            return h->vals[h->valptr[i] + code - h->mincode[i]];
    }
    return 0;
}

static int extend(int v, int n)
{
    return n == 0 ? 0 : v < (1 << (n - 1)) ? v - (1 << n) + 1 : v;
}

/* inverse DCT of one block, straightforward separable version */
static void idct(const int *in, unsigned char *out, int stride)
{
    static float c[8][8];
    static int init;
    float tmp[64];
    int x, y, u;
    if (!init) {
        for (x = 0; x < 8; x++)
            for (u = 0; u < 8; u++)
                c[x][u] = (float)((u == 0 ? sqrt(0.125) : 0.5) * cos((2 * x + 1) * u * 3.14159265358979 / 16));
        init = 1;
    }
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++) {
            float s = 0;
            for (u = 0; u < 8; u++)
                s += c[x][u] * in[y * 8 + u];
            tmp[y * 8 + x] = s;
        }
    for (x = 0; x < 8; x++)
        for (y = 0; y < 8; y++) {
            float s = 0;
            int v;
            for (u = 0; u < 8; u++)
                s += c[y][u] * tmp[u * 8 + x];
            v = (int)floor(s + 128.5f);
            out[y * stride + x] = (unsigned char)(v < 0 ? 0 : v > 255 ? 255 : v);
        }
}

static void decode_block(Jpeg *j, Comp *c, unsigned char *out, int stride)
{
    int blk[64], k, t;
    memset(blk, 0, sizeof(blk));
    t = decode(j, &j->hdc[c->td]);
    c->dc += extend(getbits(j, t), t);
    blk[0] = c->dc * j->q[c->tq][0];
    for (k = 1; k < 64;) {
        int rs = decode(j, &j->hac[c->ta]), r = rs >> 4, s = rs & 15;
        if (s == 0) {
            if (r != 15)
                break;
            k += 16;
            continue;
        }
        k += r;
        if (k > 63)
            break;
        blk[zigzag[k]] = extend(getbits(j, s), s) * j->q[c->tq][k];
        k++;
    }
    idct(blk, out, stride);
}

static int decode_scan(Jpeg *j)
{
    int mcux = (j->w + 8 * j->hmax - 1) / (8 * j->hmax);
    int mcuy = (j->h + 8 * j->vmax - 1) / (8 * j->vmax);
    int mx, my, i, x, y, count = 0;
    for (i = 0; i < j->nc; i++) {
        Comp *c = &j->c[i];
        c->bw = mcux * c->h;
        c->bh = mcuy * c->v;
        c->data = (unsigned char *)malloc((size_t)c->bw * 8 * c->bh * 8);
        c->dc = 0;
    }
    for (my = 0; my < mcuy; my++)
        for (mx = 0; mx < mcux; mx++) {
            if (j->restart && count && count % j->restart == 0) {
                /* skip to the restart marker */
                j->nbits = 0;
                if (j->marker >= 0xd0 && j->marker <= 0xd7) {
                    j->p += 2;
                    j->marker = 0;
                } else {
                    while (j->p + 1 < j->end && !(j->p[0] == 0xff && j->p[1] >= 0xd0 && j->p[1] <= 0xd7))
                        j->p++;
                    j->p += 2;
                    j->marker = 0;
                }
                for (i = 0; i < j->nc; i++)
                    j->c[i].dc = 0;
            }
            for (i = 0; i < j->nc; i++) {
                Comp *c = &j->c[i];
                int stride = c->bw * 8;
                for (y = 0; y < c->v; y++)
                    for (x = 0; x < c->h; x++)
                        decode_block(j, c, c->data + ((my * c->v + y) * 8) * stride + (mx * c->h + x) * 8, stride);
            }
            count++;
        }
    return 0;
}

unsigned char *jpegLoad(const char *file, int *w, int *h)
{
    FILE *f = fopen(file, "rb");
    unsigned char *buf, *rgb = NULL;
    long size;
    Jpeg j;
    int i, x, y;
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (unsigned char *)malloc(size);
    size = (long)fread(buf, 1, size, f);
    fclose(f);
    memset(&j, 0, sizeof(j));
    j.p = buf;
    j.end = buf + size;
    if (size < 4 || buf[0] != 0xff || buf[1] != 0xd8)
        goto done;
    j.p += 2;
    while (j.p + 4 <= j.end) {
        int m, len;
        const unsigned char *seg;
        if (j.p[0] != 0xff) {
            j.p++;
            continue;
        }
        m = j.p[1];
        j.p += 2;
        if (m == 0xd8 || (m >= 0xd0 && m <= 0xd7) || m == 0x01 || m == 0xff)
            continue;
        if (m == 0xd9)
            break;
        len = u16(j.p);
        seg = j.p + 2;
        if (j.p + len > j.end)
            goto done;
        if (m == 0xdb) {
            const unsigned char *q = seg;
            while (q < j.p + len) {
                int pq = q[0] >> 4, tq = q[0] & 3;
                q++;
                for (i = 0; i < 64; i++) {
                    j.q[tq][i] = (unsigned short)(pq ? u16(q + 2 * i) : q[i]);
                }
                q += pq ? 128 : 64;
            }
        } else if (m == 0xc4) {
            const unsigned char *q = seg;
            while (q < j.p + len) {
                int tc = q[0] >> 4, th = q[0] & 3, n = 0;
                Huff *hf = tc ? &j.hac[th] : &j.hdc[th];
                q++;
                for (i = 1; i <= 16; i++) {
                    hf->bits[i] = q[i - 1];
                    n += q[i - 1];
                }
                q += 16;
                memcpy(hf->vals, q, n > 256 ? 256 : n);
                q += n;
                build_huff(hf);
            }
        } else if (m == 0xc0 || m == 0xc1) {
            j.h = u16(seg + 1);
            j.w = u16(seg + 3);
            j.nc = seg[5];
            if (j.nc != 1 && j.nc != 3)
                goto done;
            for (i = 0; i < j.nc; i++) {
                j.c[i].id = seg[6 + 3 * i];
                j.c[i].h = seg[7 + 3 * i] >> 4;
                j.c[i].v = seg[7 + 3 * i] & 15;
                j.c[i].tq = seg[8 + 3 * i] & 3;
                if (j.c[i].h > j.hmax) j.hmax = j.c[i].h;
                if (j.c[i].v > j.vmax) j.vmax = j.c[i].v;
            }
        } else if (m == 0xc2 || m == 0xc3 || (m >= 0xc5 && m <= 0xcf && m != 0xc8 && m != 0xcc)) {
            goto done;  /* progressive or lossless: not supported */
        } else if (m == 0xdd) {
            j.restart = u16(seg);
        } else if (m == 0xda) {
            int ns = seg[0];
            for (i = 0; i < ns; i++) {
                int id = seg[1 + 2 * i], k;
                for (k = 0; k < j.nc; k++)
                    if (j.c[k].id == id) {
                        j.c[k].td = seg[2 + 2 * i] >> 4;
                        j.c[k].ta = seg[2 + 2 * i] & 3;
                    }
            }
            j.p += len;
            decode_scan(&j);
            break;
        }
        j.p += len;
    }
    if (!j.c[0].data)
        goto done;
    rgb = (unsigned char *)malloc((size_t)j.w * j.h * 3);
    for (y = 0; y < j.h; y++)
        for (x = 0; x < j.w; x++) {
            unsigned char *o = rgb + 3 * ((size_t)y * j.w + x);
            int s[3];
            for (i = 0; i < j.nc; i++) {
                Comp *c = &j.c[i];
                int sx = x * c->h / j.hmax, sy = y * c->v / j.vmax;
                s[i] = c->data[sy * c->bw * 8 + sx];
            }
            if (j.nc == 1) {
                o[0] = o[1] = o[2] = (unsigned char)s[0];
            } else {
                float Y = (float)s[0], cb = s[1] - 128.0f, cr = s[2] - 128.0f;
                int r = (int)(Y + 1.402f * cr + 0.5f);
                int g = (int)(Y - 0.344136f * cb - 0.714136f * cr + 0.5f);
                int b = (int)(Y + 1.772f * cb + 0.5f);
                o[0] = (unsigned char)(r < 0 ? 0 : r > 255 ? 255 : r);
                o[1] = (unsigned char)(g < 0 ? 0 : g > 255 ? 255 : g);
                o[2] = (unsigned char)(b < 0 ? 0 : b > 255 ? 255 : b);
            }
        }
    *w = j.w;
    *h = j.h;
done:
    for (i = 0; i < 3; i++)
        free(j.c[i].data);
    free(buf);
    return rgb;
}
