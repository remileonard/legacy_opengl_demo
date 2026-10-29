/*
 * jpeg.h - baseline JPEG decoder for the intro screens of Certain Impact.
 */
#ifndef JPEG_H
#define JPEG_H

/* RGB pixels, top row first (free() them), NULL on error */
unsigned char *jpegLoad(const char *file, int *w, int *h);

#endif
