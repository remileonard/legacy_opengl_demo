/*
 * SGI Image Library Compatibility Layer
 * Replacement for gl/image.h for cross-platform compatibility
 */
#ifndef IMAGE_COMPAT_H
#define IMAGE_COMPAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Image structure compatible with SGI image library */
typedef struct {
    unsigned short imagic;    /* image magic number */
    unsigned short type;      /* storage format */
    unsigned short dim;       /* number of dimensions */
    unsigned short xsize;     /* width in pixels */
    unsigned short ysize;     /* height in pixels */
    unsigned short zsize;     /* number of channels */
    unsigned long min;        /* minimum pixel value */
    unsigned long max;        /* maximum pixel value */
    char name[80];            /* image name */
    unsigned long colormap;   /* colormap ID */
    FILE *file;               /* file pointer */
    unsigned char *data;      /* image data buffer */
    unsigned short **rows;    /* row pointers */
    char *filename;           /* filename */
    int rw;                   /* read/write mode */
} IMAGE;

/* Image magic number */
#define IMAGIC 0x01DA

/* Storage format */
#define VERBATIM(n) (n)
#define RLE(n) ((n) | 0x0100)

/* Function prototypes */
/* For reading: iopen(filename, "r", 0, 0, 0, 0, 0) */
/* For writing: iopen(filename, "w", type, dim, xsize, ysize, zsize) */
IMAGE *iopen(const char *file, const char *mode, ...);
int iclose(IMAGE *image);
int getrow(IMAGE *image, unsigned short *buffer, unsigned int y, unsigned int z);
int putrow(IMAGE *image, unsigned short *buffer, unsigned int y, unsigned int z);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_COMPAT_H */
