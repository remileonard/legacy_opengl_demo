/*
 * SGI Image Library Compatibility Layer
 * Implementation of SGI RGB image format reader/writer
 */
#include "image_compat.h"
#include <stdarg.h>

/* Swap bytes for big-endian/little-endian conversion */
static unsigned short swapshort(unsigned short val) {
    return ((val >> 8) | (val << 8));
}

static unsigned long swaplong(unsigned long val) {
    return ((val >> 24) | ((val >> 8) & 0xFF00) | 
            ((val << 8) & 0xFF0000) | (val << 24));
}

/* Read short in big-endian format */
static unsigned short readshort(FILE *fp) {
    unsigned short val;
    fread(&val, 2, 1, fp);
#ifdef _WIN32
    return swapshort(val);  /* Windows is little-endian */
#else
    return val;
#endif
}

/* Write short in big-endian format */
static void writeshort(FILE *fp, unsigned short val) {
#ifdef _WIN32
    val = swapshort(val);
#endif
    fwrite(&val, 2, 1, fp);
}

/* Read long in big-endian format */
static unsigned long readlong(FILE *fp) {
    unsigned long val;
    fread(&val, 4, 1, fp);
#ifdef _WIN32
    return swaplong(val);
#endif
    return val;
}

/* Write long in big-endian format */
static void writelong(FILE *fp, unsigned long val) {
#ifdef _WIN32
    val = swaplong(val);
#endif
    fwrite(&val, 4, 1, fp);
}

/* Open an SGI RGB image file */
IMAGE *iopen(const char *file, const char *mode, ...) {
    IMAGE *image;
    int i;
    char buf[512];
    unsigned int type = 0, dim = 0, xsize = 0, ysize = 0, zsize = 0;
    va_list args;
    
    /* For write mode, get additional parameters */
    if (mode[0] == 'w') {
        va_start(args, mode);
        type = va_arg(args, unsigned int);
        dim = va_arg(args, unsigned int);
        xsize = va_arg(args, unsigned int);
        ysize = va_arg(args, unsigned int);
        zsize = va_arg(args, unsigned int);
        va_end(args);
    }

    image = (IMAGE *)calloc(1, sizeof(IMAGE));
    if (!image) return NULL;

    if (mode[0] == 'r') {
        /* Reading mode */
        image->file = fopen(file, "rb");
        if (!image->file) {
            free(image);
            return NULL;
        }
        image->rw = 0;

        /* Read header */
        image->imagic = readshort(image->file);
        if (image->imagic != IMAGIC) {
            fclose(image->file);
            free(image);
            return NULL;
        }

        image->type = readshort(image->file);
        image->dim = readshort(image->file);
        image->xsize = readshort(image->file);
        image->ysize = readshort(image->file);
        image->zsize = readshort(image->file);
        image->min = readlong(image->file);
        image->max = readlong(image->file);
        fread(buf, 4, 1, image->file);  /* dummy */
        fread(image->name, 80, 1, image->file);
        image->colormap = readlong(image->file);

        /* Skip to offset 512 (header is 512 bytes) */
        fseek(image->file, 512, SEEK_SET);

        /* Allocate row buffers */
        image->rows = (unsigned short **)calloc(image->zsize, sizeof(unsigned short *));
        for (i = 0; i < image->zsize; i++) {
            image->rows[i] = (unsigned short *)calloc(image->xsize, sizeof(unsigned short));
        }

    } else if (mode[0] == 'w') {
        /* Writing mode */
        image->file = fopen(file, "wb");
        if (!image->file) {
            free(image);
            return NULL;
        }
        image->rw = 1;

        /* Set up header */
        image->imagic = IMAGIC;
        image->type = type;
        image->dim = dim;
        image->xsize = xsize;
        image->ysize = ysize;
        image->zsize = zsize;
        image->min = 0;
        image->max = 255;
        image->colormap = 0;
        image->name[0] = '\0';

        /* Write header */
        writeshort(image->file, image->imagic);
        writeshort(image->file, image->type);
        writeshort(image->file, image->dim);
        writeshort(image->file, image->xsize);
        writeshort(image->file, image->ysize);
        writeshort(image->file, image->zsize);
        writelong(image->file, image->min);
        writelong(image->file, image->max);
        fwrite("\0\0\0\0", 4, 1, image->file);
        fwrite(image->name, 80, 1, image->file);
        writelong(image->file, image->colormap);

        /* Pad to 512 bytes */
        memset(buf, 0, 404);
        fwrite(buf, 404, 1, image->file);

        /* Allocate row buffers */
        image->rows = (unsigned short **)calloc(image->zsize, sizeof(unsigned short *));
        for (i = 0; i < image->zsize; i++) {
            image->rows[i] = (unsigned short *)calloc(image->xsize, sizeof(unsigned short));
        }
    }

    image->filename = strdup(file);
    return image;
}

/* Close an image file */
int iclose(IMAGE *image) {
    int i;
    
    if (!image) return -1;

    if (image->file) {
        fclose(image->file);
    }

    if (image->rows) {
        for (i = 0; i < image->zsize; i++) {
            if (image->rows[i]) {
                free(image->rows[i]);
            }
        }
        free(image->rows);
    }

    if (image->data) {
        free(image->data);
    }

    if (image->filename) {
        free(image->filename);
    }

    free(image);
    return 0;
}

/* Read a row from the image */
int getrow(IMAGE *image, unsigned short *buffer, unsigned int y, unsigned int z) {
    unsigned int x;
    long offset;
    unsigned char *bytebuf;

    if (!image || !buffer || y >= image->ysize || z >= image->zsize) {
        return -1;
    }

    if (image->type & 0x0100) {
        /* RLE compressed - simplified: treat as verbatim */
        /* For full RLE support, implement RLE decompression here */
        fprintf(stderr, "Warning: RLE compression not fully supported in getrow\n");
        return -1;
    }

    /* Calculate offset for this row */
    offset = 512 + (y * image->zsize + z) * image->xsize;
    fseek(image->file, offset, SEEK_SET);

    /* Read as bytes and convert to shorts */
    bytebuf = (unsigned char *)malloc(image->xsize);
    fread(bytebuf, 1, image->xsize, image->file);
    
    for (x = 0; x < image->xsize; x++) {
        buffer[x] = bytebuf[x];
    }

    free(bytebuf);
    return 0;
}

/* Write a row to the image */
int putrow(IMAGE *image, unsigned short *buffer, unsigned int y, unsigned int z) {
    unsigned int x;
    long offset;
    unsigned char *bytebuf;

    if (!image || !buffer || y >= image->ysize || z >= image->zsize) {
        return -1;
    }

    if (image->type & 0x0100) {
        /* RLE compressed - simplified: write as verbatim */
        fprintf(stderr, "Warning: RLE compression not fully supported in putrow\n");
    }

    /* Calculate offset for this row */
    offset = 512 + (y * image->zsize + z) * image->xsize;
    fseek(image->file, offset, SEEK_SET);

    /* Convert shorts to bytes and write */
    bytebuf = (unsigned char *)malloc(image->xsize);
    for (x = 0; x < image->xsize; x++) {
        bytebuf[x] = (unsigned char)(buffer[x] & 0xFF);
    }

    fwrite(bytebuf, 1, image->xsize, image->file);
    free(bytebuf);

    return 0;
}
