// Créer un fichier irix_window.h

#ifndef IRIX_WINDOW_H
#define IRIX_WINDOW_H

#include <GL/glut.h>

// Structure pour stocker les paramètres de fenêtre
typedef struct {
    int x, y;
    int width, height;
    char title[256];
    int window_id;
} IrixWindow;

extern IrixWindow irix_main_window;

// Initialisation
static inline void irix_init_window(int *argc, char **argv, const char *title) {
    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    strncpy(irix_main_window.title, title, 255);
}

// Création de fenêtre
static inline void irix_create_window(int x, int y, int width, int height) {
    irix_main_window.x = x;
    irix_main_window.y = y;
    irix_main_window.width = width;
    irix_main_window.height = height;
    
    glutInitWindowPosition(x, y);
    glutInitWindowSize(width, height);
    irix_main_window.window_id = glutCreateWindow(irix_main_window.title);
}

// Mapping des fonctions IRIX
#define winopen(title) irix_create_window(0, 0, 512, 512)
#define winposition(x, y, w, h) irix_create_window(x, y, w, h)
#define winclose(id) glutDestroyWindow(id)

#define getsize(w, h) do { \
    *(w) = glutGet(GLUT_WINDOW_WIDTH); \
    *(h) = glutGet(GLUT_WINDOW_HEIGHT); \
} while(0)

#define getorigin(x, y) do { \
    *(x) = glutGet(GLUT_WINDOW_X); \
    *(y) = glutGet(GLUT_WINDOW_Y); \
} while(0)

#endif // IRIX_WINDOW_H