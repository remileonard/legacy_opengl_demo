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
static PopupMenu * g_active_popup = NULL;
static int g_current_menu_selection = -1;

static int g_popup_x = 0;
static int g_popup_y = 0;
static int g_popup_hover = -1;
static int g_popup_result = -1;
static long g_active_menu_id = -1;  // Pour savoir quel menu est actif

#define MAX_QUEUE 100
static short event_queue[MAX_QUEUE * 2];  // dev, val, dev, val, ...
static int queue_head = 0;
static int queue_tail = 0;

int is_popup_active(void) {
    return (g_active_popup != NULL);
}
void draw_popup_menu(void) {
    if (!g_active_popup) return;
    
    PopupMenu* menu = g_active_popup;
    
    // Sauvegarder l'état OpenGL
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    gluOrtho2D(0, w, 0, h);  // Origine en bas à gauche
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    
    // Dimensions du menu
    int item_height = 25;
    int menu_width = 200;
    int menu_height = menu->count * item_height;
    
    // Inverser Y car l'origine de la souris est en haut
    int draw_y = h - g_popup_y;
    
    // Ombre portée (effet de profondeur)
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
        glVertex2i(g_popup_x + 4, draw_y - 4);
        glVertex2i(g_popup_x + menu_width + 4, draw_y - 4);
        glVertex2i(g_popup_x + menu_width + 4, draw_y - menu_height - 4);
        glVertex2i(g_popup_x + 4, draw_y - menu_height - 4);
    glEnd();
    
    // Fond du menu (blanc cassé)
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_QUADS);
        glVertex2i(g_popup_x, draw_y);
        glVertex2i(g_popup_x + menu_width, draw_y);
        glVertex2i(g_popup_x + menu_width, draw_y - menu_height);
        glVertex2i(g_popup_x, draw_y - menu_height);
    glEnd();
    
    // Bordure du menu (gris foncé)
    glColor3f(0.3f, 0.3f, 0.3f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2i(g_popup_x, draw_y);
        glVertex2i(g_popup_x + menu_width, draw_y);
        glVertex2i(g_popup_x + menu_width, draw_y - menu_height);
        glVertex2i(g_popup_x, draw_y - menu_height);
    glEnd();
    glLineWidth(1.0f);
    
    // Dessiner chaque item
    for (int i = 0; i < menu->count; i++) {
        int item_y = draw_y - i * item_height;
        
        // Highlight si survolé (bleu clair)
        if (i == g_popup_hover) {
            glColor3f(0.3f, 0.5f, 0.9f);
            glBegin(GL_QUADS);
                glVertex2i(g_popup_x + 2, item_y - 2);
                glVertex2i(g_popup_x + menu_width - 2, item_y - 2);
                glVertex2i(g_popup_x + menu_width - 2, item_y - item_height + 2);
                glVertex2i(g_popup_x + 2, item_y - item_height + 2);
            glEnd();
            
            // Texte en blanc si survolé
            glColor3f(1.0f, 1.0f, 1.0f);
        } else {
            // Texte en noir sinon
            glColor3f(0.0f, 0.0f, 0.0f);
        }
        
        // Séparateur entre items (ligne subtile)
        if (i > 0) {
            glColor3f(0.8f, 0.8f, 0.8f);
            glBegin(GL_LINES);
                glVertex2i(g_popup_x + 5, item_y);
                glVertex2i(g_popup_x + menu_width - 5, item_y);
            glEnd();
            
            // Restaurer la couleur du texte
            if (i == g_popup_hover) {
                glColor3f(1.0f, 1.0f, 1.0f);
            } else {
                glColor3f(0.0f, 0.0f, 0.0f);
            }
        }
        
        // Texte de l'item
        glRasterPos2i(g_popup_x + 10, item_y - 17);
        
        // Afficher le texte avec GLUT (fonte plus grande et plus lisible)
        void* font = GLUT_BITMAP_HELVETICA_18;
        for (char* c = menu->items[i]; *c; c++) {
            glutBitmapCharacter(font, *c);
        }
    }
    
    // Restaurer l'état OpenGL
    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glPopAttrib();
}

// Callback GLUT pour le menu
static void menu_callback(int value) {
    g_current_menu_selection = value;
}
// Fonctions de compatibilité

void qdevice(short dev) {
    // Ne fait rien sous GLUT - les événements sont gérés par callbacks
    (void)dev;
}

void qenter(short dev, short val) {
    if ((queue_tail + 2) % (MAX_QUEUE * 2) == queue_head) {
        // File pleine
        return;
    }
    
    event_queue[queue_tail] = dev;
    event_queue[queue_tail + 1] = val;
    queue_tail = (queue_tail + 2) % (MAX_QUEUE * 2);
}

int qtest(void) {
    return (queue_head != queue_tail);
}

short qread(short *val) {
    if (queue_head == queue_tail) {
        *val = 0;
        return 0;
    }
    
    short dev = event_queue[queue_head];
    *val = event_queue[queue_head + 1];
    queue_head = (queue_head + 2) % (MAX_QUEUE * 2);
    
    return dev;
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
    if (menu_id <= 0 || menu_id >= 256) return -1;
    if (!menus[menu_id]) return -1;
    
    // Activer le menu à la position de la souris
    g_active_popup = menus[menu_id];
    g_active_menu_id = menu_id;  // Sauvegarder l'ID pour éviter la libération prématurée
    g_popup_x = g_mouse_x;
    g_popup_y = g_mouse_y;
    g_popup_hover = -1;
    g_popup_result = -1;
    
    // Forcer le redessin
    glutPostRedisplay();
    
    return -1; // Le résultat sera disponible après un clic
}

void freepup(long menu_id) {
    if (menu_id <= 0 || menu_id >= 256 || !menus[menu_id]) return;
    
    // Ne pas libérer si c'est le menu actif
    if (menu_id == g_active_menu_id) {
        fprintf(stderr, "freepup: skipping active menu %ld\n", menu_id);
        return;
    }
    
    PopupMenu *menu = menus[menu_id];
    
    for (int i = 0; i < menu->count; i++) {
        free(menu->items[i]);
    }
    
    free(menu->items);
    free(menu);
    menus[menu_id] = NULL;
}

// Fonction pour fermer le menu actif
void close_popup_menu(void) {
    if (g_active_menu_id >= 0) {
        freepup(g_active_menu_id);
        g_active_menu_id = -1;
    }
    g_active_popup = NULL;
    glutPostRedisplay();
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

// Fonction pour vérifier si la souris est dans le menu
int is_mouse_in_popup(int mx, int my) {
    if (!g_active_popup) return 0;
    
    int item_height = 25;
    int menu_width = 200;
    int menu_height = g_active_popup->count * item_height;
    
    // Vérifier si la souris est dans les limites du menu
    return (mx >= g_popup_x && mx <= g_popup_x + menu_width &&
            my >= g_popup_y && my <= g_popup_y + menu_height);
}

// Fonction pour obtenir l'item survolé
int get_popup_item_at(int mx, int my) {
    if (!g_active_popup) return -1;
    
    int item_height = 25;
    int menu_width = 200;
    
    // Vérifier si dans les limites X
    if (mx < g_popup_x || mx > g_popup_x + menu_width) return -1;
    
    // Calculer l'index de l'item
    int relative_y = my - g_popup_y;
    if (relative_y < 0) return -1;
    
    int item_index = relative_y / item_height;
    
    if (item_index >= 0 && item_index < g_active_popup->count) {
        return item_index;
    }
    
    return -1;
}

// Fonction pour gérer le mouvement de la souris dans le menu
void popup_mouse_motion(int x, int y) {
    if (!g_active_popup) return;
    
    g_popup_hover = get_popup_item_at(x, y);
    glutPostRedisplay();
}

int popup_mouse_click(int button, int state, int x, int y) {
    if (!g_active_popup) return 0; // Pas de menu actif - ne pas gérer l'événement
    
    // Fermer le menu si on relâche le bouton droit
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        close_popup_menu();
        return -1; // Menu fermé
    }
    
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        if (is_mouse_in_popup(x, y)) {
            int item = get_popup_item_at(x, y);
            if (item >= 0) {
                g_popup_result = item + 1; // Les items commencent à 1
                
                // Copier le résultat et fermer le menu
                int result = g_popup_result;
                close_popup_menu();
                
                return result; // Retourner l'item sélectionné
            }
        } else {
            // Clic en dehors du menu - fermer sans sélection
            close_popup_menu();
            return -1; // Menu fermé sans sélection
        }
    }
    
    return 0; // Événement non géré par le menu
}