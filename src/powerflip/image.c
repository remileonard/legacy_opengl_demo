/*
 * image.c - readImageRGB of powerflip: the SGI images (.rgb, .bw, .rgba)
 * read for the textures, verbatim or RLE, 1 to 4 channels of 8 bits.
 *
 * The image is returned as RGBA bytes, from the bottom row up as in the
 * file, as texdef2d expects them with 4 components.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"

static unsigned int be16(const unsigned char *b)
{
    return ((unsigned int)b[0] << 8) | b[1];
}

static unsigned int be32(const unsigned char *b)
{
    return ((unsigned int)b[0] << 24) | ((unsigned int)b[1] << 16) | ((unsigned int)b[2] << 8) | b[3];
}

/* expands a RLE row of n pixels, 0 when the data is bad */
static int expand_row(const unsigned char *in, long avail, unsigned char *out, int n)
{
    int x = 0;
    long k = 0;
    while (k < avail) {
        int c = in[k++], count = c & 0x7f;
        if (count == 0)
            return x == n;
        if (x + count > n)
            return 0;
        if (c & 0x80) {
            if (k + count > avail)
                return 0;
            memcpy(out + x, in + k, count);
            k += count;
        } else {
            if (k >= avail)
                return 0;
            memset(out + x, in[k++], count);
        }
        x += count;
    }
    return x == n;
}

unsigned char *readImageRGB(const char *name, int *width, int *height)
{
    FILE *f;
    unsigned char hdr[512], *file = NULL, *row = NULL, *image = NULL;
    long len;
    int rle, bpc, xs, ys, zs, x, y, z;

    if ((f = fopen(name, "rb")) == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len < 512 || (file = (unsigned char *)malloc(len)) == NULL || fread(file, 1, len, f) != (size_t)len) {
        fclose(f);
        free(file);
        return NULL;
    }
    fclose(f);
    memcpy(hdr, file, 512);
    rle = hdr[2];
    bpc = hdr[3];
    xs = (int)be16(hdr + 6);
    ys = (int)be16(hdr + 8);
    zs = (int)be16(hdr + 10);
    if (be16(hdr) != 474 || bpc != 1 || xs <= 0 || ys <= 0) {
        fprintf(stderr, "readImageRGB: %s: not a supported SGI image\n", name);
        free(file);
        return NULL;
    }
    if (zs < 1)
        zs = 1;
    image = (unsigned char *)malloc((size_t)xs * ys * 4);
    row = (unsigned char *)malloc(xs);
    for (y = 0; y < ys; y++)
        for (x = 0; x < xs; x++)
            image[4 * (y * xs + x) + 3] = 255;
    for (z = 0; z < zs && z < 4; z++) {
        for (y = 0; y < ys; y++) {
            if (rle) {
                long t = 512 + 4L * (y + (long)z * ys), off, size;
                long s = 512 + 4L * ((long)ys * zs + y + (long)z * ys);
                if (s + 4 > len)
                    goto bad;
                off = (long)be32(file + t);
                size = (long)be32(file + s);
                if (off < 0 || off + size > len || !expand_row(file + off, size, row, xs))
                    goto bad;
            } else {
                long off = 512 + ((long)z * ys + y) * xs;
                if (off + xs > len)
                    goto bad;
                memcpy(row, file + off, xs);
            }
            for (x = 0; x < xs; x++) {
                unsigned char *p = image + 4 * (y * xs + x);
                if (zs < 3) {
                    /* luminance (and alpha) */
                    if (z == 0)
                        p[0] = p[1] = p[2] = row[x];
                    else
                        p[3] = row[x];
                } else {
                    p[z] = row[x];
                }
            }
        }
    }
    free(row);
    free(file);
    *width = xs;
    *height = ys;
    return image;

bad:
    fprintf(stderr, "readImageRGB: %s: bad image data\n", name);
    free(row);
    free(file);
    free(image);
    return NULL;
}
