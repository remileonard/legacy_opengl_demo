/*
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tk.h"
#include "private.h"

/******************************************************************************/

// Palette de couleurs pour le mode indexé (simulé en RGB)
#define MAX_COLORMAP_SIZE 256
static float colorPalette[MAX_COLORMAP_SIZE][3];
static float overlayPalette[MAX_COLORMAP_SIZE][3];

// Variables globales pour stocker la dernière position de la souris
static int lastMouseX = 0;
static int lastMouseY = 0;

/******************************************************************************/

// GLUT n'expose pas la taille de la palette de couleurs de la même manière que X11
// On retourne la taille de notre palette simulée
int tkGetColorMapSize(void)
{
    return MAX_COLORMAP_SIZE;
}

/******************************************************************************/

// Cette fonction doit être appelée depuis les callbacks GLUT pour mettre à jour la position
void tkUpdateMousePosition(int x, int y)
{
    lastMouseX = x;
    lastMouseY = y;
}

void tkGetMouseLoc(int *x, int *y)
{
    // GLUT ne permet pas de récupérer la position de la souris en dehors des callbacks
    // On retourne la dernière position connue stockée par les callbacks
    *x = lastMouseX;
    *y = lastMouseY;
}

/******************************************************************************/

void tkGetSystem(GLenum type, void *ptr)
{
    // Stub: GLUT ne donne pas accès direct au Display ou Window X11/Windows
    // Cette fonction n'est généralement pas utilisée dans les démos OpenGL classiques
    switch (type) {
      case TK_X_DISPLAY:
      case TK_X_WINDOW:
	ptr = NULL;
	break;
    }
}

/******************************************************************************/

// Les fonctions suivantes gèrent les palettes de couleurs en mode index
// En GLUT/OpenGL moderne, on simule une palette en la stockant en mémoire

void tkSetFogRamp(int density, int startIndex)
{
    // Créer une rampe pour le brouillard dans la palette simulée
    int fogValues = 1 << density;
    int colorValues = 1 << startIndex;
    int i, j, k, intensity;
    
    for (i = 0; i < colorValues; i++) {
        for (j = 0; j < fogValues; j++) {
            k = i * fogValues + j;
            if (k >= MAX_COLORMAP_SIZE) break;
            
            intensity = i * fogValues + j * colorValues;
            if (intensity > 255) intensity = 255;
            
            float fIntensity = (float)intensity / 255.0f;
            colorPalette[k][0] = fIntensity;
            colorPalette[k][1] = fIntensity;
            colorPalette[k][2] = fIntensity;
        }
    }
}

/******************************************************************************/

void tkSetGreyRamp(void)
{
    // Créer une rampe de gris dans la palette simulée
    int i;
    float intensity;
    int size = MAX_COLORMAP_SIZE;
    
    for (i = 0; i < size; i++) {
        intensity = (float)i / (float)(size - 1);
        colorPalette[i][0] = intensity;
        colorPalette[i][1] = intensity;
        colorPalette[i][2] = intensity;
    }
}

/******************************************************************************/

void tkSetOneColor(int index, float r, float g, float b)
{
    // Stocker la couleur dans notre palette simulée
    if (index >= 0 && index < MAX_COLORMAP_SIZE) {
        colorPalette[index][0] = r;
        colorPalette[index][1] = g;
        colorPalette[index][2] = b;
    }
    
    // Note: En mode RGB moderne avec GLUT, on ne modifie pas vraiment
    // une palette hardware. Les applications qui utilisent le mode indexé
    // devront appeler glColor3f() avec les valeurs de colorPalette[]
}

/******************************************************************************/

// Fonction helper pour récupérer une couleur de la palette
void tkGetColorRGB(int index, float *r, float *g, float *b)
{
    if (index >= 0 && index < MAX_COLORMAP_SIZE) {
        *r = colorPalette[index][0];
        *g = colorPalette[index][1];
        *b = colorPalette[index][2];
    } else {
        *r = *g = *b = 0.0f;
    }
}

/******************************************************************************/

void tkSetOverlayMap(int size, float *rgb)
{
    // Définir la palette overlay (similaire à RGBMap mais pour overlay)
    int max = (size > MAX_COLORMAP_SIZE) ? MAX_COLORMAP_SIZE : size;
    int i;
    
    for (i = 0; i < max; i++) {
        overlayPalette[i][0] = rgb[i];
        overlayPalette[i][1] = rgb[size + i];
        overlayPalette[i][2] = rgb[size*2 + i];
    }
    
    // Note: GLUT ne supporte pas les overlays de la même manière que X11
    // Cette palette est stockée mais peut ne pas être utilisée
}

/******************************************************************************/

void tkSetRGBMap(int size, float *rgb)
{
    // Définir la palette RGB à partir d'un tableau
    // rgb contient : [r0, r1, ..., rN, g0, g1, ..., gN, b0, b1, ..., bN]
    int max = (size > MAX_COLORMAP_SIZE) ? MAX_COLORMAP_SIZE : size;
    int i;
    
    for (i = 0; i < max; i++) {
        colorPalette[i][0] = rgb[i];           // Red
        colorPalette[i][1] = rgb[size + i];    // Green
        colorPalette[i][2] = rgb[size*2 + i];  // Blue
    }
}

/******************************************************************************/

GLenum tkSetWindowLevel(GLenum level)
{
    // Stub: changement de niveau de fenêtre (main/overlay)
    // GLUT ne supporte pas les overlays de la même manière que X11
    switch (level) {
      case TK_OVERLAY:
	return GL_FALSE; // Overlay non supporté
      case TK_RGB:
      case TK_INDEX:
	return GL_TRUE; // OK pour la fenêtre principale
    }
    return GL_TRUE;
}

/******************************************************************************/