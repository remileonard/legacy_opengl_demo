/*
 * image.h - readImageRGB of powerflip
 */
#ifndef PF_IMAGE_H
#define PF_IMAGE_H

/* reads an SGI image as RGBA bytes (bottom row first), NULL on error */
unsigned char *readImageRGB(const char *name, int *width, int *height);

#endif
