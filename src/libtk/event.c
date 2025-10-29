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
#include <GL/glut.h>
#include "tk.h"
#include "private.h"

/******************************************************************************/

void (*ExposeFunc)(int, int) = 0;
void (*ReshapeFunc)(int, int) = 0;
void (*DisplayFunc)(void) = 0;
GLenum (*KeyDownFunc)(int, GLenum) = 0;
GLenum (*MouseDownFunc)(int, int, GLenum) = 0;
GLenum (*MouseUpFunc)(int, int, GLenum) = 0;
GLenum (*MouseMoveFunc)(int, int, GLenum) = 0;
void (*IdleFunc)(void) = 0;
int lastEventType = -1;
GLenum drawAllowFlag = GL_TRUE;

/******************************************************************************/

// Wrapper callbacks pour GLUT
static void glutDisplayWrapper(void) {
    if (DisplayFunc) {
        (*DisplayFunc)();
    }
}

static void glutReshapeWrapper(int width, int height) {
    w.w = width;
    w.h = height;
    if (ReshapeFunc) {
        (*ReshapeFunc)(width, height);
    }
    if (ExposeFunc) {
        (*ExposeFunc)(width, height);
    }
}

static void glutKeyboardWrapper(unsigned char key, int x, int y) {
    if (KeyDownFunc) {
        int tkKey;
        GLenum mask = 0;
        
        // Conversion des touches
        if (key >= '0' && key <= '9') tkKey = TK_0 + (key - '0');
        else if (key >= 'A' && key <= 'Z') tkKey = TK_A + (key - 'A');
        else if (key >= 'a' && key <= 'z') tkKey = TK_a + (key - 'a');
        else if (key == ' ') tkKey = TK_SPACE;
        else if (key == '\r' || key == '\n') tkKey = TK_RETURN;
        else if (key == 27) tkKey = TK_ESCAPE;
        else return;
        
        // Vérifier les modificateurs
        int modifiers = glutGetModifiers();
        if (modifiers & GLUT_ACTIVE_SHIFT) mask |= TK_SHIFT;
        if (modifiers & GLUT_ACTIVE_CTRL) mask |= TK_CONTROL;
        
        (*KeyDownFunc)(tkKey, mask);
        glutPostRedisplay();
    }
}

static void glutSpecialWrapper(int key, int x, int y) {
    if (KeyDownFunc) {
        int tkKey;
        GLenum mask = 0;
        
        switch (key) {
            case GLUT_KEY_LEFT: tkKey = TK_LEFT; break;
            case GLUT_KEY_UP: tkKey = TK_UP; break;
            case GLUT_KEY_RIGHT: tkKey = TK_RIGHT; break;
            case GLUT_KEY_DOWN: tkKey = TK_DOWN; break;
            default: return;
        }
        
        int modifiers = glutGetModifiers();
        if (modifiers & GLUT_ACTIVE_SHIFT) mask |= TK_SHIFT;
        if (modifiers & GLUT_ACTIVE_CTRL) mask |= TK_CONTROL;
        
        if ((*KeyDownFunc)(tkKey, mask)) {
            glutPostRedisplay();
        }
    }
}

static void glutMouseWrapper(int button, int state, int x, int y) {
    GLenum mask = 0;
    
    switch (button) {
        case GLUT_LEFT_BUTTON: mask = TK_LEFTBUTTON; break;
        case GLUT_MIDDLE_BUTTON: mask = TK_MIDDLEBUTTON; break;
        case GLUT_RIGHT_BUTTON: mask = TK_RIGHTBUTTON; break;
    }
    
    if (state == GLUT_DOWN && MouseDownFunc) {
        if ((*MouseDownFunc)(x, y, mask)) {
            glutPostRedisplay();
        }
    } else if (state == GLUT_UP && MouseUpFunc) {
        if ((*MouseUpFunc)(x, y, mask)) {
            glutPostRedisplay();
        }
    }
}

static void glutMotionWrapper(int x, int y) {
    if (MouseMoveFunc) {
        GLenum mask = 0;
        // GLUT ne fournit pas directement l'état des boutons pendant le mouvement
        // On suppose qu'au moins un bouton est pressé si cette callback est appelée
        if ((*MouseMoveFunc)(x, y, mask)) {
            glutPostRedisplay();
        }
    }
}

static void glutIdleWrapper(void) {
    if (IdleFunc) {
        (*IdleFunc)();
        glutPostRedisplay();
    }
}

void tkExec(void)
{
    // Enregistrer les callbacks GLUT
    glutDisplayFunc(glutDisplayWrapper);
    glutReshapeFunc(glutReshapeWrapper);
    glutKeyboardFunc(glutKeyboardWrapper);
    glutSpecialFunc(glutSpecialWrapper);
    glutMouseFunc(glutMouseWrapper);
    glutMotionFunc(glutMotionWrapper);
    glutPassiveMotionFunc(glutMotionWrapper);
    
    if (IdleFunc) {
        glutIdleFunc(glutIdleWrapper);
    }
    
    // Lancer la boucle principale GLUT
    glutMainLoop();
}

/******************************************************************************/

void tkExposeFunc(void (*Func)(int, int))
{

    ExposeFunc = Func;
}

/******************************************************************************/

void tkReshapeFunc(void (*Func)(int, int))
{

    ReshapeFunc = Func;
}

/******************************************************************************/

void tkDisplayFunc(void (*Func)(void))
{

    DisplayFunc = Func;
}

/******************************************************************************/

void tkKeyDownFunc(GLenum (*Func)(int, GLenum))
{

    KeyDownFunc = Func;
}

/******************************************************************************/

void tkMouseDownFunc(GLenum (*Func)(int, int, GLenum))
{

    MouseDownFunc = Func;
}

/******************************************************************************/

void tkMouseUpFunc(GLenum (*Func)(int, int, GLenum))
{

    MouseUpFunc = Func;
}

/******************************************************************************/

void tkMouseMoveFunc(GLenum (*Func)(int, int, GLenum))
{

    MouseMoveFunc = Func;
}

/******************************************************************************/

void tkIdleFunc(void (*Func)(void))
{

    IdleFunc = Func;
}

/******************************************************************************/
