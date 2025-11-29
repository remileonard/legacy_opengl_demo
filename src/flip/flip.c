/*
 * Copyright 1991, 1992, 1993, 1994, Silicon Graphics, Inc.
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
/*
 *	flip.c
 * A complete face-lift for spin
 * New, much better user interface
 * Better lighting controls
 *
 * Now, all we need is a better object format...
 * ... and buttons and panels
 */
#include "flip.h"
#include "light.h"
#include "porting/iris2ogl.h"
#include <stdio.h>

#define HDWBUG /* needed for CLOVER1 zfunction(GE_ZERO) hdwr bug */

/* Global variables */

static long XOrigin, YOrigin, Width, Height;
static int UsePrefposition = 0, UsePrefsize = 0;

static int nobjs;      /* How many objects we're spinning */
static flipobj **fobj; /* Array of flipobj pointers */
static flipobj lobj;   /* Stores lights' transformations */
static float zmax;
static float window_near, window_far, aspect;
static int totalpolys = 0;
static int update_view; /* True if we need to constantly redraw object */
static int show_performance = 0;

/* Tests for various hardware features */
static int blending_supported;
static int smoothlines_supported;
static int clover1;
Matrix idmat = {/* Useful Matrix to have around... */
                {1.0, 0.0, 0.0, 0.0},
                {0.0, 1.0, 0.0, 0.0},
                {0.0, 0.0, 1.0, 0.0},
                {0.0, 0.0, 0.0, 1.0}};

/* Prototypes for local functions */
void main(int, char **), parse_args(int, char **);
void init_windows(char *);
void init_menus(void), do_menus(void);
void read_files(void), draw_scene(void);
void redraw_scene(void);
void remember_view(float *, float *);
void draw_rate(int);
void draw_objects(void);
void update_objeulers(void);
void rand_rotation(float *);
void alpha_on(void), alpha_off(void);
void toggle_display(int), toggle_select(int);
void toggle_alpha(int), toggle_drawtype(int);
void toggle_swirl(int);
void toggle_performance(void);
void toggle_lightsource(int);
void toggle_spinlights(void), toggle_displights(void);
void select_all(void);
void do_objmaterials(int);
void remake_objmenu(int), remake_menus(void);
void remake_lightmenu(void);
void remake_mmenu(int), remake_dtmenu(int);
void remake_lsmenu(void);
void select_lmodel(int), remake_lmmenu(int);
void anything_moving(int *);

void main(int argc, char **argv) {
    readflipobj("x29.bin");
    parse_args(argc, argv); /* This reads in the files, too */
    init_windows(argv[0]);
    make_lights();

    add_event(ANY, ESCKEY, UP, ui_exit, NULL);
    qdevice(ESCKEY);
    add_event(ANY, WINQUIT, ANY, ui_exit, NULL);
    qdevice(WINQUIT);
    add_event(ANY, REDRAW, ANY, redraw_scene, NULL);
    qdevice(REDRAW);

    init_menus(); /* Add events for menu handling */

    add_update(&update_view, draw_scene, NULL);
    add_event(ANY, ANY, ANY, anything_moving, (unsigned char *)&update_view);

    anything_moving(&update_view);

    qenter(REDRAW, 1);
    update_objeulers();

    /*
     * Note:  All the add_events and add_updates that are added in ui()
     * happen before those defined above; so, for example, the code for
     * handling REDRAW inside ui.c will happen before the draw_scene
     * specified above.  This is also why the anything_moving code works.
     */
    ui(remember_view); /* Time is spent in ui, interacting */
    gexit();
    exit(0);
}

/*
 *	Called by ui interface, passed a float[4] that is a rotation
 * specified in Euler paramaters and a float[3] that is xyz
 * translation.  (no rotation is {0.0, 0.0, 0.0, 1.0}, no translation
 * is {0.0, 0.0, 0.0})
 */
void remember_view(float *rot, float *trans) {
    int i;

    if (lobj.select) /* Moving lights, too */
    {
        vcopy(rot, lobj.espin); /* vcopy copies 3 elements */
        lobj.espin[3] = rot[3]; /* So copy fourth here */

        vadd(trans, lobj.trans, lobj.trans);
    }

    for (i = 0; i < nobjs; i++) {
        if (fobj[i]->select) {
            vcopy(rot, fobj[i]->espin);
            fobj[i]->espin[3] = rot[3];

            vadd(trans, fobj[i]->trans, fobj[i]->trans);
        }
    }
    update_objeulers(); /* Spin those puppies */
    draw_objects();
}

/*
 *	This function is called whenever a REDRAW event occurs.
 */
void redraw_scene(void) {
    long xdim, ydim;

    reshapeviewport();
    getsize(&xdim, &ydim);
    aspect = (float)xdim / (float)ydim;

    perspective(750, (float)aspect, window_near, window_far);

    draw_scene();
}

/*
 *	This function is called whenever the user isn't interacting with
 * the program (when ui_quiet is TRUE).  It just keeps on applying the
 * last rotation and drawing the scene over and over.
 */
void draw_scene(void) {
    int i;

    update_objeulers();
    draw_objects();
}

/*
 *	This routine updates an object's total rotation by applying its
 * spin rotation, and coming up with a new total rotation.  Also spins
 * lights.
 */
void update_objeulers() {
    int i;
    for (i = 0; i < nobjs; i++) {
        add_quats(fobj[i]->espin, fobj[i]->er, fobj[i]->er);
    }
    add_quats(lobj.espin, lobj.er, lobj.er);
}

/*
 *	Draw the objects (and lights)
 */
void draw_objects() {
    Matrix m;
    int i;
    int polysdrawn = 0;

#ifdef HDWBUG
    /* Speedy clear */
    if (clover1)
        czclear(0, getgdesc(GD_ZMAX));
    else
#endif
        czclear(0, getgdesc(GD_ZMIN));

    /* Transform the lights */
    pushmatrix();
    translate(lobj.trans[0], lobj.trans[1], lobj.trans[2]);
    build_rotmatrix(m, lobj.er);
    multmatrix(m);
    rebind_lights();
    if (lobj.display) /* And maybe display them */
        polysdrawn += draw_lights();
    popmatrix();

    for (i = 0; i < nobjs; i++) {
        if (fobj[i]->display && !fobj[i]->ablend) {
            pushmatrix();
            setmaterial(fobj[i]->material);

            /* Translate */
            translate(fobj[i]->trans[0], fobj[i]->trans[1], fobj[i]->trans[2]);

            /* And then rotate */
            build_rotmatrix(m, fobj[i]->er);
            
            multmatrix(m);

            if (fobj[i]->type == SUBSMOOTHLINES && smoothlines_supported) {
                if (!fobj[i]->ablend)
                    blendfunction(BF_SA, BF_MSA);
                smoothline(TRUE);
                subpixel(TRUE);
            }

            if (fobj[i]->swirl != 0)
                draw_swirl(fobj[i]);
            else
                drawflipobj(fobj[i]);

            if (smoothlines_supported) {
                if (!fobj[i]->ablend)
                    blendfunction(BF_ONE, BF_ZERO);
                smoothline(FALSE);
                subpixel(FALSE);
            }

            polysdrawn += fobj[i]->npoints / 4;
            popmatrix();
        }
    }
    /* Alpha blend, if we're on a machine that can do it */
    if (blending_supported) {
        alpha_on();
        for (i = 0; i < nobjs; i++) {
            if (fobj[i]->display && fobj[i]->ablend) {
                pushmatrix();
                setmaterial(NUM_MATERIALS + fobj[i]->material);

                translate(fobj[i]->trans[0], fobj[i]->trans[1], fobj[i]->trans[2]);

                build_rotmatrix(m, fobj[i]->er);
                multmatrix(m);

                if (fobj[i]->type == SUBSMOOTHLINES) {
                    smoothline(TRUE);
                    subpixel(TRUE);
                }

                if (fobj[i]->swirl != 0)
                    draw_swirl(fobj[i]);
                else
                    drawflipobj(fobj[i]);

                smoothline(FALSE);
                subpixel(FALSE);

                polysdrawn += fobj[i]->npoints / 4;
                popmatrix();
            }
        }
        alpha_off();
    }

    if (show_performance)
        draw_rate(polysdrawn);

    /* if (gotshare == TRUE)
        testmsg(); */

    swapbuffers();
}

#include <sys/types.h>
#ifndef _WIN32
#include <sys/param.h>
#include <sys/times.h>
#endif
#define FRAMES 60 /* Update polygons/sec every FRAMES frames */
void draw_rate(n) int n;
{
    static int numdrawn = 0;
    static long lastt = 0;
    static int frames = 0;
    struct tms buf;
    Matrix tm;
    static char s[128];

    numdrawn += n;

    if (lastt == 0 && s[0] == 0) {
        sprintf(s, "%d polygons per frame", n);
        lastt = times(&buf);
    }
    if (++frames >= FRAMES) {
        long t, et;

        t = times(&buf);
        et = t - lastt;

        sprintf(s, "%d polygons/frame, %d frames/second, %d polys/sec", n, frames * HZ / et, numdrawn * HZ / et);
        frames = 0;
        lastt = t;
        numdrawn = 0;
    }

    mmode(MPROJECTION);
    getmatrix(tm);
    ortho2(0.0, 100.0, 0.0, 100.0);
    mmode(MVIEWING);
    pushmatrix();
    loadmatrix(idmat);

    cmov2i(3, 3);
    cpack(0xffffff00);
    if (getgdesc(GD_BITS_NORM_SNG_RED) != 3)
        charstr(s);

    mmode(MPROJECTION);
    loadmatrix(tm);
    mmode(MVIEWING);
    popmatrix();
}

/*
 * Initialize graphics
 */
void init_windows(title) char *title;
{
    long xorg, yorg, xdim, ydim;
    char machinetype[20];

    /*if (getgdesc(GD_BITS_NORM_SNG_RED) == 0)
    {
        system("inform 'Your system must support RGB mode to run flip'");
        exit(1);
    }
    if (getgdesc(GD_BITS_NORM_ZBUFFER) == 0)
    {
        system("inform 'Your system must have a z-buffer to run flip'");
        exit(1);
    }*/
    if (getgdesc(GD_LINESMOOTH_RGB))
        smoothlines_supported = 1;
    else
        smoothlines_supported = 0;
    if (getgdesc(GD_BLEND))
        blending_supported = 1;
    else
        blending_supported = 0;

    gversion(machinetype);
    if (strncmp(machinetype, "GL4D-", strlen("GL4DG-")) == 0)
        clover1 = 1;
    else
        clover1 = 0;

    if (UsePrefposition)
        prefposition(XOrigin, XOrigin + Width - 1, YOrigin, YOrigin + Height - 1);
    else if (UsePrefsize)
        prefsize(Width, Height);

    /* Open with the executable's name (stripped of directory) */
    {
        char *t, *strrchr(char *, int);
        /* testshare(""); */
        winopen((t = strrchr(title, '/')) != NULL ? t + 1 : title);
    }
    wintitle(title);

    reshapeviewport();
    getsize(&xdim, &ydim);
    aspect = (float)xdim / (float)ydim;

    RGBmode();
    if (getgdesc(GD_BITS_NORM_DBL_RED) != 0)
        doublebuffer();
    gconfig();

    zbuffer(TRUE);

#ifdef HDWBUG
    if (clover1) {
        /* workaround for CLOVER1 bug */
        /*		lsetdepth(getgdesc(GD_ZMIN), getgdesc(GD_ZMAX)); */
        setdepth(getgdesc(GD_ZMIN), getgdesc(GD_ZMAX));
    } else {
#endif
        /* Setup to use simultaneous z and color buffer clear */
        zfunction(ZF_GEQUAL);
        lsetdepth(getgdesc(GD_ZMAX), getgdesc(GD_ZMIN));
#ifdef HDWBUG
    }
#endif

    window_near = 0.2; /* Should be smarter about clipping planes */
    window_far = 1.1 + zmax + 3.0;
    mmode(MPROJECTION);
    perspective(750, (float)aspect, window_near, window_far);
    mmode(MVIEWING);
    loadmatrix(idmat);
    translate(0.0, 0.0, -1.1);
}

void parse_args(argc, argv) int argc;
char **argv;
{
    int whichobj = 0; /* Which one we're doing now */
    int i, c, err, still;
    float objzmax, objmaxpoint();
    extern int optind;
    extern char *optarg;

    still = FALSE; /* Spin around like crazy by default */

    /*
     * If on a machine that can't double-buffer, stay still!
     */
    if (getgdesc(GD_BITS_NORM_DBL_RED) == 0)
        still = TRUE;

    err = FALSE;
    while ((c = getopt(argc, argv, "W:sh")) != -1) {
        switch (c) {
        case 'W': /* provide x1,y1,x2,y2  or just xsize, ysize */
            if (4 == sscanf(optarg, "%d,%d,%d,%d", &XOrigin, &YOrigin, &Width, &Height)) {
                UsePrefposition = 1;
            } else if (2 == sscanf(optarg, "%d,%d", &Width, &Height)) {
                UsePrefsize = 1;
            } else {
                err = 1;
            }
            break;

        case 's': /* Still (don't move) */
            still = TRUE;
            break;
        case 'h': /* Help */
        default:
            err = TRUE;
            break;
        }
    }

    /* First pass, figure out how many arguments */
    for (i = optind; i < argc; i++) {
        if (argv[i][0] != '-')
            ++nobjs;
    }

    /*if (err || nobjs==0 )
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "%s [-sh] [-W[xorg,yorg,]width,height] modelname [modelname]\n",argv[0]);
        fprintf(stderr, "\t-s Means stay still, objects won't get random rotation\n");
        fprintf(stderr, "\t-h Help (this message)\n");
        exit(1);
    }*/
    nobjs = 1; /* TEMPORARY HACK TO AVOID ARG PROBLEMS */

    fobj = (flipobj **)malloc(sizeof(flipobj *) * nobjs);
	optind = 0; /* TEMPORARY HACK TO AVOID ARG PROBLEMS */
    for (i = optind; i < 1; i++) {
        char temp[64];
        FILE *fp;
        int j, loaded;

        /*
         *	First check to see if this object has already been loaded
         */
        loaded = (-1);
        /*for (j = optind; j < i; j++) {
            if (strcmp(argv[j], argv[i]) == 0) {
                loaded = j - optind;
            }
        }*/
        if (loaded == (-1)) {
            /*
             * Ok, try to read in first from current directory,
             *	then from the demos directory
             */
            if ((fp = fopen("x29.bin", "r")) != NULL) {
                fclose(fp);
                fobj[whichobj] = readflipobj("x29.bin");
            } else {
                strcpy(temp, MODELDIR);
                strcat(temp, "x29.bin");
                fobj[whichobj] = readflipobj(temp);
            }
            if (fobj[whichobj] == NULL) {
                sprintf(temp, "%s, error reading %s:", argv[0], argv[i]);
                perror(temp);
                exit(1);
            }
            {
                char *t, *strrchr(char *, int);
                t = strrchr("x29.bin", '/');
                t = (t == NULL) ? argv[i] : t + 1;
                fobj[whichobj]->fname = t;
            }
        } else { /* Share data with previous instance */
            fobj[whichobj] = (flipobj *)malloc(sizeof(flipobj));
            memcpy(fobj[whichobj], fobj[loaded], sizeof(flipobj));
        }
        /* Set-up defaults */
        fobj[whichobj]->display = TRUE;
        fobj[whichobj]->type = POLYGONS; /* draw polygons */
        fobj[whichobj]->select = FALSE;
        fobj[whichobj]->material = 2;
        fobj[whichobj]->ablend = FALSE;
        fobj[whichobj]->swirl = 0;
        vzero(fobj[whichobj]->trans);

        vzero(fobj[whichobj]->er);
        fobj[whichobj]->er[3] = 1.0;
        if (still) {
            vzero(fobj[whichobj]->espin);
            fobj[whichobj]->espin[3] = 1.0;
        } else {
            rand_rotation(fobj[whichobj]->espin);
        }

        totalpolys += (fobj[whichobj]->npoints) / 4;
        if ((objzmax = objmaxpoint(fobj[whichobj])) > zmax)
            zmax = objzmax;
        ++whichobj;
    }
    fobj[0]->select = TRUE;

    /* Initialize lights */
    lobj.select = FALSE; /* Don't rotate by default */
    lobj.display = TRUE; /* But do display */
    vzero(lobj.er);      /* Start with no rotation */
    lobj.er[3] = 1.0;
    vzero(lobj.espin); /* no spin */
    lobj.espin[3] = 1.0;
    vzero(lobj.trans); /* and no translation */
}
void rand_rotation(e) float *e;
{
    static int init = 0;
    int i;
    float a[3]; /* Pick a random axis to rotate about */
    float phi;  /* And a speed of rotation */
    extern double drand48(void);
    extern void srand48(long);

    if (!init) {
        srand48(getpid());
        init = 1;
    }
    for (i = 0; i < 3; i++)
        a[i] = drand48() - 0.5;

    /* Un-comment this out to get random rotation speeds
     *	phi = drand48() * .3;
     */
    phi = 0.1;

    /* Now figure out Euler paramaters for given axis */
    axis_to_euler(a, phi, e);
}

static int lights[NUM_LIGHTS];

/*
 * Define the lighting model, lights, and initial material
 */
make_lights() {
    int i;
    for (i = 0; i < NUM_LIGHTS; i++)
        lights[i] = 0; /* Off */

    defineshading(); /* lmdef everything used */

    setmodel(0); /* Use infinite viewer */

    switch_light(0); /* Infinite White on */
    lights[0] = 1;
    switch_light(2); /* Infinite Yellow on */
    lights[2] = 1;
}

static long flipmenu;  /* Top-level menu */
static long lightmenu; /* Lights */
static long lmmenu;    /* Light models submenu */
static long lsmenu;    /* Light sources submenu */
static long *objmenus; /* Menu for each object */
static long *mmenus;   /* Materials for each object */
static long *dtmenus;  /* Draw types for each object */

void init_menus() {
    int i;
    char temp[64];

    lsmenu = newpup();
    remake_lsmenu();
    lmmenu = newpup();
    remake_lmmenu(0);
    lightmenu = newpup();
    remake_lightmenu();

    objmenus = (long *)malloc(sizeof(long) * nobjs);
    dtmenus = (long *)malloc(sizeof(long) * nobjs);
    mmenus = (long *)malloc(sizeof(long) * nobjs);
    for (i = 0; i < nobjs; i++) {
        mmenus[i] = newpup();
        remake_mmenu(i);
        dtmenus[i] = newpup();
        remake_dtmenu(i);
        objmenus[i] = newpup();
        remake_objmenu(i);
    }
    flipmenu = newpup();

    remake_menus();

    qdevice(RIGHTMOUSE);
    add_event(ANY, RIGHTMOUSE, DOWN, do_menus, NULL);
}
void do_menus() {
    dopup(flipmenu);
    draw_scene();
}

/*
 *	Scheme for re-making minumum number of menus:
 * don't remake the whole structure, just what could have possibly
 * changed (the lowest-level object menus).  Assumes all lowest-level
 * object menus are already made.
 */
void remake_menus(void) {
    int i, j;
    char temp[256];

    freepup(flipmenu);
    flipmenu = newpup();
    addtopup(flipmenu, "Flip %t");
    addtopup(flipmenu, "Lights %m", lightmenu);
    for (i = 0; i < nobjs; i++) {
        sprintf(temp, "%s %%m", fobj[i]->fname);
        addtopup(flipmenu, temp, objmenus[i]);
    }
    if (nobjs > 1)
        addtopup(flipmenu, "Select all %f", select_all);
    if (show_performance)
        addtopup(flipmenu, "No Performance Numbers %f", toggle_performance);
    else
        addtopup(flipmenu, "Show Performance Numbers %f", toggle_performance);
    addtopup(flipmenu, "Exit %f", ui_exit);
}

void remake_lightmenu(void) {
    freepup(lightmenu);
    lightmenu = newpup();
    addtopup(lightmenu, "Sources %m", lsmenu);
    addtopup(lightmenu, "Lighting Models %m", lmmenu);
    if (lobj.select == TRUE)
        addtopup(lightmenu, "Deselect lights %f", toggle_spinlights);
    else
        addtopup(lightmenu, "Select lights %f", toggle_spinlights);
    if (lobj.display == TRUE)
        addtopup(lightmenu, "Hide local lights %f", toggle_displights);
    else
        addtopup(lightmenu, "Show local lights %f", toggle_displights);
}

void remake_mmenu(int n) {
    int i, j, start;
    char temp[64];
    start = n * NUM_MATERIALS;

    freepup(mmenus[n]);
    mmenus[n] = newpup();
    addtopup(mmenus[n], "Material Properties %t %F", do_objmaterials);
    for (j = 0; j < NUM_MATERIALS; j++) {
        sprintf(temp, "%s %%x%d", matnames[j], start + j);
        addtopup(mmenus[n], temp);
        setpup(mmenus[n], j + 1, (fobj[n]->material == j) ? PUP_CHECK : PUP_BOX);
    }
}

void remake_dtmenu(int n) {
    char temp[64];
    int i;

    i = n * (int)NUM_DrawTypes;

    freepup(dtmenus[n]);
    dtmenus[n] = newpup();
    addtopup(dtmenus[n], "Drawing Modes %t %F", toggle_drawtype);
    sprintf(temp, "Polygons %%x%d", i + (int)POLYGONS);
    addtopup(dtmenus[n], temp);
    setpup(dtmenus[n], 1, (fobj[n]->type == POLYGONS) ? PUP_CHECK : PUP_BOX);
    sprintf(temp, "Lines %%x%d", i + (int)LINES);
    addtopup(dtmenus[n], temp);
    setpup(dtmenus[n], 2, (fobj[n]->type == LINES) ? PUP_CHECK : PUP_BOX);
    if (smoothlines_supported) {
        sprintf(temp, "Subpixel, antialised lines %%x%d", i + (int)SUBSMOOTHLINES);
        addtopup(dtmenus[n], temp);
        setpup(dtmenus[n], 3, (fobj[n]->type == SUBSMOOTHLINES) ? PUP_CHECK : PUP_BOX);
    }
}

void remake_objmenu(n) int n;
{
    char temp[64];

    freepup(objmenus[n]);
    objmenus[n] = newpup();

    if (fobj[n]->display == TRUE)
        sprintf(temp, "Hide %%f %%x%d", n);
    else
        sprintf(temp, "Show %%f %%x%d", n);
    addtopup(objmenus[n], temp, toggle_display);

    if (fobj[n]->select == TRUE)
        sprintf(temp, "Deselect %%f %%x%d", n);
    else
        sprintf(temp, "Select %%f %%x%d", n);
    addtopup(objmenus[n], temp, toggle_select);

    addtopup(objmenus[n], "Object Materials %m", mmenus[n]);

    if (blending_supported) {
        if (fobj[n]->ablend == TRUE)
            sprintf(temp, "Make Opaque %%f %%x%d", n);
        else
            sprintf(temp, "Make Transparent %%f %%x%d", n);
        addtopup(objmenus[n], temp, toggle_alpha);
    }

    addtopup(objmenus[n], "Display Object as... %m", dtmenus[n]);

    if (fobj[n]->swirl == 0)
        sprintf(temp, "Swirl me %%f %%x%d", n);
    else
        sprintf(temp, "Stop swirling %%f %%x%d", n);
    addtopup(objmenus[n], temp, toggle_swirl);
}

void toggle_performance(void) {
    show_performance = !show_performance;

    remake_menus();
}

void do_objmaterials(int n) {
    int obj, m;

    obj = n / NUM_MATERIALS;
    m = n % NUM_MATERIALS;

    fobj[obj]->material = m;
    remake_mmenu(obj);
    remake_objmenu(obj);
    remake_menus();
}

void toggle_spinlights(void) {
    lobj.select = !lobj.select;

    remake_lightmenu();
    remake_menus();
}
void toggle_display(n) int n;
{
    fobj[n]->display = !fobj[n]->display;

    remake_objmenu(n);
    remake_menus();
}
void toggle_drawtype(n) int n;
{
    int obj;
    enum DrawType dt;

    obj = n / (int)NUM_DrawTypes;
    dt = (enum DrawType)(n % (int)NUM_DrawTypes);

    fobj[obj]->type = dt;

    remake_dtmenu(obj);
    remake_objmenu(obj);
    remake_menus();
}
void toggle_displights(void) {
    lobj.display = !lobj.display;

    remake_lightmenu();
    remake_menus();
}
void toggle_alpha(n) int n;
{
    fobj[n]->ablend = !fobj[n]->ablend;

    remake_objmenu(n);
    remake_menus();
}
void toggle_swirl(n) int n;
{
    fobj[n]->swirl = !fobj[n]->swirl;

    remake_objmenu(n);
    remake_menus();
}

void select_all() {
    int i;
    for (i = 0; i < nobjs; i++) {
        if (fobj[i]->select == FALSE) {
            fobj[i]->select = TRUE;
            remake_objmenu(i);
        }
    }
    remake_menus();
}

void toggle_select(n) int n;
{
    int t;

    fobj[n]->select = !fobj[n]->select;

    remake_objmenu(n);
    remake_menus();
}

void toggle_lightsource(int ls) {
    switch_light(ls);
    lights[ls] = !lights[ls];

    remake_lsmenu();
    remake_lightmenu();
    remake_menus();
}

void remake_lsmenu(void) {
    int i;
    char temp[64];

    freepup(lsmenu);
    lsmenu = newpup();
    addtopup(lsmenu, "Light Sources %t %F", toggle_lightsource);
    for (i = 0; i < NUM_LIGHTS; i++) {
        sprintf(temp, "%s %%x%d", lightnames[i], i);
        addtopup(lsmenu, temp);
        setpup(lsmenu, i + 1, lights[i] ? PUP_CHECK : PUP_BOX);
    }
}

void select_lmodel(int n) {
    setmodel(n);
    remake_lmmenu(n);
    remake_lightmenu();
    remake_menus();
}

void remake_lmmenu(int n) {
    int i;
    char temp[64];

    freepup(lmmenu);
    lmmenu = newpup();
    addtopup(lmmenu, "Light Models %t %F", select_lmodel);
    for (i = 0; i < NUM_LMODELS; i++) {
        sprintf(temp, "%s %%x%d", lmodelnames[i], i);
        addtopup(lmmenu, temp, select_lmodel);
        setpup(lmmenu, i + 1, (i == n ? PUP_CHECK : PUP_BOX));
    }
}

void alpha_on() {
    blendfunction(BF_SA, BF_MSA);
    zwritemask(0x0);
}

void alpha_off() {
    blendfunction(BF_ONE, BF_ZERO);
    zwritemask(0xffffff);
}

/*
 * This function figures out if anything is moving.  If it is, it
 * returns TRUE in ptr; if not, FALSE
 */
void anything_moving(int *ptr) {
    int spinning(float *);

    *ptr = FALSE;

    if (ui_quiet) {
        int i;

        /* If lights are spinning... */
        if (spinning(lobj.espin))
            *ptr = TRUE;
        /* Or objects are spinning or swirling, redraw */
        else
            for (i = 0; i < nobjs; i++) {
                if (fobj[i]->display && (fobj[i]->swirl || spinning(fobj[i]->espin))) {
                    *ptr = TRUE;
                }
            }
    }
}

static int spinning(float *r) {
    float sum;

    sum = r[0] + r[1] + r[2] + r[3];
    if (sum > 0.999 && sum < 1.001)
        return FALSE;
    else
        return TRUE;
}
