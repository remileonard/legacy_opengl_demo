/*
 * Copyright 1993, 1994, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 * rights reserved under the Copyright Laws of the United States.
 */
#include "porting/iris2ogl.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

#define SIZE 6 /* Number of cubes across the playing area */

#define ZSPEED -0.1 /* How fast the pieces fall initially (one */
/* unit is one grid space) */
#define ZSPEEDDELTA -0.02 /* Delta for above, every time a plane is */
/* cleared. */
#define FASTDROP -1.0 /* How fast it falls if we're going fast. */

#define SCOREPIECE 10 /* Score per piece */
#define SCOREFILL 500 /* Score for filling a plane */
#define SCOREFAST 2   /* Score for each frame we have FASTDROP */

#define CUB_HALFTONE 1

unsigned short halftone[] = {0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa,
                             0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa};

int board[SIZE][SIZE][SIZE];
int height[SIZE][SIZE];
int planecount[SIZE];

typedef struct _PaintRec {
    int x, y, z, m;
} PaintRec, *Paint;

PaintRec paintlist[SIZE * SIZE * SIZE];
int numpaint;

typedef int Shape[4][3];

typedef struct _PieceRec {
    int rotatevertex; /* 1 if rotate around vertex, 0 if */
    /* rotate around face */
    int numshapes;
    int material;
    Shape shapes[6];
} PieceRec, *Piece;

PieceRec bar = {
    1, 2, 1, {{{-3, 1, 0}, {-1, 1, 0}, {1, 1, 0}, {3, 1, 0}}, {{1, 1, 4}, {1, 1, 2}, {1, 1, 0}, {1, 1, -2}}}};

PieceRec square = {
    1, 2, 2, {{{-1, -1, 0}, {1, -1, 0}, {-1, 1, 0}, {1, 1, 0}}, {{-1, -1, 0}, {1, -1, 0}, {-1, -1, 2}, {1, -1, 2}}}};

PieceRec ell = {0,
                6,
                3,
                {
                    {{-2, 0, 0}, {0, 0, 0}, {2, 0, 0}, {2, 2, 0}},
                    {{-2, 0, 0}, {0, 0, 0}, {2, 0, 0}, {2, 0, 2}},
                    {{-2, 0, 0}, {0, 0, 0}, {2, 0, 0}, {2, -2, 0}},
                    {{-2, 0, 0}, {0, 0, 0}, {2, 0, 0}, {2, 0, -2}},
                    {{0, -2, 2}, {0, 0, 2}, {0, 0, 0}, {0, 0, -2}},
                    {{0, -2, -2}, {0, 0, -2}, {0, 0, 0}, {0, 0, 2}},
                }};

PieceRec axis = {
    1, 2, 4, {{{-1, 1, 0}, {-1, -1, 0}, {1, -1, 0}, {-1, -1, -2}}, {{-1, 1, 0}, {-1, -1, 0}, {1, -1, 0}, {-1, -1, 2}}}};

PieceRec tee = {0,
                4,
                5,
                {{{-2, 0, 0}, {0, 0, 0}, {0, 2, 0}, {2, 0, 0}},
                 {{-2, 0, 0}, {0, 0, 0}, {0, 0, 2}, {2, 0, 0}},
                 {{0, 0, -2}, {0, 0, 0}, {0, 2, 0}, {0, 0, 2}},
                 {{-2, 0, 0}, {0, 0, 0}, {0, 0, -2}, {2, 0, 0}}}};

PieceRec zig = {0,
                4,
                6,
                {
                    {{-2, 0, 0}, {0, 0, 0}, {0, 2, 0}, {2, 2, 0}},
                    {{0, 0, 2}, {0, 0, 0}, {0, 2, 0}, {0, 2, -2}},
                    {{-2, 2, 0}, {0, 2, 0}, {0, 0, 0}, {2, 0, 0}},
                    {{-2, 0, 2}, {0, 0, 2}, {0, 0, 0}, {2, 0, 0}},
                }};

PieceRec twist1 = {1,
                   3,
                   7,
                   {{{-1, -1, 2}, {-1, -1, 0}, {-1, 1, 0}, {1, 1, 0}},
                    {{-1, -1, 0}, {-1, 1, 0}, {-1, 1, 2}, {1, 1, 2}},
                    {{-1, 1, 0}, {-1, 1, 2}, {-1, -1, 2}, {1, -1, 2}}}};

PieceRec twist2 = {1,
                   3,
                   8,
                   {{{-1, -1, 2}, {-1, -1, 0}, {1, -1, 0}, {1, 1, 0}},
                    {{-1, -1, 0}, {1, -1, 0}, {1, -1, 2}, {1, 1, 2}},
                    {{1, -1, 0}, {1, -1, 2}, {-1, -1, 2}, {-1, 1, 2}}}};

Piece pieces[] = {&bar, &square, &ell, &axis, &tee, &zig, &twist1, &twist2};
#define NUMPIECES 8

int score;
char scorestr[20];
Piece curpiece;
int curshape;
float currot;
Coord curx, cury, curz;
float dropspeed;
float zspeed;
int gameover;
int usebevels = TRUE;

int debug = FALSE;

Matrix objmat = {
    {1.0, 0.0, 0.0, 0.0},
    {0.0, 1.0, 0.0, 0.0},
    {0.0, 0.0, 1.0, 0.0},
    {0.0, 0.0, 0.0, 1.0},
};

Matrix idmat = {
    {1.0, 0.0, 0.0, 0.0},
    {0.0, 1.0, 0.0, 0.0},
    {0.0, 0.0, 1.0, 0.0},
    {0.0, 0.0, 0.0, 1.0},
};

static int azimuth = 0, elevation = 0;

static float light[] = {

    AMBIENT, 0.1, 0.1, 0.1, LCOLOR, 1.0, 1.0, 1.0, POSITION, 1.0, 0.5, 0.5, 0.0, LMNULL};

#define NUMMATERIALS 9
float materials[NUMMATERIALS][15] = {{
                                         AMBIENT,
                                         0.3,
                                         0.3,
                                         0.3,
                                         DIFFUSE,
                                         0.8,
                                         0.8,
                                         0.8,
                                         SPECULAR,
                                         1.0,
                                         1.0,
                                         1.0,
                                         SHININESS,
                                         30.0,
                                         LMNULL,
                                     },
                                     {
                                         AMBIENT,
                                         0.3,
                                         0.1,
                                         0.1,
                                         DIFFUSE,
                                         0.8,
                                         0.1,
                                         0.1,
                                         SPECULAR,
                                         1.0,
                                         0.1,
                                         0.1,
                                         SHININESS,
                                         30.0,
                                         LMNULL,
                                     },
                                     {
                                         AMBIENT,
                                         0.7,
                                         0.1,
                                         0.3,
                                         DIFFUSE,
                                         0.1,
                                         0.8,
                                         0.1,
                                         SPECULAR,
                                         0.1,
                                         1.0,
                                         0.5,
                                         SHININESS,
                                         30.0,
                                         LMNULL,
                                     },
                                     {
                                         AMBIENT,
                                         0.1,
                                         0.3,
                                         0.1,
                                         DIFFUSE,
                                         0.1,
                                         0.8,
                                         0.1,
                                         SPECULAR,
                                         0.1,
                                         1.0,
                                         0.1,
                                         SHININESS,
                                         30.0,
                                         LMNULL,
                                     },
                                     {
                                         AMBIENT,
                                         0.3,
                                         0.3,
                                         0.1,
                                         DIFFUSE,
                                         0.8,
                                         0.8,
                                         0.1,
                                         SPECULAR,
                                         1.0,
                                         1.0,
                                         0.1,
                                         SHININESS,
                                         30.0,
                                         LMNULL,
                                     },
                                     {
                                         AMBIENT,
                                         0.1,
                                         0.3,
                                         0.3,
                                         DIFFUSE,
                                         0.1,
                                         0.8,
                                         0.8,
                                         SPECULAR,
                                         0.1,
                                         1.0,
                                         1.0,
                                         SHININESS,
                                         30.0,
                                         LMNULL,
                                     },
                                     {
                                         AMBIENT,
                                         0.3,
                                         0.1,
                                         0.3,
                                         DIFFUSE,
                                         0.8,
                                         0.1,
                                         0.8,
                                         SPECULAR,
                                         1.0,
                                         0.1,
                                         1.0,
                                         SHININESS,
                                         30.0,
                                         LMNULL,
                                     },
                                     {
                                         AMBIENT,
                                         0.3,
                                         0.1,
                                         0.7,
                                         DIFFUSE,
                                         0.1,
                                         0.8,
                                         0.1,
                                         SPECULAR,
                                         0.5,
                                         1.0,
                                         0.1,
                                         SHININESS,
                                         30.0,
                                         LMNULL,
                                     },
                                     {
                                         AMBIENT,
                                         0.3,
                                         0.7,
                                         0.1,
                                         DIFFUSE,
                                         0.1,
                                         0.1,
                                         0.8,
                                         SPECULAR,
                                         0.1,
                                         0.1,
                                         0.5,
                                         SHININESS,
                                         30.0,
                                         LMNULL,
                                     }};

static float light_model[] = {

    AMBIENT, 1.0, 1.0, 1.0, LOCALVIEWER, 0.0, LMNULL};

enum { NOTHING, ORIENT, MOVE } mode;

short omx, mx, omy, my; /* old and new mouse position */

/* like qread only compresses extra MOUSEX, MOUSEY events */

/*
static long nextqueue(short*);
static long nextqtest();
*/

#define nextqueue qread
#define nextqtest qtest

draw_cube(int x, int y, int z);

addscore(int value) {
    score += value;
    sprintf(scorestr, "%d", score);
}

int getgridloc(int which, int *rx, int *ry, int *rz) {
    int dx, dy, dz, t;
    Shape *s;
    s = curpiece->shapes + curshape;
    dx = (*s)[which][0];
    dy = (*s)[which][1];
    dz = (*s)[which][2];
    if (currot == 90.0) {
        t = dx;
        dx = -dy;
        dy = t;
    } else if (currot == 180.0) {
        dx = -dx;
        dy = -dy;
    } else if (currot == 270.0) {
        t = dx;
        dx = dy;
        dy = -t;
    }
    *rx = (int)((curx + dx) / 2.0);
    *ry = (int)((cury + dy) / 2.0);
    *rz = (int)floor((curz + dz) / 2.0);
    /*    fprintf(stderr, "%d %d %d %d\n", which, *rx, *ry, *rz); */
}

setboard(int x, int y, int z, int m) {
    if (board[x][y][z] == 0) {
        Paint ptr = paintlist + numpaint;
        board[x][y][z] = m;
        ptr->x = x;
        ptr->y = y;
        ptr->z = z;
        ptr->m = m;
        numpaint++;
        if (height[x][y] <= z)
            height[x][y] = z + 1;
        planecount[z]++;
    }
}

checkfilled() {
    int z, i, j, k;
    for (z = 0; z < SIZE; z++) {
        while (planecount[z] == SIZE * SIZE) {
            addscore(SCOREFILL);
            zspeed += ZSPEEDDELTA;
            numpaint = 0;
            for (i = 0; i < SIZE; i++)
                planecount[i] = 0;
            for (i = 0; i < SIZE; i++) {
                for (j = 0; j < SIZE; j++) {
                    height[i][j] = 0;
                    for (k = 0; k < SIZE - 1; k++) {
                        if (k >= z)
                            board[i][j][k] = board[i][j][k + 1];
                        if (board[i][j][k]) {
                            int m = board[i][j][k];
                            board[i][j][k] = 0;
                            setboard(i, j, k, m);
                        }
                    }
                    board[i][j][SIZE - 1] = 0;
                }
            }
        }
    }
}

refigurefallenobjects() {
    int i;
    Paint ptr;
    makeobj(2);
    for (i = 0, ptr = paintlist; i < numpaint; i++, ptr++) {
        lmbind(MATERIAL, ptr->m);
        pushmatrix();
        translate(ptr->x + 0.5, ptr->y + 0.5, ptr->z + 0.5);
        draw_cube(ptr->x, ptr->y, ptr->z);
        popmatrix();
    }
    closeobj();
}

keeppieceonboard() {
    int i, x, y, z;
    for (i = 0; i < 4; i++) {
        getgridloc(i, &x, &y, &z);
        if (x < 0)
            curx += 2.0 * (-x);
        if (y < 0)
            cury += 2.0 * (-y);
        if (x >= SIZE)
            curx -= 2.0 * (x - SIZE + 1);
        if (y >= SIZE)
            cury -= 2.0 * (y - SIZE + 1);
    }
}

dopause() {
    static char mesg[30][60] = {
        "Paused  -  Press <Return> to continue",
        "  ",
        "Left button either drags the falling piece around,",
        "or rotates the scene if selected somewhere not on",
        "the falling piece.",
        " ",
        "Middle button cycles the piece through all useful",
        "rotations about the X and Y axis.",
        " ",
        "Right button rotates the piece about the Z axis.",
        " ",
        "Numeric pad keys move piece one unit in the XY plane.",
        " ",
        "Space  - quickly drops the piece down (extra points)",
        "B      - Toggles bevelled cubes (helps performance)",
        "P or H - pauses game and shows this help",
        "F1     - Shows view from above",
        "F2     - Shows view from below",
        "Esc    - exits",
        " ",
        "If you fill an entire plane, it will disappear,",
        "and you get 500 points.",
        "",
    };
    int width, height, i;
    short dev, value;
    for (;;) {
        czclear(0, 0);
        getsize(&width, &height);
        loadmatrix(idmat);
        ortho2(0, width, 0, height);
        RGBcolor(0, 255, 0);
        for (i = 0; mesg[i][0]; i++) {
            cmov2(2, height - (i + 1) * 14);
            charstr(mesg[i]);
        }
        swapbuffers();
        dev = nextqueue(&value);
        if (dev == ESCKEY)
            exit(0);
        if (dev == REDRAW)
            reshapeviewport();
        if (dev == RETKEY)
            return;
    }
}

int pushit(int dx, int dy) {
    curx += dx * 2;
    cury += dy * 2;
    if (!validposition()) {
        curx -= dx * 2;
        cury -= dy * 2;
    }
}

main(int argc, char **argv) {

    int dev;
    short val;
    int i;
    time_t t;
    int finished = FALSE;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-debug") == 0)
            debug = TRUE;
        else {
            fprintf(stderr, "Usage: %s [-debug]\n", argv[0]);
            exit(1);
        }
    }

    time(&t);
    srandom(t);

    initialize();

    initgame();

    while (!finished) {
        draw_scene();

        while (!finished && (nextqtest() || gameover)) {

            switch (dev = nextqueue(&val)) {

            case ESCKEY:
                if (val == 0)
                    finished = TRUE; /* exit on up, not down */
                break;

            case GKEY:
                if (gameover)
                    initgame();
                break;

            case HKEY:
            case PKEY:
                dopause();
                break;

            case BKEY:
                if (val) {
                    usebevels = !usebevels;
                    define_cube();
                    refigurefallenobjects();
                }
                break;

            case SPACEKEY:
                if (val) {
                    dropspeed = FASTDROP;
                }
                break;

            case F1KEY:
                azimuth = elevation = 0;
                break;

            case F2KEY:
                azimuth = 0;
                elevation = 1800;
                break;

            case REDRAW:
                reshapeviewport();
                draw_scene();
                break;

            case LEFTMOUSE:
                if (nextqtest() == MOUSEX) {
                    nextqueue(&mx);
                    if (nextqtest() == MOUSEY)
                        nextqueue(&my);
                }
                omx = mx;
                omy = my;
                if (val) {
                    mode = ORIENT;
                    if (curpiece) {
                        long ox, oy;
                        unsigned long pixel;
                        czclear(0, 0);
                        draw_piece();
                        getorigin(&ox, &oy);
                        lrectread(mx - ox, my - oy, mx - ox, my - oy, &pixel);
                        if (pixel != 0)
                            mode = MOVE;
                        /*
                                    short buf[10];
                                    int numpicked;
                                    buf[0] = 0;
                                    pick(buf, 10);
                                    perspective(400, 5.0/4.0, 2.0, 6.0);
                                    loadname(0);
                                    clear();
                                    loadname(2);
                                    draw_piece();
                                    if (endpick(buf) > 1) {
                                        mode = MOVE;
                                    }
                        */
                    }
                } else
                    mode = NOTHING;
                break;

            case MIDDLEMOUSE:
                if (val && curpiece) {
                    curshape = (curshape + 1) % curpiece->numshapes;
                    keeppieceonboard();
                }
                break;

            case RIGHTMOUSE:
                if (val && curpiece) {
                    currot += 90.0;
                    if (currot >= 360.0)
                        currot = 0.0;
                    keeppieceonboard();
                }
                break;

            case PAD1:
                if (val)
                    pushit(-1, -1);
                break;
            case PAD2:
                if (val)
                    pushit(0, -1);
                break;
            case PAD3:
                if (val)
                    pushit(1, -1);
                break;
            case PAD4:
                if (val)
                    pushit(-1, 0);
                break;
            case PAD6:
                if (val)
                    pushit(1, 0);
                break;
            case PAD7:
                if (val)
                    pushit(-1, 1);
                break;
            case PAD8:
                if (val)
                    pushit(0, 1);
                break;
            case PAD9:
                if (val)
                    pushit(1, 1);
                break;

            case MOUSEX:
                omx = mx;
                mx = val;
                break;

            case MOUSEY:
                omy = my;
                my = val;

                update_scene();
                break;
            }
            if (gameover && !nextqtest())
                draw_scene();
        }
        if (curpiece) {
            for (i = 0; i < 4; i++) {
                int x, y, z;
                getgridloc(i, &x, &y, &z);
                if (z < SIZE) {
                    if (z < 0 || board[x][y][z]) {
                        do {
                            curz += 2.0;
                            getgridloc(i, &x, &y, &z);
                        } while (z < 0);
                        for (i = 0; i < 4; i++) {
                            getgridloc(i, &x, &y, &z);
                            if (z >= 0 && z < SIZE) {
                                setboard(x, y, z, curpiece->material);
                            } else {
                                gameover = TRUE;
                                curpiece = NULL;
                                break;
                            }
                        }
                        if (!gameover) {
                            addscore(SCOREPIECE);
                            checkfilled();
                            pickpiece();
                        }
                        refigurefallenobjects();
                        break;
                    }
                }
            }
        }
        curz += dropspeed;
        if (dropspeed == FASTDROP && curpiece)
            addscore(SCOREFAST);
    }
}

initgame() {
    int i, j, k;
    for (i = 0; i < SIZE; i++) {
        planecount[i] = 0;
        for (j = 0; j < SIZE; j++) {
            height[i][j] = 0;
            for (k = 0; k < SIZE; k++) {
                board[i][j][k] = 0;
            }
        }
    }
    numpaint = 0;
    pickpiece();
    score = 0;
    addscore(0);
    zspeed = ZSPEED;
    gameover = FALSE;
    refigurefallenobjects();
    draw_scene();
    define_cube();
}

int myran(int t) { return random() % t; }

int outofbounds() {
    int i, x, y, z;
    for (i = 0; i < 4; i++) {
        getgridloc(i, &x, &y, &z);
        if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0)
            return TRUE;
    }
    return FALSE;
}

pickpiece() {
    curpiece = pieces[myran(NUMPIECES)];
    curshape = myran(curpiece->numshapes);
    currot = myran(4) * 90.0;
    curz = (Coord)(SIZE * 2);
    do {
        curx = (Coord)(myran(SIZE) * 2 + curpiece->rotatevertex);
        cury = (Coord)(myran(SIZE) * 2 + curpiece->rotatevertex);
    } while (outofbounds());
    dropspeed = ZSPEED;
}

initialize() {
    int i;

    if (debug)
        foreground();

    prefsize(500, 400);
    winopen("Cubetris");
    keepaspect(5, 4);
    winconstraints();
    icontitle("Cubetris");

    doublebuffer();
    RGBmode();
    backface(TRUE);
    zbuffer(TRUE);
    /* fprintf(stderr, "%d, %d\n", getgdesc(GD_ZMAX), getgdesc(GD_ZMIN)); */
    lsetdepth(getgdesc(GD_ZMAX), 0);
    gconfig();

    zfunction(ZF_GREATER);
    shademodel(FLAT);

    qdevice(ESCKEY);
    qdevice(BKEY);
    qdevice(GKEY);
    qdevice(HKEY);
    qdevice(PKEY);
    qdevice(RETKEY);
    qdevice(SPACEKEY);
    qdevice(F1KEY);
    qdevice(F2KEY);
    qdevice(LEFTMOUSE);
    qdevice(MIDDLEMOUSE);
    qdevice(RIGHTMOUSE);
    qdevice(MOUSEX);
    qdevice(MOUSEY);
    qdevice(PAD1);
    qdevice(PAD2);
    qdevice(PAD3);
    qdevice(PAD4);
    qdevice(PAD5);
    qdevice(PAD6);
    qdevice(PAD7);
    qdevice(PAD8);
    qdevice(PAD9);

    tie(LEFTMOUSE, MOUSEX, MOUSEY);

    defpattern(CUB_HALFTONE, 16, halftone);

    mmode(MVIEWING);

    lmdef(DEFLIGHT, 1, 0, light);
    lmdef(DEFLMODEL, 1, 0, light_model);
    for (i = 0; i < NUMMATERIALS; i++) {
        lmdef(DEFMATERIAL, i + 1, 0, materials[i]);
    }
}

Coord roundit(Coord v, int makeodd) {
    if (makeodd) {
        return 2 * ((int)((v - 1.0) / 2.0)) + 1;
    } else {
        return 2 * ((int)(v / 2.0));
    }
}

Coord coordmax(Coord a, Coord b) { return (a > b) ? a : b; }

validposition() {
    int i, x, y, z;
    if (outofbounds())
        return FALSE;
    for (i = 0; i < 4; i++) {
        getgridloc(i, &x, &y, &z);
        if (board[x][y][z])
            return FALSE;
    }
    return TRUE;
}

update_scene() {
    long ox, oy;
    Coord x1, y1, z1, x2, y2, z2, oldx, oldy, destx, desty, dx, dy, z;
    int nsteps, i;

    switch (mode) {

    case ORIENT:
        orient();
        break;
    case MOVE:
        if (curpiece == NULL)
            return;
        getorigin(&ox, &oy);
        mapw(1, mx - ox, my - oy, &x1, &y1, &z1, &x2, &y2, &z2);
        if (z1 == z2)
            break;
        oldx = curx;
        oldy = cury;
        z = curz / 2.0 + 0.5;
        curx = (x2 * (z - z1) - x1 * (z - z2)) / (z2 - z1);
        cury = (y2 * (z - z1) - y1 * (z - z2)) / (z2 - z1);
        /*
        fprintf(stderr, "%f %f   %f %f %f %f %f %f\n", mx, my, x1, y1, z1, x2, y2, z2);
        fprintf(stderr, "%f %f    \n", curx, cury);
        */
        curx = roundit((curx - 0.5) * 2, curpiece->rotatevertex);
        cury = roundit((cury - 0.5) * 2, curpiece->rotatevertex);
        keeppieceonboard();
        destx = curx;
        desty = cury;

        nsteps = coordmax(fabs(destx - oldx), fabs(desty - cury));
        if (nsteps == 0)
            break;
        dx = (destx - oldx) / nsteps;
        dy = (desty - oldy) / nsteps;
        for (; nsteps > 0; nsteps--) {
            oldx += dx;
            oldy += dy;
            curx = roundit(oldx, curpiece->rotatevertex);
            cury = roundit(oldy, curpiece->rotatevertex);
            if (!validposition()) {
                oldx -= dx;
                oldy -= dy;
                curx = roundit(oldx, curpiece->rotatevertex);
                cury = roundit(oldy, curpiece->rotatevertex);
                break;
            }
        }
        break;
    }

    /*    if (mode) draw_scene(); */
}

orient() {

    /*
        pushmatrix();

        loadmatrix(idmat);
    */
    azimuth -= mx - omx;
    elevation -= omy - my;
    /*
        polarview(0, azimuth, elevation, 0);
        getmatrix(objmat);
    */

    /*
        rotate(mx-omx, 'y');
        rotate(omy-my, 'x');

        multmatrix(objmat);
        getmatrix(objmat);
    */

    /*    popmatrix(); */
}

draw_scene() {
    czclear(0, 0);

    perspective(400, 5.0 / 4.0, 2.0, 6.0);
    loadmatrix(idmat);
    translate(0.0, 0.0, -4.0);

    RGBcolor(0, 255, 0);
    cmov(-1.3, -1.0, 1.0);
    charstr(scorestr);
    if (gameover)
        charstr(" Press 'G' for new game");

    lmbind(LIGHT0, 1);
    lmbind(LMODEL, 1);

    /*    multmatrix(objmat); */
    polarview(0, azimuth, elevation, 0);

    /*
     * At this point, we're ready to draw stuff in a cube centered at
     * (0,0,0) and 2 units on a side.
     */

    draw_grid();

    /*
     * Now, I want to change coordinate axis to range from (0,0,0) to
     * (SIZE,SIZE,SIZE).  This makes each position on a board a unit
     * cube.
     */

    translate(-1.0, -1.0, -1.0);
    scale(2.0 / SIZE, 2.0 / SIZE, 2.0 / SIZE);

    makeobj(1);
    closeobj(); /* Save these coordinates in an object. */

    callobj(2); /* Paint fallen pieces */

    draw_piece();

#define FRAMES 100
    if (debug) {
        static long frames = 1000, lasttime;
        if (++frames >= FRAMES) {
            long t = time((long *)0);
            if (frames < 1000) {
                fprintf(stderr, "%d frames/sec\n", frames / (t - lasttime));
            }
            lasttime = t;
            frames = 0;
        }
    }

    swapbuffers();
}

float cube_vtx[8][3] = {

    {
        0.5,
        0.5,
        -0.5,
    },
    {
        0.5,
        -0.5,
        -0.5,
    },
    {
        -0.5,
        -0.5,
        -0.5,
    },
    {
        -0.5,
        0.5,
        -0.5,
    },
    {
        0.5,
        0.5,
        0.5,
    },
    {
        0.5,
        -0.5,
        0.5,
    },
    {
        -0.5,
        -0.5,
        0.5,
    },
    {
        -0.5,
        0.5,
        0.5,
    },
};

float normals[6][3] = {
    {0.0, 0.0, -1.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {-1.0, 0.0, 0.0}, {0.0, -1.0, 0.0},
};

/* Drws the playing grid. */

float wallface[4][3] = {
    {1.0, 1.0, -1.0},
    {1.0, -1.0, -1.0},
    {1.0, -1.0, 1.0},
    {1.0, 1.0, 1.0},
};
float wallnormal[3] = {-1.0, 0.0, 0.0};

draw_grid() {
    int i, j;
    Coord x, z;

    zbuffer(FALSE);

    pushmatrix();
    lmbind(MATERIAL, 1);
    for (i = 0; i < 5; i++) {
        bgnpolygon();
        n3f(wallnormal);
        v3f(wallface[0]);
        v3f(wallface[1]);
        v3f(wallface[2]);
        v3f(wallface[3]);
        endpolygon();
        if (i < 3)
            rotate(900, 'z');
        else if (i == 3)
            rotate(900, 'y');
    }

    popmatrix();

    zbuffer(TRUE);

    pushmatrix();

    RGBcolor(0, 0, 255);
    for (i = 0; i < 2; i++) {
        for (j = 0; j <= SIZE; j++) {
            x = ((float)j) / (SIZE / 2.0) - 1.0;
            move(x, 1.0, 1.0);
            draw(x, 1.0, -1.0);
            draw(x, -1.0, -1.0);
            draw(x, -1.0, 1.0);
        }
        if (i == 0)
            rotate(900, 'z');
    }

    for (j = 0; j <= SIZE; j++) {
        move(-1.0, -1.0, -1.0);
        draw(1.0, -1.0, -1.0);
        draw(1.0, 1.0, -1.0);
        draw(-1.0, 1.0, -1.0);
        draw(-1.0, -1.0, -1.0);
        translate(0.0, 0.0, 2.0 / SIZE);
    }

    popmatrix();
}

#define D 0.1 /* Bevel size */

float bevel_vtx[11][3] = {
    /* Cube face */
    {0.5, 0.0, 0.0},
    {0.5, 0.5 - D, 0.0},
    {0.5, 0.5 - D, 0.5 - D},
    {0.5, 0.0, 0.5 - D},

    /* Edge face (Bevel) */
    {0.5, 0.5 - D, 0.0},
    {0.5 - D, 0.5, 0.0},
    {0.5 - D, 0.5, 0.5 - D},
    {0.5, 0.5 - D, 0.5 - D},

    /* Vertex face (Bevel) */

    {0.5, 0.5 - D, 0.5 - D},
    {0.5 - D, 0.5, 0.5 - D},
    {0.5 - D, 0.5 - D, 0.5}, /* Wrong, but it will work... */
};

float bevel_normals[3][3] = {
    {1.0, 0.0, 0.0},
    {0.70710678, 0.70710678, 0.0},        /* = 1/sqrt(2) */
    {0.57735027, 0.57735027, 0.57735027}, /* = 1/sqrt(3) */
};

#undef D

float face[4][3];

#define M 0.45

float flatface[4][3] = {
    {M, M, M},
    {M, -M, M},
    {M, -M, -M},
    {M, M, -M},
};

#undef M

#define E 0.5
#define M 0.4
#define Z 0.0

float bevelface[4][3] = {
    {E, M, M},
    {E, -M, M},
    {E, -M, -M},
    {E, M, -M},
};

float edge[4][3] = {
    {E, M, -M},
    {M, E, -M},
    {M, E, M},
    {E, M, M},
};

float vertex[3][3] = {
    {E, M, M},
    {M, E, M},
    {M, M, E},
};

float norms[3][3] = {
    {1.0, 0.0, 0.0},
    {0.70710678, 0.70710678, 0.0},        /* = 1/sqrt(2) */
    {0.57735027, 0.57735027, 0.57735027}, /* = 1/sqrt(3) */
};

#undef E
#undef M
#undef Z

mydoone(float *x, float *y) {
    float t = *x;
    *x = -*y;
    *y = t;
}

myrotit(float obj[][3], int size, char dir) {
    int i;
    for (i = 0; i < size; i++) {
        switch (dir) {
        case 'x':
            mydoone(&(obj[i][1]), &(obj[i][2]));
            break;
        case 'y':
            mydoone(&(obj[i][2]), &(obj[i][0]));
            break;
        case 'z':
            mydoone(&(obj[i][0]), &(obj[i][1]));
            break;
        }
    }
}

myrot(float obj[][3], int size, char dir) {
    myrotit(obj, size, dir);
    myrotit(norms, 3, dir);
}

copyvector(float a[3], float b[3]) {
    int i;
    for (i = 0; i < 3; i++)
        a[i] = b[i];
}

float useface[6][4][3];
float useedge[12][4][3];
float usevertex[8][4][3];

float facenorm[6][3];
float edgenorm[12][3];
float vertexnorm[8][3];

/* Draws a cube, a unit on a side, centered at (0, 0, 0). */

define_cube() {
    int i, j, k, cur;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            face[i][j] = usebevels ? bevelface[i][j] : flatface[i][j];
        }
    }

    cur = 0;
    for (i = 0; i < 4; i++) {
        copyvector(facenorm[cur], norms[0]);
        for (j = 0; j < 4; j++) {
            copyvector(useface[cur][j], face[j]);
        }
        cur++;
        myrot(face, 4, 'z');
    }
    myrot(face, 4, 'y');
    copyvector(facenorm[cur], norms[0]);
    for (j = 0; j < 4; j++) {
        copyvector(useface[cur][j], face[j]);
    }
    cur++;
    myrot(face, 4, 'y');
    myrot(face, 4, 'y');

    copyvector(facenorm[cur], norms[0]);
    for (j = 0; j < 4; j++) {
        copyvector(useface[cur][j], face[j]);
    }
    cur++;

    myrot(face, 4, 'y');

    cur = 0;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            copyvector(edgenorm[cur], norms[1]);
            for (k = 0; k < 4; k++) {
                copyvector(useedge[cur][k], edge[k]);
            }
            cur++;
            myrot(edge, 4, 'y');
        }
        myrot(edge, 4, 'x');
    }
    myrot(edge, 4, 'x');

    cur = 0;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 4; j++) {
            copyvector(vertexnorm[cur], norms[2]);
            for (k = 0; k < 3; k++) {
                copyvector(usevertex[cur][k], vertex[k]);
            }
            cur++;
            myrot(vertex, 3, 'y');
        }
        myrot(vertex, 3, 'x');
        myrot(vertex, 3, 'x');
    }
}

draw_cube(int x, int y, int z) {

    int i;

#define ISEMPTY(a, b, c)                                                                                               \
    (!usebevels || a < 0 || a >= SIZE || b < 0 || b >= SIZE || c < 0 || c >= SIZE || board[a][b][c] == 0)

#define FACE(i)                                                                                                        \
    {                                                                                                                  \
        bgnpolygon();                                                                                                  \
        n3f(facenorm[i]);                                                                                              \
        v3f(useface[i][0]);                                                                                            \
        v3f(useface[i][1]);                                                                                            \
        v3f(useface[i][2]);                                                                                            \
        v3f(useface[i][3]);                                                                                            \
        endpolygon();                                                                                                  \
    }

    if (ISEMPTY(x + 1, y, z))
        FACE(0);
    if (ISEMPTY(x, y + 1, z))
        FACE(1);
    if (ISEMPTY(x - 1, y, z))
        FACE(2);
    if (ISEMPTY(x, y - 1, z))
        FACE(3);
    if (ISEMPTY(x, y, z - 1))
        FACE(4);
    if (ISEMPTY(x, y, z + 1))
        FACE(5);

    if (usebevels) {
        for (i = 0; i < 12; i++) {
            bgnpolygon();
            n3f(edgenorm[i]);
            v3f(useedge[i][0]);
            v3f(useedge[i][1]);
            v3f(useedge[i][2]);
            v3f(useedge[i][3]);
            endpolygon();
        }
        for (i = 0; i < 8; i++) {
            bgnpolygon();
            n3f(vertexnorm[i]);
            v3f(usevertex[i][0]);
            v3f(usevertex[i][1]);
            v3f(usevertex[i][2]);
            endpolygon();
        }
    }

    /*
        int i, j, k;
        pushmatrix();

    #define FACE \
        bgnpolygon();	\
        n3f(norms[0]);	\
        v3f(face[0]);	\
        v3f(face[1]);	\
        v3f(face[2]);	\
        v3f(face[3]);	\
        endpolygon();

        for (i=0 ; i<4 ; i++) {
        FACE;
        rotate(900, 'z');
        }
        rotate(900, 'y');
        FACE;
        rotate(1800, 'y');

        FACE;


    #undef FACE

        popmatrix();
        pushmatrix();

    #define EDGE	\
        bgnpolygon();	\
        n3f(norms[0]);	\
        v3f(edge[0]);	\
        v3f(edge[1]);	\
         v3f(edge[2]);	\
        v3f(edge[3]);	\
        endpolygon();


        for (i=0 ; i<3 ; i++) {
        for (j=0 ; j<4 ; j++) {
            EDGE;
            rotate(900, 'y');
        }
        rotate(900, 'x');
        }

    #undef EDGE

        popmatrix();

    #define VERTEX	\
        bgnpolygon();	\
        n3f(norms[0]);	\
        v3f(vertex[0]);	\
        v3f(vertex[1]);	\
        v3f(vertex[2]);	\
        endpolygon();

        for (i=0 ; i<2 ; i++) {
        for (j=0 ; j<4 ; j++) {
            VERTEX;
            rotate(900, 'y');

        }
        rotate(1800, 'x');
        }

    #undef VERTEX

    */
    /*
        for (i=0 ; i<2 ; i++) {
        for (j=0 ; j<4 ; j++) {
            for (k=0 ; k<3 ; k++) {
            bgnpolygon();
            n3f(bevel_normals[0]);
            v3f(bevel_vtx[0]);
            v3f(bevel_vtx[1]);
            v3f(bevel_vtx[2]);
            v3f(bevel_vtx[3]);
            endpolygon();

            bgnpolygon();
            n3f(bevel_normals[1]);
            v3f(bevel_vtx[4]);
            v3f(bevel_vtx[5]);
            v3f(bevel_vtx[6]);
            v3f(bevel_vtx[7]);
            endpolygon();

            bgnpolygon();
            n3f(bevel_normals[2]);
            v3f(bevel_vtx[8]);
            v3f(bevel_vtx[9]);
            v3f(bevel_vtx[10]);
            endpolygon();

            rotate(900, 'z');
            rotate(900, 'y');
            }
            rotate(900, 'y');
        }
        rotate(1800, 'x');
        }

    */

    /*
        bgnpolygon();
        n3f(normals[0]);
        v3f(cube_vtx[0]);
        v3f(cube_vtx[1]);
        v3f(cube_vtx[2]);
        v3f(cube_vtx[3]);
        endpolygon();

        bgnpolygon();
        n3f(normals[1]);
        v3f(cube_vtx[0]);
        v3f(cube_vtx[4]);
        v3f(cube_vtx[5]);
        v3f(cube_vtx[1]);
        endpolygon();

        bgnpolygon();
        n3f(normals[2]);
        v3f(cube_vtx[0]);
        v3f(cube_vtx[3]);
        v3f(cube_vtx[7]);
        v3f(cube_vtx[4]);
        endpolygon();

        bgnpolygon();
        n3f(normals[3]);
        v3f(cube_vtx[7]);
        v3f(cube_vtx[6]);
        v3f(cube_vtx[5]);
        v3f(cube_vtx[4]);
        endpolygon();

        bgnpolygon();
        n3f(normals[4]);
        v3f(cube_vtx[7]);
        v3f(cube_vtx[3]);
        v3f(cube_vtx[2]);
        v3f(cube_vtx[6]);
        endpolygon();

        bgnpolygon();
        n3f(normals[5]);
        v3f(cube_vtx[1]);
        v3f(cube_vtx[5]);
        v3f(cube_vtx[6]);
        v3f(cube_vtx[2]);
        endpolygon();
    */
    /*
        lmbind(LMODEL, 0);
        RGBcolor(255, 255, 255);

        bgnclosedline();
        v3f(cube_vtx[0]);
        v3f(cube_vtx[1]);
        v3f(cube_vtx[2]);
        v3f(cube_vtx[3]);
        endclosedline();

        bgnclosedline();
        v3f(cube_vtx[4]);
        v3f(cube_vtx[5]);
        v3f(cube_vtx[6]);
        v3f(cube_vtx[7]);
        endclosedline();

        bgnline();
        v3f(cube_vtx[0]);
        v3f(cube_vtx[4]);
        endline();

        bgnline();
        v3f(cube_vtx[1]);
        v3f(cube_vtx[5]);
        endline();

        bgnline();
        v3f(cube_vtx[2]);
        v3f(cube_vtx[6]);
        endline();

        bgnline();
        v3f(cube_vtx[3]);
        v3f(cube_vtx[7]);
        endline();

        RGBcolor(255, 0, 0);
        str[1] = 0;
        for (i=0 ; i<8 ; i++) {
        cmov(cube_vtx[i][0], cube_vtx[i][1], cube_vtx[i][2]);
        str[0] = i + '0';
        charstr(str);
        }
    */
}

draw_piece() {
    int i;
    Shape *s;
    if (curpiece == NULL)
        return;
    s = curpiece->shapes + curshape;
    pushmatrix();
    translate(curx / 2.0 + 0.5, cury / 2.0 + 0.5, curz / 2.0 + 0.5);
    rot(currot, 'z');
    lmbind(MATERIAL, curpiece->material);
    for (i = 0; i < 4; i++) {
        pushmatrix();
        translate((*s)[i][0] / 2.0, (*s)[i][1] / 2.0, (*s)[i][2] / 2.0);
        draw_cube(-5, -5, -5);
        popmatrix();
    }
    popmatrix();
    setpattern(CUB_HALFTONE);
    RGBcolor(0, 0, 0);
    for (i = 0; i < 4; i++) {
        int x, y, z;
        Coord fx, fy, fz;
        getgridloc(i, &x, &y, &z);
        fx = x;
        fy = y;
        fz = height[x][y] + 0.05;

        pmv(fx, fy, fz);
        pdr(fx + 1.0, fy, fz);
        pdr(fx + 1.0, fy + 1.0, fz);
        pdr(fx, fy + 1.0, fz);
        pdr(fx, fy, fz);
        pclos();
    }
    setpattern(0);
}

/*

static short qbuf[50];
static long num;
static int ptr;

static long nextqueue(short* val) {

    short dev;

    if (ptr>=num) {
    ptr = 0;
    num = blkqread(qbuf, 50);

    if (qbuf[ptr]==MOUSEX && qbuf[ptr+2]==MOUSEY) {
        ptr += 4;
        while (ptr+4<num && qbuf[ptr+4]==MOUSEX && qbuf[ptr+6]==MOUSEY) {
        ptr+=4;
        }
    }

    }

    dev = qbuf[ptr];
    *val = qbuf[ptr+1];
    ptr+=2;

    return (long)dev;
}

static long nextqtest() {
    if (ptr < num) return qbuf[ptr];
    return qtest();
}

*/
