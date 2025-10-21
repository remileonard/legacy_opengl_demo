#ifndef IRISGL_COMPAT_H
#define IRISGL_COMPAT_H

#include <GL/glut.h>

// Définitions des constantes IRISGL
#define ESCKEY 27
#define LEFTMOUSE 0
#define MIDDLEMOUSE 1
#define RIGHTMOUSE 2
#define MOUSEX 3
#define MOUSEY 4
#define UP 1
#define DOWN 0
#define TRUE 1
#define FALSE 0

// Structure pour la queue d'événements
typedef struct {
    short dev;
    short val;
} QueueEvent;

// Variables globales pour stocker la position de la souris
extern int g_mouse_x;
extern int g_mouse_y;
extern int g_mouse_buttons[3];

// Fonctions de compatibilité IRISGL
void qdevice(short dev);
void qread(short *val);
void qenter(short dev, short val);
long newpup(void);
void addtopup(long menu, char *str);
long dopup(long menu);
void freepup(long menu);

// Fonctions de gestion de fenêtre
void winposition(long x1, long x2, long y1, long y2);
void reshapeviewport(void);
void keepaspect(long x, long y);
void winconstraints(void);
void getorigin(long *x, long *y);
void getsize(long *x, long *y);
void prefposition(long x1, long x2, long y1, long y2);

#endif // IRISGL_COMPAT_H