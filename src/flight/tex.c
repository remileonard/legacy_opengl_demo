/*
 * Copyright 1989, 1990, 1991, 1992, 1993, 1994, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 * rights reserved under the Copyright Laws of the United States.
 */

/*
 *  flight/tex.c $Revision: 1.1 $
 */

#include "fcntl.h"
#include "flight.h"
#include "porting/iris2ogl.h"
#include "stdio.h"

static float texps[] = {TX_MAGFILTER, TX_BILINEAR, TX_MINFILTER, TX_MIPMAP_LINEAR, 0};
static float texps_point[] = {TX_MAGFILTER, TX_POINT, TX_MINFILTER, TX_MIPMAP_POINT, 0};
static float tevps[] = {0};

int texon = FALSE;

init_texturing() {
    long *image;
    unsigned char bw[128 * 128];
    int i;

    printf("=== INIT TEXTURING ===\n");
    
    readtex("hills.t", bw, 128 * 128);
    
    printf("First 16 bytes of texture data: ");
    for (i = 0; i < 16; i++) {
        printf("%02X ", bw[i]);
    }
    printf("\n");
    
    texdef2d(1, 1, 128, 128, (unsigned long *)bw, 5, texps_point);
    tevdef(1, 0, tevps);
    
    printf("Texture system initialized\n");
    printf("======================\n\n");
    fflush(stdout);
}

texturing(b) {
    printf("texturing(%s)\n", b ? "TRUE" : "FALSE");
    
    if (b) {
        texon = TRUE;
        texbind(0, 1);
        tevbind(0, 1);
        
        // Vérifier l'état OpenGL
        GLboolean tex_enabled;
        GLint bound_tex;
        GLint tex_env_mode;
        
        glGetBooleanv(GL_TEXTURE_2D, &tex_enabled);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound_tex);
        glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &tex_env_mode);
        
        printf("  GL_TEXTURE_2D: %s\n", tex_enabled ? "ENABLED" : "DISABLED");
        printf("  Bound texture ID: %d\n", bound_tex);
        printf("  Texture env mode: %d\n", tex_env_mode);
        GLboolean gen_s = glIsEnabled(GL_TEXTURE_GEN_S);
        GLboolean gen_t = glIsEnabled(GL_TEXTURE_GEN_T);
        printf("TEXTURE_GEN: S=%s, T=%s\n", gen_s ? "ON" : "OFF", gen_t ? "ON" : "OFF");
        
        if (gen_s) {
            GLint mode_s;
            GLfloat plane_s[4];
            glGetTexGeniv(GL_S, GL_TEXTURE_GEN_MODE, &mode_s);
            glGetTexGenfv(GL_S, GL_OBJECT_PLANE, plane_s);
            printf("  S mode: %s\n", mode_s == GL_OBJECT_LINEAR ? "OBJECT_LINEAR" : 
                                    mode_s == GL_EYE_LINEAR ? "EYE_LINEAR" : "OTHER");
            printf("  S plane: [%.4f, %.4f, %.4f, %.4f]\n", 
                plane_s[0], plane_s[1], plane_s[2], plane_s[3]);
        }
        
        if (gen_t) {
            GLint mode_t;
            GLfloat plane_t[4];
            glGetTexGeniv(GL_T, GL_TEXTURE_GEN_MODE, &mode_t);
            glGetTexGenfv(GL_T, GL_OBJECT_PLANE, plane_t);
            printf("  T mode: %s\n", mode_t == GL_OBJECT_LINEAR ? "OBJECT_LINEAR" : 
                                    mode_t == GL_EYE_LINEAR ? "EYE_LINEAR" : "OTHER");
            printf("  T plane: [%.4f, %.4f, %.4f, %.4f]\n", 
                plane_t[0], plane_t[1], plane_t[2], plane_t[3]);
        }

        debug_texture_coordinates();
    } else {
        texon = FALSE;
        texbind(0, 0);
        tevbind(0, 0);
    }
    fflush(stdout);
}

readtex(fname, buf, size) char *fname;
unsigned long *buf;
{
    long ifd;
    char file[80];

    strcpy(file, datadir);
    strcat(file, fname);

    if ((ifd = open(file, O_RDONLY)) == -1) {
        fprintf(stderr, "flight: can't open texture file %s\n", file);
        exit(1);
    }

    read(ifd, buf, size);

    close(ifd);
}
