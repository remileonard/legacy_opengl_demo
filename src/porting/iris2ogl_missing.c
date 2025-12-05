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

// Forward declarations for spaceball state accessors (à définir dans iris2ogl.c)
extern float iris_get_spaceball_tx(void);
extern float iris_get_spaceball_ty(void);
extern float iris_get_spaceball_tz(void);
extern float iris_get_spaceball_rx(void);
extern float iris_get_spaceball_ry(void);
extern float iris_get_spaceball_rz(void);
extern int   iris_get_spaceball_period_ms(void);

int32_t getvaluator(Device dev) {
    switch (dev) {
    // Souris / curseur
    case MOUSEX:
        return (int32_t)iris_get_mouse_x();
    case MOUSEY:
        return (int32_t)iris_get_mouse_y();

    // Spaceball : on renvoie des valeurs mises à l’échelle (milliers)
    case SBTX:
        return (int32_t)(iris_get_spaceball_tx() * 1000.0f);
    case SBTY:
        return (int32_t)(iris_get_spaceball_ty() * 1000.0f);
    case SBTZ:
        return (int32_t)(iris_get_spaceball_tz() * 1000.0f);
    case SBRX:
        return (int32_t)(iris_get_spaceball_rx() * 1000.0f);
    case SBRY:
        return (int32_t)(iris_get_spaceball_ry() * 1000.0f);
    case SBRZ:
        return (int32_t)(iris_get_spaceball_rz() * 1000.0f);
    case SBPERIOD:
        return (int32_t)iris_get_spaceball_period_ms();

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

// === Rendering State Functions ===
void backface(Boolean enable) {
    // IRIS GL backface culling
    if (enable) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void blendfunction(int sfactor, int dfactor) {
    // IRIS GL blending function
    // Map IRIS GL blend factors to OpenGL blend factors
    GLenum gl_sfactor, gl_dfactor;
    
    switch (sfactor) {
        case BF_ZERO:  gl_sfactor = GL_ZERO; break;
        case BF_ONE:   gl_sfactor = GL_ONE; break;
        case BF_DC:    gl_sfactor = GL_DST_COLOR; break;
        case BF_SC:    gl_sfactor = GL_SRC_COLOR; break;
        case BF_SA:    gl_sfactor = GL_SRC_ALPHA; break;
        case BF_MSA:   gl_sfactor = GL_ONE_MINUS_SRC_ALPHA; break;
        default:       gl_sfactor = GL_ONE; break;
    }
    
    switch (dfactor) {
        case BF_ZERO:  gl_dfactor = GL_ZERO; break;
        case BF_ONE:   gl_dfactor = GL_ONE; break;
        case BF_DC:    gl_dfactor = GL_DST_COLOR; break;
        case BF_SC:    gl_dfactor = GL_SRC_COLOR; break;
        case BF_SA:    gl_dfactor = GL_SRC_ALPHA; break;
        case BF_MSA:   gl_dfactor = GL_ONE_MINUS_SRC_ALPHA; break;
        default:       gl_dfactor = GL_ZERO; break;
    }
    
    glBlendFunc(gl_sfactor, gl_dfactor);
    
    // Enable blending if factors are not (ONE, ZERO)
    if (sfactor != BF_ONE || dfactor != BF_ZERO) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}

void zwritemask(unsigned long mask) {
    // IRIS GL depth buffer write mask
    // If mask is 0, disable depth writes; otherwise enable
    glDepthMask(mask != 0 ? GL_TRUE : GL_FALSE);
}

void wmpack(unsigned long mask) {
    // IRIS GL color write mask (packed format)
    // If mask is 0, disable all color writes; otherwise enable
    if (mask == 0) {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    } else {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
}

// === Texture Coordinate Functions ===
void t2f(float texcoord[2]) {
    // IRIS GL texture coordinate (2D)
    glTexCoord2fv(texcoord);
}

// === Picking/Feedback Functions ===
static short *feedback_buffer = NULL;
static long feedback_size = 0;
static int feedback_active = FALSE;

void feedback(short *buffer, long size) {
    // IRIS GL feedback mode - simplified implementation
    // OpenGL feedback is different, so we'll use a simple stub
    feedback_buffer = buffer;
    feedback_size = size;
    feedback_active = TRUE;
    
    // In a full implementation, we'd switch to GL_FEEDBACK mode
    // For now, just track that we're in feedback mode
}

int endfeedback(short *buffer) {
    // End feedback mode and return number of values written
    // For our simplified implementation, we'll just check if anything was drawn
    // Return 0 if nothing was drawn (culled), non-zero otherwise
    
    feedback_active = FALSE;
    
    // Simple heuristic: assume geometry is visible
    // A full implementation would actually process GL feedback buffer
    return 1;  // Non-zero means geometry is visible
}

void loadname(short name) {
    // IRIS GL load name for selection
    glLoadName((GLuint)name);
}

void pushname(short name) {
    // IRIS GL push name for selection
    glPushName((GLuint)name);
}

void popname(void) {
    // IRIS GL pop name for selection
    glPopName();
}

// === Graphics Descriptor Functions ===
int getgdesc(int descriptor) {
    // IRIS GL graphics descriptor query
    switch (descriptor) {
        case GD_TEXTURE:
            // Check if texturing is supported
            return 1;  // Modern OpenGL always supports texturing
        case GD_ZBUFFER:
            // Check if depth buffer is available
            return 1;  // GLUT always creates a depth buffer
        case GD_STEREO:
            // Check if stereo is supported
            return 0;  // Not commonly supported
        case GD_BLEND:
            return 1;
        case GD_BITS_NORM_SNG_RED: {
            GLint bits = 0;
            glGetIntegerv(GL_RED_BITS, &bits);
            return bits;
        }
        case GD_BITS_NORM_SNG_GREEN: {
            GLint bits = 0;
            glGetIntegerv(GL_GREEN_BITS, &bits);
            return bits;
        }
        case GD_BITS_NORM_SNG_BLUE: {
            GLint bits = 0;
            glGetIntegerv(GL_BLUE_BITS, &bits);
            return bits;
        }
        case GD_BITS_NORM_ZBUFFER: {
            GLint bits = 0;
            glGetIntegerv(GL_DEPTH_BITS, &bits);
            return bits;         // en pratique: 16, 24 ou 32
        }
        case GD_LINESMOOTH_RGB:
            // On considère que le lissage de lignes RGB est disponible
            return 1;
        case GD_XPMAX:
            return 1024;  // Valeur arbitraire pour max X
        case GD_YPMAX:
            return 768;   // Valeur arbitraire pour max Y
        case GD_ZMIN:
            return 0;  // Valeur minimale de profondeur
        case GD_ZMAX:
            return 1;  // Valeur maximale de profondeur
        default:
            return 0;
    }
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
