/*
 *                     OpenGL Maze Example
 *
 *              by  Stan Melax  melax@bioware.com
 *
 * In this little demo the player navigates through a simple maze
 * using the arrow keys.  The maze is defined by a 2D array where
 * each element in the array indicates solid or empty space.  This
 * program wraps polygon (quad) walls around the solid space and
 * disallows the player to navigate into solid space during the demo.
 * Note that all the walls are limited to being 90 degrees to each
 * other - there are no "angled" features.  The purpose of this
 * sample program is to show a beginning 3D game programmer some
 * things they can do.
 *
 * One other cool thing that this program does is that it constucts
 * a single quad strip to draw all the walls by doing a recursive
 * depth first search on the maze array data.
 *
 * Permission to execute this program, or look at the code is only
 * granted to those who do not like to sue other people :-)
 * Some of the window setup code was stolen from a simple Cosmo example.
 * OpenGL is a trademark of SGI.
 */
#include <windows.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#define IDM_APPLICATION_EXIT (101)
#define IDM_APPLICATION_TEXTURE (102)
#define IDM_APPLICATION_BANK (103)

int enablebank = 0;
int enabletexture = 0;

#define MAZE_HEIGHT (16)
#define MAZE_WIDTH (16)

// unfortunately due to the way the polygon walls are generated there
// are restrictions on what the wall/maze data can look like.  See below.
char *mazedata[MAZE_HEIGHT] = {
    "****************", "*       *      *", "* * *** * *    *", "* **  * ** * * *",
    "*     *      * *", "********** *** *", "*           *  *", "* ***** *** ****",
    "* *   *   *    *", "*   *******    *", "* *   *   *  * *", "* ***** **** * *",
    "*     *      * *", "** ** **** *** *", "*   * *    *   *", "************* **",
};

void readtexture() {
    unsigned char *image;
    int rc;
    // the bitmap must be a 24 bit bmp file thats 128x128
    // I think i mixed up red and blue componants - oh well :-)
    FILE *fp;
    fp = fopen("maze.bmp", "rb");
    if (!fp)
        return;
    fseek(fp, 54, SEEK_SET);
    image = (unsigned char *)malloc(128 * 128 * 3);
    assert(image);
    rc = fread(image, sizeof(unsigned char), 128 * 128 * 3, fp);
    assert(rc == 128 * 128 * 3);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // let texture wrap-around
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // use point sampleing (fastest)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);

    fclose(fp);
}

static void menu_callback(int value) {
    switch (value) {
    case IDM_APPLICATION_EXIT:
        glutLeaveMainLoop();
        break;
    case IDM_APPLICATION_TEXTURE:
        enabletexture ^= 1;
        (enabletexture ? glEnable : glDisable)(GL_TEXTURE_2D);
        if (enabletexture)
            readtexture();
        break;
    case IDM_APPLICATION_BANK:
        enablebank ^= 1;
        break;
    default:
        break;
    }
}
int wall(int x, int y) {
    // true if the region at x,y is solid space that
    // should be surrounded by other solid space or polygon walls
    return (x >= 0 && y >= 0 && x < MAZE_WIDTH && y < MAZE_HEIGHT && ' ' != mazedata[y][x]);
}

/*
 * The next group of routines implements the depth-first search
 * that is used to wrap a quad strip around all the solid regions of the
 * maze.  Note this enforces certain topological restrictions on the
 * maze data itself.  There cant be any loops, clusters, or floating pieces
 * existing by themselves.  The solid nodes must be a tree (graph theory speak).
 *
 */
int onopen(int x, int y) {
    //  returns whether node x,y is on the depth-first search open list
    assert(wall(x, y));
    return (mazedata[y][x] == '*');
}
void closeit(int x, int y) {
    //  puts node x,y on the closed list
    assert(wall(x, y));
    assert(onopen(x, y));
    mazedata[y][x] = 'X';
}
int neighbor(int x, int y, int w, int *nx, int *ny) {
    // if x,y has a neighbor in direction w then returns true
    switch (w) {
    case 0:
        *nx = x - 1;
        *ny = y;
        break;
    case 1:
        *nx = x;
        *ny = y + 1;
        break;
    case 2:
        *nx = x + 1;
        *ny = y;
        break;
    case 3:
        *nx = x;
        *ny = y - 1;
        break;
    default:
        assert(0);
    }
    return wall(*nx, *ny);
}
int diagnol(int x, int y, int w, int *nx, int *ny) {
    switch (w) {
    case 0:
        *nx = x - 1;
        *ny = y - 1;
        break;
    case 1:
        *nx = x - 1;
        *ny = y + 1;
        break;
    case 2:
        *nx = x + 1;
        *ny = y + 1;
        break;
    case 3:
        *nx = x + 1;
        *ny = y - 1;
        break;
    default:
        assert(0);
    }
    return wall(*nx, *ny);
}
// normal vectors for each wall direction
float nrml[4][3] = {
    {-1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
};
// default color for each wall direction
float clr[4][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f},
};
static float texcoordX = 0.0f;
int dw(int x, int y, int p) {
    // the recursive draw wall routine that extends the quad strip
    int w = p; // w is the current wall direction being considered
    closeit(x, y);
    do {
        int x2, y2;
        if (neighbor(x, y, w, &x2, &y2)) {
            if (onopen(x2, y2)) {
                dw(x2, y2, (w + 3) % 4);
            } else {
                assert((w + 1) % 4 == p); // or a loop or cluster exists
                return 1;
            }
        } else {
            float fx;
            float fy;
            if (diagnol(x, y, w, &x2, &y2) && onopen(x2, y2)) {
                dw(x2, y2, (w + 2) % 4);
            }
            glNormal3fv(nrml[w]); // useful iff using lighting
            glColor3fv(clr[w]);
            texcoordX = (texcoordX < 0.5) ? 1.0f : 0.0f;
            fx = (float)x + ((w == 1 || w == 2) ? 1.0f : 0.0f);
            fy = (float)y + ((w == 0 || w == 1) ? 1.0f : 0.0f);
            glTexCoord2f(texcoordX, 0.0f); // useful iff using textures
            glVertex3f(fx, fy, 0.0f);
            glTexCoord2f(texcoordX, 1.0f);
            glVertex3f(fx, fy, 1.0f);
        }

        w++;
        w %= 4;
    } while (w != p);
    return 1;
}
int drawwalls() {
    int dl;
    glNewList(dl = glGenLists(1), GL_COMPILE);
    glBegin(GL_QUAD_STRIP);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);
    dw(0, 0, 0);
    glEnd();
    glEndList();
    return dl;
}
//-----------------------------------------------

int drawtop() {
    // draws the top and the bottom of the maze
    // which is useful for overhead views
    // The display list created here is only used for the intro
    // spinning bit.  No optimizations such as using quad strips
    // or combining adjacent polygons are done here.
    int x, y, dl;
    glNewList(dl = glGenLists(1), GL_COMPILE);
    /*glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);*/
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    for (y = 0; y < MAZE_HEIGHT; y++) {
        for (x = 0; x < MAZE_WIDTH; x++) {
            if (wall(x, y)) {
                // bottomside:
                glVertex3f(x + 0.0f, y + 0.0f, 0.0f);
                glVertex3f(x + 0.0f, y + 1.0f, 0.0f);
                glVertex3f(x + 1.0f, y + 1.0f, 0.0f);
                glVertex3f(x + 1.0f, y + 0.0f, 0.0f);
                // topside:
                glVertex3f(x + 0.0f, y + 0.0f, 1.0f);
                glVertex3f(x + 1.0f, y + 0.0f, 1.0f);
                glVertex3f(x + 1.0f, y + 1.0f, 1.0f);
                glVertex3f(x + 0.0f, y + 1.0f, 1.0f);
            }
        }
    }
    glEnd();
    //glPopAttrib();
    glEndList();
    return (dl);
}

#define STARTING_POINT_X (1.5f);
#define STARTING_POINT_Y (1.5f);
#define STARTING_HEADING (90.0f);
float player_x = STARTING_POINT_X;
float player_y = STARTING_POINT_Y;
float player_h = STARTING_HEADING; // player's heading
float player_s = 0.0f;             // forward speed of the player
float player_m = 1.0f;             // speed multiplier of the player
float player_t = 0.0f;             // player's turning (change in heading)
float player_b = 0.0f;             // viewpoint bank (roll)
int walllist = 0;
int mazelist = 0;
void spinmaze(void);
void entermaze(void);
void navmaze(void);
void (*idlefunc)(void) = NULL;
int forward(float px, float py, float bf) {
    // this routine does wall collision detection
    // the inputs to this routine are:
    //         - the desired location
    //         - the minimum distance to wall allowed
    // changes:
    //         - the player's x and y coordinates
    // returns:
    //         - whether a wall caused change in target position
    // This is really easy with these walls that lie only on axes.
    // If the player collides into a wall at an angle he/she will
    // still slide along the wall - I hate programs where you stick
    // to the polygon and cant move until you back away from it.
    // This collision detection isn't perfect - if you're precise you
    // can jump through at a corner - but its tough.
    int x = ((int)player_x);
    int y = ((int)player_y);
    int h = 0; // number of walls hit
    if ((px > x + 1.0f - bf) && wall(x + 1, y)) {
        px = (float)(x) + 1.0f - bf;
        h++;
    }
    if (py > y + 1.0f - bf && wall(x, y + 1)) {
        py = (float)(y) + 1.0f - bf;
        h++;
    }
    if (px < x + bf && wall(x - 1, y)) {
        px = (float)(x) + bf;
        h++;
    }
    if (py < y + bf && wall(x, y - 1)) {
        py = (float)(y) + bf;
        h++;
    }
    player_x = px;
    player_y = py;
    return h;
}
void spinmaze(void) {
    static float spin = 720.0f;
    spin -= 5.0f; // TODO: lier au temps si tu veux

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -20.0f);
    glRotatef(spin, 0.0f, 1.0f, 1.0f);
    glTranslatef(-MAZE_WIDTH / 2.0f, -MAZE_HEIGHT / 2.0f, 0.0f);
    glCallList(walllist);
    glCallList(mazelist);
    glPopMatrix();
    glutSwapBuffers();

    if (spin <= 0.0f) {
        spin = 720.0f;
        idlefunc = entermaze;
        player_x = STARTING_POINT_X;
        player_y = STARTING_POINT_Y;
        player_h = STARTING_HEADING;
    }
}

void entermaze(void) {
    static float p = 0.0f;
    p += 0.02f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glPushMatrix();
    glRotatef(-90.0f * p, 1.0f, 0.0f, 0.0f);
    glRotatef(player_h * p, 0.0f, 0.0f, 1.0f);
    glTranslatef(-(player_x * p + MAZE_WIDTH / 2.0f * (1 - p)), -(player_y * p + MAZE_HEIGHT / 2.0f * (1 - p)),
                 -(0.5f * p + 20.0f * (1 - p)));
    glCallList(walllist);
    glCallList(mazelist);
    glPopMatrix();
    glutSwapBuffers();

    if (p >= 1.0f) {
        p = 0.0f;
        idlefunc = navmaze;
    }
}

void navmaze(void) {

    forward(player_x + player_m * player_s * (float)sin(player_h * 3.14 / 180),
            player_y + player_m * player_s * (float)cos(player_h * 3.14 / 180), 0.2f);
    player_h += player_t;
    player_b = 3 * player_b / 4 + player_t / 4;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glPushMatrix();

    // 1) Construire la vue (caméra)
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    if (enablebank)
        glRotatef(-player_b, 0.0f, 1.0f, 0.0f);
    glRotatef(player_h, 0.0f, 0.0f, 1.0f);
    glTranslatef(-player_x, -player_y, -0.5f);

    GLfloat light_position[] = {player_x, player_y, 0.0f, 1.0f};
    GLfloat light_dir_world[3] = {
        (GLfloat)sin(player_h * 3.14 / 180.0),
        (GLfloat)cos(player_h * 3.14 / 180.0),
        0.0f
    };

    glLightfv(GL_LIGHT0, GL_POSITION,       light_position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir_world);
    // 3) Dessiner la scène
    // Grand sol
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.6f, 0.0f);
    glVertex3f(0.0f      , 0.0f       , 0.0f);
    glVertex3f(MAZE_WIDTH, 0.0f       , 0.0f);
    glVertex3f(MAZE_WIDTH, MAZE_HEIGHT, 0.0f);
    glVertex3f(0.0f      , MAZE_HEIGHT, 0.0f);
    glEnd();

    // Grand plafond
    glNormal3f(0.0f, 0.0f, -1.0f);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.0f, 0.6f);
    glVertex3f(0.0f      , 0.0f       , 1.0f);
    glVertex3f(0.0f      , MAZE_HEIGHT, 1.0f);
    glVertex3f(MAZE_WIDTH, MAZE_HEIGHT, 1.0f);
    glVertex3f(MAZE_WIDTH, 0.0f       , 1.0f);
    glEnd();

    glCallList(walllist);
    glPopMatrix();
    glutSwapBuffers();

    if (player_x > MAZE_WIDTH || player_y > MAZE_HEIGHT) {
        idlefunc = spinmaze;
    }
}
static void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (h > 0) ? (float)w / (float)h : 1.0f, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
}

static void idle(void) {
    // on se contente de redessiner en continu
    glutPostRedisplay();
}

static void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
    case 27: // ESC
        glutLeaveMainLoop();
        break;
    case 'b':
    case 'B':
        enablebank ^= 1;
        break;
    case 't':
    case 'T':
        enabletexture ^= 1;
        (enabletexture ? glEnable : glDisable)(GL_TEXTURE_2D);
        if (enabletexture)
            readtexture();
        break;
    default:
        break;
    }
}

static void special(int key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
    case GLUT_KEY_LEFT:
        player_t = -5.0f;
        break;
    case GLUT_KEY_RIGHT:
        player_t = 5.0f;
        break;
    case GLUT_KEY_UP:
        player_s = 0.05f;
        break;
    case GLUT_KEY_DOWN:
        player_s = -0.02f;
        break;
    case GLUT_KEY_SHIFT_L:
    case GLUT_KEY_SHIFT_R:
        player_m = 3.0f;
        break;
    default:
        break;
    }
}

static void specialUp(int key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
    case GLUT_KEY_LEFT:
        if (player_t < 0.0f)
            player_t = 0.0f;
        break;
    case GLUT_KEY_RIGHT:
        if (player_t > 0.0f)
            player_t = 0.0f;
        break;
    case GLUT_KEY_UP:
    case GLUT_KEY_DOWN:
        player_s = 0.0f;
        break;
    case GLUT_KEY_SHIFT_L:
    case GLUT_KEY_SHIFT_R:
        player_m = 1.0f;
        break;
    default:
        break;
    }
}

static void display(void) {
    if (idlefunc) {
        idlefunc();
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glutSwapBuffers();
    }
}
static void initGL(void) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    // --- configuration de la lumière (lampe torche) ---
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // ambiance quasi nulle pour que le labyrinthe soit sombre hors du cône
    GLfloat light_ambient[]  = {0.01f, 0.01f, 0.01f, 1.0f};
    GLfloat light_diffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat light_specular[] = {0.0f, 0.0f, 0.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    // position/direction par défaut (seront recalculées dans navmaze)
    GLfloat light_position[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat light_dir_world[3] = {
        (GLfloat)sin(player_h * 3.14 / 180.0),
        (GLfloat)cos(player_h * 3.14 / 180.0),
        0.0f
    };

    glLightfv(GL_LIGHT0, GL_POSITION,       light_position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_dir_world);

     // cône un peu plus large et moins agressif
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF,   20.0f);  // au lieu de 12.0f
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 40.0f);  // au lieu de 80.0f

    // atténuation un peu plus douce
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,  0.8f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.3f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.1f);
    // Matériau de base neutre
    GLfloat mat_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat mat_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    walllist = drawwalls();
    mazelist = drawtop();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    idlefunc = navmaze;
}
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    // Double buffer + RGB + z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Maze Example (FreeGLUT)");

    int menu = glutCreateMenu(menu_callback);
    glutAddMenuEntry("Exit", IDM_APPLICATION_EXIT);
    glutAddMenuEntry("Add/Remove Texture", IDM_APPLICATION_TEXTURE);
    glutAddMenuEntry("Add/Remove Banking", IDM_APPLICATION_BANK);

    // bouton droit de la souris = menu
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);

    glutMainLoop();
    return 0;
}