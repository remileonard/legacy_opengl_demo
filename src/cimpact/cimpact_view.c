/*
 * cimpact_view.c - viewer for the models and the terrain of Certain Impact
 * (Paradigm Simulation, 1996), stored in Vega's fast file format (.fst).
 *
 * usage: cimpact_view [data directory | model.fst ...]
 */
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <GL/freeglut.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

#include "fst.h"

#define MAX_FILES 256

static char *files[MAX_FILES];
static int nfiles, cur = -1;
static pfNode *model;
static pfVec3 bmin, bmax, center;
static float radius = 1;
static float yaw = 30, pitch = 20, dist = 3;
static pfVec3 pan;
static int win_w = 1024, win_h = 768;
static int wire, textures = 1, lighting = 1, show_help = 1, paused;
static int drag_button = -1, drag_x, drag_y, drag_shift;
static double pause_time, time_offset;
static int nverts_total;

/* --- bounds --------------------------------------------------------------- */

static void bound_point(const pfVec3 p)
{
    int k;
    for (k = 0; k < 3; k++) {
        if (p[k] < bmin[k]) bmin[k] = p[k];
        if (p[k] > bmax[k]) bmax[k] = p[k];
    }
}

static void bounds(pfNode *n, pfMatrix m, int depth)
{
    int i, j;
    pfMatrix mm;
    if (!n || depth > 64)
        return;
    if (n->type == PFNODE_DCS || n->type == PFNODE_SCS)
        pfMultMat(mm, n->mat, m);
    else
        pfCopyMat(mm, m);
    for (i = 0; i < n->ngsets; i++) {
        pfGeoSet *g = n->gsets[i];
        const float *v = (const float *)g->attr[PFGS_COORD3];
        if (!v)
            continue;
        nverts_total += g->nverts;
        for (j = 0; j < g->nverts; j++) {
            pfVec3 p;
            pfXformPt3(p, v + 3 * j, mm);
            if (n->type == PFNODE_BBOARD && n->bbpos)
                pfAddVec3(p, p, n->bbpos[i]);
            bound_point(p);
        }
    }
    if (n->type == PFNODE_LOD || n->type == PFNODE_SEQUENCE) {
        if (n->nchildren)
            bounds(n->children[0], mm, depth + 1);
        return;
    }
    for (i = 0; i < n->nchildren; i++)
        bounds(n->children[i], mm, depth + 1);
}

/* --- files ---------------------------------------------------------------- */

static int is_fst(const char *name)
{
    size_t n = strlen(name);
    return (n > 4 && strcmp(name + n - 4, ".fst") == 0) ||
           (n > 6 && strcmp(name + n - 6, ".fst.Z") == 0) ||
           (n > 7 && strcmp(name + n - 7, ".fst.gz") == 0);
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static void add_file(const char *path)
{
    if (nfiles < MAX_FILES) {
        files[nfiles] = (char *)malloc(strlen(path) + 1);
        strcpy(files[nfiles++], path);
    }
}

static void scan_dir(const char *dir)
{
    int first = nfiles;
#ifdef _WIN32
    struct _finddata_t fd;
    char pat[4096];
    intptr_t h;
    sprintf(pat, "%.4000s\\*", dir);
    if ((h = _findfirst(pat, &fd)) == -1)
        return;
    do {
        if (is_fst(fd.name)) {
            char p[4096];
            sprintf(p, "%.2000s/%.1000s", dir, fd.name);
            add_file(p);
        }
    } while (_findnext(h, &fd) == 0);
    _findclose(h);
#else
    DIR *d = opendir(dir);
    struct dirent *e;
    if (!d)
        return;
    while ((e = readdir(d)) != NULL) {
        if (is_fst(e->d_name)) {
            char p[4096];
            sprintf(p, "%.2000s/%.1000s", dir, e->d_name);
            add_file(p);
        }
    }
    closedir(d);
#endif
    qsort(files + first, nfiles - first, sizeof(char *), cmp_str);
}

static void reset_view(void)
{
    yaw = 30;
    pitch = 20;
    dist = 2.5f * radius;
    pfSetVec3(pan, 0, 0, 0);
}

static void load(int i)
{
    pfMatrix ident;
    char title[512];
    if (nfiles == 0)
        return;
    cur = (i + nfiles) % nfiles;
    model = fstLoadFile(files[cur]);
    pfSetVec3(bmin, 1e30f, 1e30f, 1e30f);
    pfSetVec3(bmax, -1e30f, -1e30f, -1e30f);
    nverts_total = 0;
    pfMakeIdentMat(ident);
    bounds(model, ident, 0);
    if (bmin[0] > bmax[0]) {
        pfSetVec3(bmin, -1, -1, -1);
        pfSetVec3(bmax, 1, 1, 1);
    }
    pfAddVec3(center, bmin, bmax);
    pfScaleVec3(center, 0.5f, center);
    radius = 0.5f * pfDistancePt3(bmin, bmax);
    if (radius <= 0)
        radius = 1;
    reset_view();
    sprintf(title, "cimpact_view - %.400s", strrchr(files[cur], '/') ? strrchr(files[cur], '/') + 1 : files[cur]);
    glutSetWindowTitle(title);
}

/* --- drawing -------------------------------------------------------------- */

static void text(int x, int y, const char *s)
{
    glRasterPos2i(x, y);
    while (*s)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s++);
}

static void display(void)
{
    float lpos[4] = { 0.4f, -0.5f, 0.8f, 0 };
    float white[4] = { 1, 1, 1, 1 }, amb[4] = { 0.35f, 0.35f, 0.35f, 1 };
    double t = paused ? pause_time : pfGetTime() - time_offset;
    char buf[512];

    glViewport(0, 0, win_w, win_h);
    glClearColor(0.45f, 0.6f, 0.8f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (double)win_w / win_h, radius * 0.002, radius * 20 + dist * 2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-pan[0], -pan[1], -dist);
    glRotatef(pitch - 90, 1, 0, 0);     /* Performer is Z up */
    glRotatef(-yaw, 0, 0, 1);
    glTranslatef(-center[0], -center[1], -center[2]);

    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    glEnable(GL_LIGHT0);

    glPolygonMode(GL_FRONT_AND_BACK, wire ? GL_LINE : GL_FILL);
    if (lighting) pfEnable(PFEN_LIGHTING); else pfDisable(PFEN_LIGHTING);
    if (textures) pfEnable(PFEN_TEXTURE); else pfDisable(PFEN_TEXTURE);
    if (model)
        pfDrawScene(model, t);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (show_help) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, win_w, 0, win_h);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glColor3f(1, 1, 1);
        sprintf(buf, "%d/%d  %.300s  (%d vertices, size %.1f)", cur + 1, nfiles,
                nfiles ? files[cur] : "no file", nverts_total, 2 * radius);
        text(10, win_h - 20, buf);
        text(10, 10, "n/p model  drag: turn  right drag/wheel: zoom  shift drag: pan  "
             "w wire  t textures  l light  space pause  r reset  h help");
        glEnable(GL_DEPTH_TEST);
    }
    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    win_w = w;
    win_h = h > 0 ? h : 1;
}

static void idle(void)
{
    glutPostRedisplay();
}

static void keyboard(unsigned char k, int x, int y)
{
    (void)x; (void)y;
    switch (k) {
    case 27: case 'q': exit(0);
    case 'n': load(cur + 1); break;
    case 'p': load(cur - 1); break;
    case 'w': wire = !wire; break;
    case 't': textures = !textures; break;
    case 'l': lighting = !lighting; break;
    case 'h': show_help = !show_help; break;
    case 'r': reset_view(); break;
    case '+': case '=': dist *= 0.9f; break;
    case '-': dist /= 0.9f; break;
    case ' ':
        if (paused)
            time_offset = pfGetTime() - pause_time;
        else
            pause_time = pfGetTime() - time_offset;
        paused = !paused;
        break;
    }
}

static void special(int k, int x, int y)
{
    (void)x; (void)y;
    switch (k) {
    case GLUT_KEY_LEFT: yaw -= 5; break;
    case GLUT_KEY_RIGHT: yaw += 5; break;
    case GLUT_KEY_UP: pitch += 5; break;
    case GLUT_KEY_DOWN: pitch -= 5; break;
    case GLUT_KEY_PAGE_DOWN: load(cur + 1); break;
    case GLUT_KEY_PAGE_UP: load(cur - 1); break;
    }
}

static void mouse(int b, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        if (b == 3) { dist *= 0.9f; return; }
        if (b == 4) { dist /= 0.9f; return; }
        drag_button = b;
        drag_x = x;
        drag_y = y;
        drag_shift = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
    } else {
        drag_button = -1;
    }
}

static void motion(int x, int y)
{
    int dx = x - drag_x, dy = y - drag_y;
    drag_x = x;
    drag_y = y;
    if (drag_button == GLUT_MIDDLE_BUTTON || (drag_button == GLUT_LEFT_BUTTON && drag_shift)) {
        pan[0] -= dx * dist / win_h;
        pan[1] += dy * dist / win_h;
    } else if (drag_button == GLUT_LEFT_BUTTON) {
        yaw += dx * 0.4f;
        pitch += dy * 0.4f;
        if (pitch > 90) pitch = 90;
        if (pitch < -90) pitch = -90;
    } else if (drag_button == GLUT_RIGHT_BUTTON) {
        dist *= (float)pow(1.01, dy);
    }
}

int main(int argc, char **argv)
{
    int i;
    glutInit(&argc, argv);
    for (i = 1; i < argc; i++) {
        if (is_fst(argv[i])) {
            add_file(argv[i]);
        } else {
            char p[4096];
            sprintf(p, "%.4000s/models_ogl", argv[i]);
            scan_dir(p);
            sprintf(p, "%.4000s/contours_ogl", argv[i]);
            scan_dir(p);
            if (nfiles == 0)
                scan_dir(argv[i]);
        }
    }
    if (argc < 2) {
        scan_dir("data/cimpact/models_ogl");
        scan_dir("data/cimpact/contours_ogl");
    }
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(win_w, win_h);
    glutCreateWindow("cimpact_view");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idle);
    if (nfiles == 0)
        fprintf(stderr, "cimpact_view: no .fst file found\n");
    load(0);
    glutMainLoop();
    return 0;
}
