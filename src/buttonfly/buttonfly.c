#include <Windows.h>
#include <GL/GL.h>
#include <GL/GLU.h>
#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "irisgl_compat.h"
#include "event.h"
#include "bartfont.h"
#include "buttonfly.h"
#include "data.h"
#include "graph.h"

#ifdef _WIN32
    #include <shellapi.h>
    #define popen _popen
    #define pclose _pclose
    
    // Définir timespec pour Windows
    /*struct timespec {
        long tv_sec;
        long tv_nsec;
    };*/
    
    #define CLOCK_MONOTONIC 0
    
    static int clock_gettime(int clk_id, struct timespec *spec) {
        static LARGE_INTEGER frequency;
        static int initialized = 0;
        LARGE_INTEGER count;
        
        if (!initialized) {
            QueryPerformanceFrequency(&frequency);
            initialized = 1;
        }
        
        QueryPerformanceCounter(&count);
        
        spec->tv_sec = (long)(count.QuadPart / frequency.QuadPart);
        spec->tv_nsec = (long)(((count.QuadPart % frequency.QuadPart) * 1000000000LL) / frequency.QuadPart);
        
        return 0;
    }
#else
    #include <time.h>
#endif
// ...existing code...

static int esc_pressed = 0;
static struct timespec esc_press_time;
static button_struct *hovered_button = NULL;  // Bouton actuellement survolé

static double diff_timespecs(struct timespec *t1, struct timespec *t2) {
    return (t1->tv_sec - t2->tv_sec) + (t1->tv_nsec - t2->tv_nsec)/1000000000.0;
}

// Stop at the slow frame in the animation so we can analyze it.
#define BLOCK_AT_SLOW_FRAME 0

#define X 0
#define Y 1
#define Z 2

#define M_PI 3.14159265358979323846
#define XMAXSCREEN 1024
#define YMAXSCREEN 768

short dev,val;
long originx, originy, sizex, sizey;
long s_originx, s_originy, s_sizex, s_sizey;

int flyinflag = 0;
int flyoutflag = 0;
int selectflag = 0;
int exitflag = 0;
// Ajouter en haut du fichier
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000 / TARGET_FPS)


void draw_buttons(button_struct *buttons);
void draw_button(button_struct * button);
void push_button(button_struct *selected);
button_struct *load_buttons(), *which_button();
extern button_struct *new_button(char *);

button_struct *current_buttons=NULL, *selected=NULL, *secret_buttons=NULL;

path_struct *path=NULL;

button_struct *rootbutton;

float *curbackcolor;	/* current background color */

/* matrix for projecting next set of buttons
   on the back of the selected button */

float tv[4][4] = {

    {SCREEN, 0.0, 0.0, 0.0},
    {0.0, SCREEN, 0.0, 0.0},
    {0.0, 0.0, SCREEN, -1.0},
    {0.0, 0.0, 0.0, 0.0},
};

void bf_redraw(), bf_exit(), bf_selecting(), bf_deselect();
void bf_quick(), bf_fly(), do_popup(), toggle_window();
void bf_escdown(), bf_escup();
void flyindraw(), flyoutdraw(), selectdraw();
void parse_args(), doclear();

// Fonction idle pour gérer les animations
void idle_func(void) {
    // Traiter les événements en file d'attente
    static short dev, val;
    
    // Simuler le traitement des événements IRIS GL
    while (qtest()) {
        dev = qread(&val);
        
        switch (dev) {
        case RIGHTMOUSE:
            if (val == DOWN) {
                do_popup();
            }
            break;
        case LEFTMOUSE:
            if (val == DOWN) {
                bf_fly();
            }
            break;
        case MIDDLEMOUSE:
            if (val == DOWN) {
                bf_deselect();
            }
            break;
        case ESCKEY:
            if (val == DOWN) {
                bf_escdown();
            } else {
                bf_escup();
            }
            break;
        }
    }
    
    // Gérer les animations
    if (flyinflag) {
        flyindraw();
    } else if (flyoutflag) {
        flyoutdraw();
    } else if (selectflag) {
        selectdraw();
    }
}
// Remplacer idle_func par cette fonction timer
void timer_func(int value) {
    // Traiter les événements en file d'attente
    static short dev, val;
    
    while (qtest()) {
        dev = qread(&val);
        
        switch (dev) {
        case RIGHTMOUSE:
            if (val == DOWN) {
                do_popup();
            }
            break;
        case LEFTMOUSE:
            if (val == DOWN) {
                bf_fly();
            }
            break;
        case MIDDLEMOUSE:
            if (val == DOWN) {
                bf_deselect();
            }
            break;
        case ESCKEY:
            if (val == DOWN) {
                bf_escdown();
            } else {
                bf_escup();
            }
            break;
        }
    }
    
    // Gérer les animations
    if (flyinflag || flyoutflag || selectflag) {
        if (flyinflag) {
            flyindraw();
        } else if (flyoutflag) {
            flyoutdraw();
        } else if (selectflag) {
            selectdraw();
        }
    }
    
    // Rappeler le timer
    glutTimerFunc(FRAME_TIME_MS, timer_func, 0);
}
void keyboard_handler(unsigned char key, int x, int y) {
    if (key == 27) { // ESC
        bf_escdown();
    }
}

void keyboard_up_handler(unsigned char key, int x, int y) {
    if (key == 27) { // ESC
        bf_escup();
    }
}

static int g_menu_result = -1;

void mouse_motion(int x, int y) {
    g_mouse_x = x;
    g_mouse_y = y;
    
    // Gérer le survol du menu popup
    popup_mouse_motion(x, y);
    
    // Activer le mode sélection au premier mouvement de souris
    if (!selectflag && !flyinflag && !flyoutflag && !is_popup_active()) {
        qenter(MOUSEX, x);
        qenter(MOUSEY, y);
        bf_selecting();
    }
    
    // Mettre à jour le bouton survolé en mode sélection
    if (selectflag) {
        button_struct *new_hovered = which_button(x, y);
        if (new_hovered != hovered_button) {
            hovered_button = new_hovered;
            glutPostRedisplay();  // Redessiner pour afficher le changement
        }
    }
}

void mouse_click(int button, int state, int x, int y) {
    // Stocker les coordonnées globales
    g_mouse_x = x;
    g_mouse_y = y;
    
    // D'abord, vérifier si on clique dans un menu popup actif
    if (is_popup_active()) {
        int menu_result = popup_mouse_click(button, state, x, y);
        if (menu_result > 0) {
            // Un item du menu a été sélectionné
            if (selected) {
                popup_struct *scan = selected->popup;
                
                // Si c'est "Do It" (item 1), simuler un clic gauche
                if (menu_result == 1 && selected != rootbutton) {
                    qenter(LEFTMOUSE, UP);
                    qenter(MOUSEX, x);
                    qenter(MOUSEY, y);
                } 
                // Sinon, parcourir les items du popup
                else if (menu_result > 1) {
                    // Aller à l'item correspondant (menu_result - 2 car "Do It" est à 1)
                    for (int i = 0; scan && i < (menu_result - 2); i++) {
                        scan = scan->next;
                    }
                    
                    if (scan && scan->action) {
                        printf("Executing: %s\n", scan->action);
#ifdef _WIN32
                        system(scan->action);
#else
                        system(scan->action);
#endif
                    }
                }
            }
            return; // Menu géré, sortir
        } else if (menu_result == -1) {
            // Menu fermé sans sélection
            return;
        }
        // Si menu_result == 0, continuer le traitement normal
    }
    
    // Logique normale des boutons de souris
    switch (button) {
    case GLUT_RIGHT_BUTTON:
        if (state == GLUT_DOWN) {
            g_mouse_buttons[RIGHTMOUSE] = DOWN;
            qenter(RIGHTMOUSE, DOWN);
            qenter(MOUSEX, x);
            qenter(MOUSEY, y);
        } else {
            g_mouse_buttons[RIGHTMOUSE] = UP;
            qenter(RIGHTMOUSE, UP);
        }
        break;
    case GLUT_MIDDLE_BUTTON:
        if (state == GLUT_DOWN) {
            g_mouse_buttons[MIDDLEMOUSE] = DOWN;
            qenter(MIDDLEMOUSE, DOWN);
        } else {
            g_mouse_buttons[MIDDLEMOUSE] = UP;
            qenter(MIDDLEMOUSE, UP);
        }
        break;
    case GLUT_LEFT_BUTTON:
        if (state == GLUT_DOWN) {
            g_mouse_buttons[LEFTMOUSE] = DOWN;
            qenter(LEFTMOUSE, DOWN);
            qenter(MOUSEX, x);
            qenter(MOUSEY, y);
        } else {
            g_mouse_buttons[LEFTMOUSE] = UP;
            qenter(LEFTMOUSE, UP);
        }
        break;
    }
}

int main (int argc, char *argv[]) {
    static int windows;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(XMAXSCREEN, YMAXSCREEN);

    rootbutton = new_button("");
    selected = rootbutton;
    parse_args(argc, argv);
    selected = NULL;

    windows = glutCreateWindow("ButtonFly");

    // Configuration OpenGL
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    
    // Améliorer la précision du depth test
    glDepthFunc(GL_LEQUAL);
    glClearDepth(1.0);

    // Configuration de l'éclairage
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
    GLfloat light_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    
    // CORRECTION : Activer l'éclairage sur les deux faces
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);  // BLANC pour le texte
    // Enregistrer les callbacks GLUT
    glutDisplayFunc(bf_redraw);
    glutMouseFunc(mouse_click);
    glutMotionFunc(mouse_motion);        // Mouvement avec bouton pressé
    glutPassiveMotionFunc(mouse_motion);  // Mouvement sans bouton
    glutTimerFunc(FRAME_TIME_MS, timer_func, 0);
    glutKeyboardFunc(keyboard_handler);
    glutKeyboardUpFunc(keyboard_up_handler);

    // Configuration de la projection avec near/far optimisés
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)XMAXSCREEN / YMAXSCREEN, 0.1, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -5.0/4.0);

    curbackcolor = rootbutton->backcolor;
    doclear();

    draw_buttons(current_buttons);
    glutSwapBuffers();

    glutMainLoop();
    return 0;
}

button_struct *load_buttons_from_file(char *program_name, char *filename)
{
    button_struct *buttons;
    FILE *fp;

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "%s: can't open file %s\n",
                program_name, filename);
        exit(1);
    }
    buttons = load_buttons(fp);
    fclose(fp);

    return buttons;
}

void parse_args(int argc, char ** argv) {
    if (argc>3) {
        fprintf(stderr, "usage: %s [infile] [secretfile]\n", argv[0]);
        exit(1);
    } else if (argc == 3) {
        current_buttons = load_buttons_from_file(argv[0], argv[1]);
        secret_buttons = load_buttons_from_file(argv[0], argv[2]);
    } else if (argc == 2) {
        current_buttons = load_buttons_from_file(argv[0], argv[1]);
    } else {
        current_buttons = load_buttons_from_file(argv[0], "menu");
    }
}


void bf_exit()
{
	exitflag = TRUE;
}


void  bf_selecting()
{
    short mx, my;
    qread(&mx); qread(&my);	/* Yank off queue */
    selectflag = TRUE;
    flyinflag = flyoutflag = FALSE;
}

void bf_quick()
{
    short mx, my;
    qread(&mx); qread(&my);	/* Yank off queue */

    selectflag = flyinflag = flyoutflag = FALSE;
    hovered_button = NULL;  // Réinitialiser le bouton survolé
    selected = which_button(mx, my);
    if (selected) {
        push_button(selected);
        if (selected->forward) {
            add_button_to_path(current_buttons, selected);
            current_buttons = selected->forward;
        }
        selected = NULL;
    }
    else if (path) {
        path_struct *step;
        selected = path->button;
        draw_selected_button(selected, 1.0);
        selected = NULL;
        current_buttons = path->current_buttons;
        step = path;
        path = path->back;
        free(step);
        if (path) curbackcolor=path->button->backcolor;
        else curbackcolor=rootbutton->backcolor;
    }
    bf_redraw();
}

void bf_fly()
{
    short mx, my;
    qread(&mx); qread(&my);	/* Yank off queue */

    selectflag = flyinflag = flyoutflag = FALSE;
    hovered_button = NULL;  // Réinitialiser le bouton survolé
    selected = which_button(mx, my);
    if (selected) {
        push_button(selected);
        if (selected->forward) {
            add_button_to_path(current_buttons, selected);
            flyinflag = TRUE;
        }
        else {
            selected = NULL;
            bf_redraw();
        }
    }
    else if (path) {
        path_struct *step;
        flyoutflag = TRUE;
        selected = path->button;
        current_buttons = path->current_buttons;
        step = path;
        path = path->back;
        free(step);
        if (path) curbackcolor=path->button->backcolor;
        else curbackcolor=rootbutton->backcolor;
    }
}


void  do_popup()
{
    short mx, my;
    button_struct *b;
    void do_buttons_menu();
    
    qread(&mx); qread(&my);
    b = which_button(mx, my);
    if (b)
    {
        do_buttons_menu(b, mx, my);
    }
    else if (path)
    {
        do_buttons_menu(path->button, mx, my);
    }
    else if (rootbutton->popup)
    {
        do_buttons_menu(rootbutton, mx, my);
    }
}

void bf_escdown()
{
    // Keep track of when Esc is pressed so we can see how long it was pressed.
    if (!esc_pressed) {
        esc_pressed = 1;
        clock_gettime(CLOCK_MONOTONIC, &esc_press_time);
    }
}

void bf_escup()
{
    // See how long Esc was pressed.
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double elapsed = diff_timespecs(&now, &esc_press_time);
    if (elapsed > 1 && secret_buttons != NULL) {
        // Show secret menu.
        selectflag = flyinflag = flyoutflag = FALSE;
        add_button_to_path(current_buttons, secret_buttons);
        current_buttons = secret_buttons;
        bf_redraw();
    } else {
        bf_deselect();
    }
    esc_pressed = 0;
}
void bf_deselect()
{
    if (path) {
	path_struct *step;
	flyoutflag = TRUE;
	selected = path->button;
	current_buttons = path->current_buttons;
	step = path;
	path = path->back;
	free(step);
	if (path) curbackcolor=path->button->backcolor;
	else curbackcolor=rootbutton->backcolor;
    }
}

#define BIG	0
#define LITTLE	1
void  toggle_window()
{
    static int size=LITTLE;

    if (size==BIG) {
        size = LITTLE;
        winposition(s_originx, s_originx+s_sizex,
            s_originy, s_originy+s_sizey);
        reshapeviewport();
        keepaspect(XMAXSCREEN, YMAXSCREEN);
        winconstraints();
        getorigin(&originx, &originy);
        getsize(&sizex, &sizey);
    } else {
        size = BIG;
        getorigin(&s_originx, &s_originy);
        getsize(&s_sizex, &s_sizey);
        winposition(0, XMAXSCREEN, 0, YMAXSCREEN);
        reshapeviewport();
        prefposition(0, XMAXSCREEN, 0, YMAXSCREEN);
        winconstraints();
        originx=0; originy=0;
        sizex=XMAXSCREEN; sizey=YMAXSCREEN;
    }
}

void do_buttons_menu(b, mx, my)
button_struct *b;
short mx, my;
{
    long menu;
    int i, num;
    char t[128];
    popup_struct *scan;
    
    menu = newpup();

    if (b != rootbutton) {
        sprintf(t, "Do It");
            addtopup(menu, t);
    }

    for (num=0, scan=b->popup; scan != NULL; num++, scan=scan->next) {

        sprintf(t, "%s%%x%d", scan->title, num+2);
        addtopup(menu, t);
    }

    i = dopup(menu);
    freepup(menu);

    if (i == 1) {	/* Execute button */

        qenter(LEFTMOUSE, UP);
        qenter(MOUSEX, mx);
        qenter(MOUSEY, my);

    } else if ((i > 1) && (i <= num+1)) {

        for (num=0, scan=b->popup; num != (i-2);
             num++, scan=scan->next)
        ;	/* Keep on scanning... */
#ifdef _WIN32
        system(scan->action);
#else
        system(scan->action);
#endif
    }
}


void selectdraw()
{
    // Ne pas redessiner en permanence en mode sélection
    if (!selectflag) return;
    
    doclear();
    
    // Dessiner tous les boutons sauf celui survolé
    if (current_buttons) {
        button_struct *scan = current_buttons;
        while (scan) {
            if (scan != hovered_button) {
                draw_button(scan);
            }
            scan = scan->next;
        }
    }
    
    // Dessiner le bouton survolé avec l'effet de sélection
    if (hovered_button) {
        draw_selected_button(hovered_button, 1.0);
    }
    
    glutSwapBuffers();
}
void flyindraw()
{
    static float t = 1.0;
    
    if (!flyinflag) {
        t = 1.0;
        return;
    }
    
    if (t > 0.2 || !BLOCK_AT_SLOW_FRAME) {
        t -= 0.01;  // Changé de 0.02 à 0.01 pour ralentir 2x
    }
    if (t<=0.0) {
        current_buttons = selected->forward;
        selected=NULL;
        flyinflag = 0;
        t=1.0;
        curbackcolor = path->button->backcolor;
        doclear();
        draw_buttons(current_buttons);
    } else {
        doclear();
        draw_selected_button(selected, t);
    }
    glutSwapBuffers();
}

void flyoutdraw()
{
    static float t = 0.0;

    if (!flyoutflag) {
        t = 0.0;
        return;
    }

    doclear();

    t += 0.01;  // Changé de 0.02 à 0.01 pour ralentir 2x
    if (t>=1.0) {
        t = 0.0;
        selected = NULL;
        flyoutflag = 0;
        draw_buttons(current_buttons);
    } else {
        draw_buttons(current_buttons);
        draw_selected_button(selected, t);
    }
    glutSwapBuffers();
}

void bf_redraw()
{
    originx = 0;
    originy = 0;
    sizex = glutGet(GLUT_WINDOW_WIDTH);
    sizey = glutGet(GLUT_WINDOW_HEIGHT);
    
    glViewport(0, 0, sizex, sizey);
    
    doclear();
    draw_buttons(current_buttons);
    draw_popup_menu();
    glutSwapBuffers();
    // Ne pas appeler glutPostRedisplay() ici pour éviter les boucles infinies
}


/*
 *	This is called to do whatever action is required when a button is
 * pushed.  It just mucks with the button given to it.
 */
void push_button(button_struct *selected)
{
    int needpipe;
    FILE *fp;

    /* Need to open a pipe if submenu == "-" */
    if ((selected->submenu != NULL) && 
        (strcmp(selected->submenu, "-") == 0))
            needpipe = 1;
    else needpipe = 0;

    /* First, figure out where we'll build submenus from */
    fp = NULL;
    if ((selected->action != NULL) && needpipe)
    {
#ifdef _WIN32
        // Construire une commande cmd.exe pour Windows
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "cmd.exe /c %s", selected->action);
        fp = _popen(cmd, "r");
#else
        fp = popen(selected->action, "r");
#endif
        if (fp == NULL) {
            fprintf(stderr, "ButtonFly: Failed to execute command: %s\n", selected->action);
        }
    }
    else if (selected->submenu != NULL)
    {
        fp = fopen(selected->submenu, "r");
        if (fp == NULL) {
            fprintf(stderr, "ButtonFly: Failed to open file: %s\n", selected->submenu);
        }
    }
    
    /* Now, do action */
    if ((selected->action != NULL) && !needpipe)
    {
#ifdef _WIN32
        // Méthode 1 : Utiliser system() simple
        int result = system(selected->action);
        if (result != 0 && result != -1) {
            fprintf(stderr, "ButtonFly: Command failed with code %d: %s\n", result, selected->action);
        }
        
        /* Méthode 2 (alternative) : Utiliser ShellExecute pour ouvrir des fichiers/URLs
        HINSTANCE result = ShellExecuteA(NULL, "open", selected->action, NULL, NULL, SW_SHOWNORMAL);
        if ((INT_PTR)result <= 32) {
            fprintf(stderr, "ButtonFly: ShellExecute failed for: %s\n", selected->action);
        }
        */
#else
        system(selected->action);
#endif
    }
    
    /* Ok, now build submenus if we can */
    if (fp != NULL)
    {
        selected->forward = load_buttons(fp);
        if (needpipe) {
#ifdef _WIN32
            _pclose(fp);
#else
            pclose(fp);
#endif
        }
        else {
            fclose(fp);
        }
    }
}
void draw_buttons(button_struct *buttons)
{
    if (buttons) {
        button_struct *scan = buttons;
        while (scan) {
            if (scan != selected) {
                draw_button(scan);
            }
            scan = scan->next;
        }
    }
}
draw_selected_button(button, t)
button_struct *button;
float t;
{
    float gls, glc;
    GLfloat ax, ay, az;
    float tx, ty, tz;
    int bc[3], i;

    glPushMatrix();

    ax = (GLfloat)((float)button->ax*t);
    ay = (GLfloat)((float)button->ay*t);
    az = (GLfloat)((float)button->az*t);

    tx = t*button->tx;
    ty = t*button->ty;
    tz = t*button->tz;

    glTranslatef(tx, ty, tz);

    glRotatef(.1*ax, 1.0f, 0.0f, 0.0f);
    glRotatef(.1*ay, 0.0f, 1.0f, 0.0f);
    glRotatef(.1*az, 0.0f, 0.0f, 1.0f);

    // CORRECTION : Utiliser la vraie couleur du bouton
    GLfloat mat_diffuse[4];
    mat_diffuse[0] = button->highcolor[0];
    mat_diffuse[1] = button->highcolor[1];
    mat_diffuse[2] = button->highcolor[2];
    mat_diffuse[3] = 1.0f;
    
    GLfloat mat_ambient[4];
    mat_ambient[0] = button->highcolor[0] * 0.3f;
    mat_ambient[1] = button->highcolor[1] * 0.3f;
    mat_ambient[2] = button->highcolor[2] * 0.3f;
    mat_ambient[3] = 1.0f;
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
        
        
    draw_edge();

    gls = sinf(ax * M_PI / 1800.0f - M_PI/2.0f);
    glc = cosf(ax * M_PI / 1800.0f - M_PI/2.0f);

    if (gls<glc*ty/(-tz+SCREEN+THICK)) {
        glPushMatrix();

        for (i=0; i<3; i++)
            bc[i] = (int)(t*255.0 + (1.0-t)*selected->backcolor[i]*255.0);

        // CORRECTION: Désactiver complètement le polygon offset pour le fond
        glDisable(GL_POLYGON_OFFSET_FILL);
        
        // Normaliser bc[] pour OpenGL (0-1 au lieu de 0-255)
        GLfloat mat_diffuse_back[4];
        mat_diffuse_back[0] = 1.0f;
        mat_diffuse_back[1] = 1.0f;
        mat_diffuse_back[2] = 1.0f;
        mat_diffuse_back[3] = 1.0f;
        
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_back);
        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_diffuse_back);
        // Dessiner le fond du bouton LÉGÈREMENT EN ARRIÈRE
        glBegin(GL_POLYGON);
            glNormal3f(0.0f, 0.0f, -1.0f);
            glVertex3f(back_polys[0][0][0], back_polys[0][0][1], back_polys[0][0][2]-0.01f);
            glVertex3f(back_polys[0][1][0], back_polys[0][1][1], back_polys[0][1][2]-0.01f);
            glVertex3f(back_polys[0][2][0], back_polys[0][2][1], back_polys[0][2][2]-0.01f);
            glVertex3f(back_polys[0][3][0], back_polys[0][3][1], back_polys[0][3][2]-0.01f);
        glEnd();

        // Transformations pour positionner les boutons
        glTranslatef(0.0, 0.0, THICK);
        glTranslatef(0.0, 0.0, SCREEN);
        glMultMatrixf((GLfloat*)tv);
        glTranslatef(0.0, 0.0, -SCREEN-THICK);

        // Dessiner les boutons en arrière-plan
        glPushAttrib(GL_LIGHTING_BIT);
        GLfloat light_position[] = { 0.0, 0.0, 1.0, 0.0 };  // Lumière directionnelle vers l'avant
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        
        draw_buttons(button->forward);
        glPopAttrib();
        glPopMatrix();

    } else {
        draw_front(button);
    }
    
    glPopMatrix();
}


draw_edge() {
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-0.5f, -0.5f);

    for (int i=0; i<8; i++) {
        glNormal3fv(edge_normals[i]);
        glBegin(GL_POLYGON);
            glVertex3fv(edge_polys[i][0]);
            glVertex3fv(edge_polys[i][1]);
            glVertex3fv(edge_polys[i][2]);
            glVertex3fv(edge_polys[i][3]);
        glEnd();
    }
    
    glDisable(GL_POLYGON_OFFSET_FILL);
}

draw_front(button)
button_struct *button;
{
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-0.25f, -0.25f);
    glNormal3f(0.0, 0.0, -1.0);
    glBegin(GL_POLYGON);
        glVertex3fv(front_polys[0][0]);
        glVertex3fv(front_polys[0][1]);
        glVertex3fv(front_polys[0][2]);
        glVertex3fv(front_polys[0][3]);
    glEnd();

    glDisable(GL_POLYGON_OFFSET_FILL);

    // Translater pour dessiner le texte
    glTranslatef(0.0, 0.0, -THICK - 0.005f);

    draw_button_label(button);
}
void draw_button(button_struct * button)
{
    glPushMatrix();

    // IMPORTANT : Chaque bouton définit son propre matériau
    
    glTranslatef(button->tx, button->ty, button->tz);

    glRotatef(.1*button->ax, 1.0f, 0.0f, 0.0f);
    glRotatef(.1*button->ay, 0.0f, 1.0f, 0.0f);
    glRotatef(.1*button->az, 0.0f, 0.0f, 1.0f);

    GLfloat mat_diffuse[] = {button->color[0], button->color[1], button->color[2], 1.0f};
    GLfloat mat_ambient[] = {button->color[0] * 0.3f, button->color[1] * 0.3f, button->color[2] * 0.3f, 1.0f};
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

    if (flyinflag) {
        draw_front(button);
    } else if (flyoutflag) {
        draw_front(button);
    } else {
        draw_edge();
        draw_front(button);
    }
    
    

    glPopMatrix();
}

draw_highlighted_button(button)
button_struct *button;
{
    if (button) {
        GLfloat mat_diffuse[] = {button->highcolor[0], button->highcolor[1], button->highcolor[2], 1.0f};
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_diffuse);

        glPushMatrix();

        glTranslatef(button->tx, button->ty, button->tz);

        glRotatef(.1*button->ax, 1.0f, 0.0f, 0.0f);
        glRotatef(.1*button->az, 0.0f, 0.0f, 1.0f);

        draw_edge();
        draw_front(button);

        glPopMatrix();
    }
}

button_struct *load_buttons(fp)
FILE *fp;
{
	button_struct *scan;
	int nb, i;
	extern FILE *lex_fp;
	extern button_struct *buttons_input;

	lex_fp = fp;
	yyparse();

	nb = 0;	/* Figure out how many buttons were made */
	for (scan = buttons_input; scan != NULL; scan = scan->next)
		++nb;

	if (nb > MAX_SPOTS)
	{
		fprintf(stderr,
		    "Buttonfly Warning: %d is too many buttons\n", nb);
		fprintf(stderr, "(Maximum number is %d)\n", MAX_SPOTS);
		return NULL;
	}

	i = 0;	/* And now figure out where to put them */
	for (scan = buttons_input; scan != NULL; scan = scan->next)
	{
		scan->tx = spots[nb-1][i+0];
		scan->ty = spots[nb-1][i+1];
		scan->tz = spots[nb-1][i+2];
		i += 3;
		scan->ax = (int)(random(1.0)+0.5)*3600-1800;
		scan->ay = 0;
		scan->az = (int)(random(1.0)+0.5)*3600-1800;
	}
	return buttons_input;
}

stroke(str)
char *str;
{
    register int i, j;
    int x = 0, y = 0;  // Position courante
    int in_stroke = 0;

    while (*str) {
        unsigned char c = (unsigned char)*str;
        
        if (chrtbl[c][0][0]) {
            i = 0;

            while ((j = chrtbl[c][i][0]) != 0) {
                int dx = chrtbl[c][i][1];
                int dy = chrtbl[c][i][2];

                switch (j) {
                    case 3:  // MOVE - déplacer sans tracer
                        if (in_stroke) {
                            glEnd();
                            in_stroke = 0;
                        }
                        x += dx;
                        y += dy;
                        break;

                    case 2:  // DRAW - tracer une ligne
                        if (!in_stroke) {
                            glBegin(GL_LINE_STRIP);
                            glVertex2i(x, y);  // Point de départ
                            in_stroke = 1;
                        }
                        x += dx;
                        y += dy;
                        glVertex2i(x, y);  // Point d'arrivée
                        break;
                }
                i++;
            }
            
            // Terminer la ligne pour ce caractère
            if (in_stroke) {
                glEnd();
                in_stroke = 0;
            }
            
            // Avancer au caractère suivant (largeur fixe de 6 unités)
            x += 6;
            y = 0;
        } else {
            // Caractère non défini, avancer quand même
            x += 6;
        }
        
        str++;
    }
    
    // S'assurer que la dernière ligne est terminée
    if (in_stroke) {
        glEnd();
    }
}
draw_button_label(button)
button_struct *button;
{
    // Sauvegarder l'état actuel
    glPushMatrix();
    
    // Désactiver l'éclairage pour le texte
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);  // Important : désactiver le test de profondeur pour le texte
    
    // Couleur blanche pour le texte
    glColor3f(1.0f, 1.0f, 1.0f);

    // Mise à l'échelle pour le texte (inverser X pour la lisibilité)
    glScalef(-0.01f, 0.01f, 0.01f);

    // Épaisseur de ligne
    glLineWidth(2.0f);

    switch (button->wc) {

        case 1:
            glPushMatrix();
            // Augmenter le décalage pour centrer plus à gauche
            glTranslatef(-5.5f * (float)strlen(button->name[0]), -4.0f, 0.0f);
            stroke(button->name[0]);
            glPopMatrix();
            break;

        case 2:
            glPushMatrix();
            glTranslatef(-5.5f * (float)strlen(button->name[0]), 1.0f, 0.0f);
            stroke(button->name[0]);
            glPopMatrix();
            
            glPushMatrix();
            glTranslatef(-5.5f * (float)strlen(button->name[1]), -9.0f, 0.0f);
            stroke(button->name[1]);
            glPopMatrix();
            break;

        case 3:
            glPushMatrix();
            glTranslatef(-5.5f * (float)strlen(button->name[0]), 6.0f, 0.0f);
            stroke(button->name[0]);
            glPopMatrix();
            
            glPushMatrix();
            glTranslatef(-5.5f * (float)strlen(button->name[1]), -4.0f, 0.0f);
            stroke(button->name[1]);
            glPopMatrix();
            
            glPushMatrix();
            glTranslatef(-5.5f * (float)strlen(button->name[2]), -14.0f, 0.0f);
            stroke(button->name[2]);
            glPopMatrix();
            break;
    }

    glLineWidth(1.0f);
    
    // Réactiver le test de profondeur
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}
button_struct *which_button(mx, my)
int mx, my;
{
    float x, y;
    button_struct *button;

    sizex = glutGet(GLUT_WINDOW_WIDTH);
    sizey = glutGet(GLUT_WINDOW_HEIGHT);
    originx = 0;
    originy = 0;

    button = current_buttons;

    if (button) do {
        // Convertir les coordonnées de fenêtre en coordonnées OpenGL
        x = (float)(mx-sizex/2-originx)/(float)sizex;
        y = (float)((sizey-my)-sizey/2-originy)/(float)sizey/1.25; // Inverser Y

        x = x * (-button->tz+SCREEN+THICK*2.0);
        y = y * (-button->tz+SCREEN+THICK*2.0);

        if (button->tx-0.625<x && button->tx+0.625>x &&
            button->ty-0.5<y && button->ty+0.5>y) {
            return (button);
        }

    } while ((button = button->next) != 0);

    return(NULL);
}

add_button_to_path(current, button)
button_struct *current;
button_struct *button;
{
    path_struct *step;

    step = (path_struct *)malloc(sizeof(path_struct));

    step->current_buttons = current;
    step->button = button;
    step->back = path;
    path = step;
}

void doclear() {
    glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // S'assurer que le depth buffer est bien configuré
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
}