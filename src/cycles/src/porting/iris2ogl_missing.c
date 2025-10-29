/*
 * iris2ogl_missing.c - Additional IRIS GL compatibility functions
 * Functions that were missing from the initial iris2ogl.c implementation
 */

#include "iris2ogl.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// === Font Scaling Support ===
#define MAX_SCALED_FONTS 32
static ScaledFont scaled_fonts[MAX_SCALED_FONTS];
static int scaled_font_count = 0;

// === Additional Color Functions ===
void RGBcolor(RGBvalue r, RGBvalue g, RGBvalue b) {
    glColor3ub((GLubyte)r, (GLubyte)g, (GLubyte)b);
}

// === Additional Matrix Functions ===
void rotate(Angle angle, char axis) {
    rot((float)angle / 10.0f, axis);  // IRIS GL Angle is in tenths of degrees
}

void ortho2(Coord left, Coord right, Coord bottom, Coord top) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(left, right, bottom, top);
    glMatrixMode(GL_MODELVIEW);
}

// === Additional Font Functions ===
void charstr(const char *str) {
    fmprstr(str);  // Alias to fmprstr
}

fmfonthandle fmscalefont(fmfonthandle font, float scale) {
    // Chercher si cette combinaison font/scale existe déjà
    for (int i = 0; i < scaled_font_count; i++) {
        if (scaled_fonts[i].base_font == font && 
            fabsf(scaled_fonts[i].scale - scale) < 0.01f) {
            return (fmfonthandle)&scaled_fonts[i];
        }
    }
    
    // Créer une nouvelle entrée
    if (scaled_font_count < MAX_SCALED_FONTS) {
        scaled_fonts[scaled_font_count].base_font = font;
        scaled_fonts[scaled_font_count].scale = scale;
        
        // Détecter si c'est une stroke font
        scaled_fonts[scaled_font_count].is_stroke = 
            (font == (void*)GLUT_STROKE_ROMAN || font == (void*)GLUT_STROKE_MONO_ROMAN);
        
        return (fmfonthandle)&scaled_fonts[scaled_font_count++];
    }
    
    // Si on manque de place, retourner le font original
    return font;
}

// Fonction helper pour vérifier si un font handle est un ScaledFont
int is_scaled_font(fmfonthandle font, ScaledFont** out_sf) {
    for (int i = 0; i < scaled_font_count; i++) {
        if ((void*)font == (void*)&scaled_fonts[i]) {
            if (out_sf) *out_sf = &scaled_fonts[i];
            return 1;
        }
    }
    return 0;
}

// === Additional Window Functions ===
int getwindow(void) {
    return glutGetWindow();
}

void window(Coord left, Coord right, Coord bottom, Coord top, Coord ccnear, Coord ccfar) {
    // IRIS GL window() creates a perspective viewing frustum
    // Similar to glFrustum in OpenGL
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(left, right, bottom, top, ccnear, ccfar);
    glMatrixMode(GL_MODELVIEW);
}

// === Additional Device Functions ===
// Forward declarations for mouse position accessors in iris2ogl.c
extern int iris_get_mouse_x(void);
extern int iris_get_mouse_y(void);

int32_t getvaluator(Device dev) {
    // Return the current value of a valuator device
    switch (dev) {
        case MOUSEX:
            return iris_get_mouse_x();
        case MOUSEY:
            return iris_get_mouse_y();
        default:
            return 0;
    }
}

// === Additional Geometric Functions ===
void swaptmesh(void) {
    // IRIS GL triangle mesh swapping
    // In OpenGL we just use glBegin(GL_TRIANGLE_STRIP)
    // This is a no-op for compatibility
}

// === UNIX Compatibility Functions ===

// times() implementation for Windows
#ifdef _WIN32
#include <time.h>

clock_t times(struct tms *buffer) {
    if (buffer) {
        clock_t t = clock();
        buffer->tms_utime = t;
        buffer->tms_stime = 0;
        buffer->tms_cutime = 0;
        buffer->tms_cstime = 0;
    }
    return clock();
}

// getopt implementation for Windows
char *optarg = NULL;
int optind = 1;
static int optopt = 0;
static char *nextchar = NULL;

int getopt(int argc, char * const argv[], const char *optstring) {
    if (optind >= argc || argv[optind] == NULL) {
        return -1;
    }

    if (nextchar == NULL || *nextchar == '\0') {
        if (optind >= argc) {
            return -1;
        }

        nextchar = argv[optind];

        if (nextchar == NULL || nextchar[0] != '-' || nextchar[1] == '\0') {
            return -1;
        }

        if (nextchar[1] == '-' && nextchar[2] == '\0') {
            optind++;
            return -1;
        }

        nextchar++;
    }

    optopt = *nextchar++;
    const char *optptr = strchr(optstring, optopt);

    if (optptr == NULL || optopt == ':') {
        if (*nextchar == '\0') {
            optind++;
            nextchar = NULL;
        }
        return '?';
    }

    if (optptr[1] == ':') {
        if (*nextchar != '\0') {
            optarg = nextchar;
            optind++;
            nextchar = NULL;
        } else if (optind + 1 < argc) {
            optind++;
            optarg = argv[optind];
            optind++;
            nextchar = NULL;
        } else {
            if (*nextchar == '\0') {
                optind++;
                nextchar = NULL;
            }
            return '?';
        }
    } else {
        if (*nextchar == '\0') {
            optind++;
            nextchar = NULL;
        }
    }

    return optopt;
}

#endif /* _WIN32 */
