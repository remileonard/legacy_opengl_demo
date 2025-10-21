#include "irisgl_compat.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Variables globales
int g_mouse_x = 0;
int g_mouse_y = 0;
int g_mouse_buttons[3] = {UP, UP, UP};

// Structure pour stocker les menus
typedef struct {
    char **items;
    int count;
    int capacity;
} PopupMenu;

static PopupMenu *menus[256] = {NULL};
static int next_menu_id = 1;

// Fonctions de compatibilité

void qdevice(short dev) {
    // Ne fait rien sous GLUT - les événements sont gérés par callbacks
    (void)dev;
}

void qread(short *val) {
    // Retourne la dernière position de la souris ou état du bouton
    static int read_index = 0;
    
    if (read_index == 0) {
        *val = g_mouse_x;
        read_index = 1;
    } else {
        *val = g_mouse_y;
        read_index = 0;
    }
}

void qenter(short dev, short val) {
    // Simule l'entrée d'un événement dans la queue
    switch(dev) {
        case MOUSEX:
            g_mouse_x = val;
            break;
        case MOUSEY:
            g_mouse_y = val;
            break;
        case LEFTMOUSE:
        case MIDDLEMOUSE:
        case RIGHTMOUSE:
            g_mouse_buttons[dev] = val;
            break;
    }
}

long newpup(void) {
    int id = next_menu_id++;
    if (id >= 256) return -1;
    
    PopupMenu *menu = (PopupMenu*)malloc(sizeof(PopupMenu));
    menu->capacity = 10;
    menu->count = 0;
    menu->items = (char**)malloc(sizeof(char*) * menu->capacity);
    
    menus[id] = menu;
    return id;
}

void addtopup(long menu_id, char *str) {
    if (menu_id <= 0 || menu_id >= 256 || !menus[menu_id]) return;
    
    PopupMenu *menu = menus[menu_id];
    
    if (menu->count >= menu->capacity) {
        menu->capacity *= 2;
        menu->items = (char**)realloc(menu->items, sizeof(char*) * menu->capacity);
    }
    
    // Extraire le texte avant %x
    char *item_text = strdup(str);
    char *percent = strstr(item_text, "%x");
    if (percent) *percent = '\0';
    
    menu->items[menu->count++] = item_text;
}

long dopup(long menu_id) {
    if (menu_id <= 0 || menu_id >= 256 || !menus[menu_id]) return -1;
    
    PopupMenu *menu = menus[menu_id];
    
    // Créer un menu GLUT
    int glut_menu = glutCreateMenu(NULL);
    
    for (int i = 0; i < menu->count; i++) {
        glutAddMenuEntry(menu->items[i], i + 1);
    }
    
    // Attacher le menu au bouton droit (temporaire)
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    // GLUT ne permet pas d'afficher un menu de manière synchrone
    // On retourne -1 pour indiquer qu'aucune sélection n'a été faite
    // Dans une vraie implémentation, il faudrait gérer cela avec des callbacks
    
    glutDestroyMenu(glut_menu);
    
    return -1;
}

void freepup(long menu_id) {
    if (menu_id <= 0 || menu_id >= 256 || !menus[menu_id]) return;
    
    PopupMenu *menu = menus[menu_id];
    
    for (int i = 0; i < menu->count; i++) {
        free(menu->items[i]);
    }
    
    free(menu->items);
    free(menu);
    menus[menu_id] = NULL;
}

// Fonctions de gestion de fenêtre

void winposition(long x1, long x2, long y1, long y2) {
    glutPositionWindow(x1, y1);
    glutReshapeWindow(x2 - x1, y2 - y1);
}

void reshapeviewport(void) {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    glViewport(0, 0, w, h);
}

void keepaspect(long x, long y) {
    // GLUT ne supporte pas directement le ratio d'aspect fixe
    // Cette fonction est ignorée
    (void)x;
    (void)y;
}

void winconstraints(void) {
    // GLUT ne supporte pas les contraintes de fenêtre
    // Cette fonction est ignorée
}

void getorigin(long *x, long *y) {
    *x = glutGet(GLUT_WINDOW_X);
    *y = glutGet(GLUT_WINDOW_Y);
}

void getsize(long *x, long *y) {
    *x = glutGet(GLUT_WINDOW_WIDTH);
    *y = glutGet(GLUT_WINDOW_HEIGHT);
}

void prefposition(long x1, long x2, long y1, long y2) {
    // Équivalent à winposition sous GLUT
    winposition(x1, x2, y1, y2);
}