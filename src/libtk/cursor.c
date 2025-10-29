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

#define MAX_CURSOR 32

typedef struct _cursorRec {
    GLint id;
    int glutCursor;  // Type de curseur GLUT
} cursorRec;

int cursorNum = 0;
cursorRec cursors[MAX_CURSOR];

/******************************************************************************/

// GLUT ne supporte pas les curseurs personnalisés de la même manière que X11
// On mappe les IDs de curseurs à des curseurs prédéfinis GLUT
void tkNewCursor(GLint id, GLubyte *shapeBuf, GLubyte *maskBuf, GLenum fgColor,
		 GLenum bgColor, GLint hotX, GLint hotY)
{
    if (cursorNum >= MAX_CURSOR - 1) {
        return;
    }
    
    cursors[cursorNum].id = id;
    
    // Mapper à un curseur GLUT approprié basé sur l'ID ou d'autres critères
    // Pour un portage complet, on pourrait utiliser l'API Windows native
    // pour créer des curseurs personnalisés, mais pour la compatibilité
    // basique, on utilise les curseurs GLUT standard
    switch (id) {
        case 0:
            cursors[cursorNum].glutCursor = GLUT_CURSOR_LEFT_ARROW;
            break;
        case 1:
            cursors[cursorNum].glutCursor = GLUT_CURSOR_CROSSHAIR;
            break;
        case 2:
            cursors[cursorNum].glutCursor = GLUT_CURSOR_INFO;
            break;
        default:
            cursors[cursorNum].glutCursor = GLUT_CURSOR_INHERIT;
            break;
    }
    
    cursorNum++;
}

void tkSetCursor(GLint id)
{
    int i;
    
    // Chercher le curseur avec l'ID correspondant
    for (i = 0; i < cursorNum; i++) {
        if (cursors[i].id == id) {
            glutSetCursor(cursors[i].glutCursor);
            return;
        }
    }
    
    // Si non trouvé, utiliser le curseur par défaut
    glutSetCursor(GLUT_CURSOR_INHERIT);
}

/******************************************************************************/