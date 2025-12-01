/*
 * Copyright (C) 1992, 1993, 1994, Silicon Graphics, Inc.
 * Modified for GLUT compatibility
 */
#include "space.h"

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define getcwd _getcwd
#endif
#include <GL/glut.h>
static uint16 cursor[16] = {0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0xFFFE,
                            0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0000};

static flot32 blurray[3][3] = {
    0.05, 0.10, 0.05, 0.10, 0.80, 0.10, 0.05, 0.10, 0.05,
};

char datapath[256];
V4 raww[NUM_CLIP_PLANES];
t_boss flaggs;
t_galaga ggaa[STARSQ][STARSQ];
extern t_stopwatch Counter;
long control_height;

static char *space_assist[] = {" COMMAND LINE ARGS", "   -h       : help", "   -f path  : set data directory path", "",
                               0};

static char *space_help[] = {" MOUSE CONTROLS",
                             "    left   : accelerates",
                             "    middle : brakes",
                             "    right  : pan/rotate toggle",
                             "",
                             "    left+right        : face nearest moon",
                             "    middle+right      : face nearest planet",
                             "    left+middle       : face nearest star",
                             "    left+middle+right : face companion star",
                             "",
                             " KEY CONTROLS",
                             "    Esc   : quit",
                             "    Shift : defeat mouse controls (for menu)",
                             "    Ctrl  : disconnect view vector from velocity vector",
                             "    a : autopilot",
                             "    b : single buffer mode (left mouse is trigger)",
                             "    h : on screen help toggle",
                             "    i : moons/orbits toggle",
                             "    l : reshade polyplanet every frame",
                             "    n : star name toggle",
                             "    o : planets/orbits toggle",
                             "    q : control panel toggle",
                             "    r : decelerates time by 10",
                             "    s : stellar system statistics",
                             "    t : accelerates time by 10",
                             "    u : user entry mode (use with system statistics)",
                             "    v : reverse velocity",
                             "    x : text display toggle",
                             "    y : time reset",
                             "    z : display constellation boundaries",
                             "    - : time direction toggle",
                             "    up   arrow : increase tesselation level (max: 5)",
                             "    down arrow : decrease tesselation level (min: 0)",
                             "    PrintScrn  : snap rgb image and quit",
                             "",
                             " TEXT COLOR CODES",
                             "    red   : in interplanetary space",
                             "    blue  : in interstellar space",
                             "    yellow: in intergalactic space",
                             "    green : currently being eclipsed",
                             "",
                             " TOURIST SPOTS",
                             "    Sol   :    0",
                             "    Sirius:  714",
                             "    Mizar : 1421",
                             0};

static char *opening_credit[] = {
    "SGI Space Simulator Version 2.00", "", "", "A simulation of the Solar System and the Milky Way galaxy",
    "with certain artistic liberties.", 0};

static char *geosphere_credit[] = {"Satellite Earth color data provided by",
                                   "TOM VAN SANT and the GEOSPHERE PROJECT",
                                   "With assistance from NOAA, NASA, EYES ON EARTH",
                                   "Technical Direction Lloyd van Warren, Leo Blume, Jim Knighton",
                                   "Source data derived from NOAA/TIROS-N Series Satellites",
                                   "All rights reserved by Tom Van Sant, Inc.",
                                   "146 Entrada Drive, Santa Monica, CA 90402",
                                   "310-459-4342",
                                   0};

static char *sub_geosphere_credit[] = {"SATELLITE EARTH COLOR DATA", "Tom Van Sant/The Geosphere Project",
                                       "L.V. Warren, L. Blume, J. Knighton", "Santa Monica, CA  310-459-4342", 0};

int GeosphereData = 0;

static void initialize_graphics(void);
static void fly(void);
static void print_screen_text(flot32);
static void blur_galaxy(uint32[256][256]);
static void print_opening_credit(void);

/* GLUT display callback */
static void display_callback(void);
/* GLUT idle callback */
static void idle_callback(void);

/* Frame counter for FPS calculation */
static sint32 clocker = 20;
static flot32 timer, timer_old;

/**********************************************************************/
/*  MAIN ()                                                           */
/**********************************************************************/
int main(sint32 argc, char *argv[])

{
    char *token;
    sint32 i;

    Counter.flags = PRBIT_FLAG | MRBIT_FLAG | FLSCR_FLAG;

    while (--argc > 0 && **++argv == '-')
        for (token = argv[0] + 1; *token; token++)
            switch (*token) {
            case 'd':
                Counter.flags &= ~FLSCR_FLAG;
                break;
            case 'f':
                Counter.flags |= DPATH_FLAG;
                argc--;
                strcpy(datapath, *++argv);
                strcat(datapath, "/");
                break;
            case 'h':
                for (i = 0; space_assist[i] != NULL; i++)
                    fprintf(stdout, "\t%s\n", space_assist[i]);
                for (i = 0; space_help[i] != NULL; i++)
                    fprintf(stdout, "\t%s\n", space_help[i]);
                exit(0);
                break;
            case 's':
                Counter.flags |= SPACB_FLAG;
                break;
            default:
                printf("Invalid Option: %d\n", *token);
                exit(0);
                break;
            }

    spIdentifyMachine();

    if (!spOpenWindow())
        exit(-1);

    spQuantifyMachine();

    if (!spInitFont("-adobe-times-bold-r-normal--0-150-100-100-p-100-iso8859-1")) {
        /* Font initialization failure is non-fatal with GLUT */
        printf("Using default GLUT font\n");
    }

    initialize_graphics();
    spInitCursor();
    spInitBell();
    spInitKeyboard();
    initialize_time();
    initialize_shmem(&flaggs);

    key_press(&flaggs, SP_IO_q);

    /* Initialize timer variables for FPS calculation */
    timer_old = Counter.timer_old;
    timer = Counter.timer;
    Counter.fps = 20.0;
    clocker = 20;

    /* Set up GLUT callbacks */
    glutDisplayFunc(display_callback);
    glutIdleFunc(idle_callback);

    /* Start GLUT main loop */
    glutMainLoop();

    return 0;
}

/**********************************************************************/
/*  initialize_graphics()                                             */
/**********************************************************************/
static void initialize_graphics(void)

{
    flot32 tevps[8], texps[8], att[4];
    sint32 i, j, k, m;
    int arr[8];
    uint32 txt[32 * 32], qwe[256][256];
    uint16 stt[128 * 128];
    char *p;
    FILE *fd;

    set_window_view(300.0);

    glDepthFunc(GL_LEQUAL);
    glClearDepth(1.0);
    glShadeModel(GL_FLAT);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    print_opening_credit();
    spSwapBuffers();

    spTevDef();

    if (Counter.alpha & HW_AAPNT)
        glEnable(GL_POINT_SMOOTH);

    if (Counter.alpha & HW_AALIN)
        glEnable(GL_LINE_SMOOTH);

    spInitLight();

    Counter.sky_clear_color[0] = 0.0;
    Counter.sky_clear_color[1] = 0.0;
    Counter.sky_clear_color[2] = 0.0;
    Counter.sky_clear_color[3] = 0.0;

    for (p = (char *)txt, i = 0; i < 32; i++)
        for (j = 0; j < 32; j++) {
            if (((i & 8) + (j & 8)) & 8)
                *p++ = 0x80;
            else
                *p++ = 0xff;
        }
    flaggs.stat.texdf = spTexDef(1, 32, 32, txt, 1);

    flaggs.stat.cliprad = 150.0;
    flaggs.stat.bodyobj[0] = object_space_station(12);
    flaggs.stat.bodyobj[1] = object_space_station(8);
    flaggs.stat.bodyobj[2] = object_space_station(4);
    Counter.shadowsqobj = object_shadow_squares();

#if 0
   Yread();
#endif

    if ((fd = fopen(datatrail("corona.ring"), "r"))) {
        fread(stt, 2, 128 * 128, fd);
        fclose(fd);
        Counter.corona = spTexDef(2, 128, 128, stt, 0);
    } else {
        printf("USER ERROR: Texture file not found: %s\n", datatrail("corona.ring"));
        exit(0);
    }

    if ((fd = fopen(datatrail("galaxy.texture"), "r"))) {
        fread(qwe, 4, 256 * 256, fd);
        fclose(fd);
        Counter.galaxy = spTexDef(4, 256, 256, qwe, 0);
    } else {
        printf("USER ERROR: Texture file not found: %s\n", datatrail("galaxy.texture"));
        exit(0);
    }

    blur_galaxy(qwe);
}

/**********************************************************************
 *  GLUT idle callback - main render loop
 **********************************************************************/
static void idle_callback(void) {
    /* Update state */
    read_time();
    scan_galactic_system(&flaggs);
    spReadEvents(&flaggs);
    evaluate_mouse(&flaggs);

    /* Redraw */
    glutPostRedisplay();

    /* FPS calculation */
    if (--clocker <= 0) {
        timer_old = timer;
        timer = Counter.timer;
        Counter.fps = ((Counter.fps < 1.0) ? 1.0 : (sint32)Counter.fps) / (timer - timer_old);
        clocker = Counter.fps;
    }
}

/**********************************************************************
 *  GLUT display callback
 **********************************************************************/
static void display_callback(void) {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth(1.0);

    switch (Counter.status) {
    case STELL_STAT:
        if ((flaggs.plan_current < 0) || (Counter.flags & PRBIT_FLAG)) {
            glClear(GL_COLOR_BUFFER_BIT);
        } else {
            glClearColor(Counter.sky_clear_color[0], Counter.sky_clear_color[1], Counter.sky_clear_color[2],
                         Counter.sky_clear_color[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        break;

    case GALAC_STAT:
        glClear(GL_COLOR_BUFFER_BIT);
        break;

    case COSMC_STAT:
        if (Counter.flags & TEXTR_FLAG)
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        else if (Counter.flags & SLOWZ_FLAG)
            glClear(GL_COLOR_BUFFER_BIT);
        else
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        break;
    }

    Counter.sky_clear_color[0] = 0.0;
    Counter.sky_clear_color[1] = 0.0;
    Counter.sky_clear_color[2] = 0.0;
    Counter.sky_clear_color[3] = 0.0;

    if (Counter.flags & ACCUM_FLAG) {
        accumulation(&flaggs);
        Counter.flags &= ~ACCUM_FLAG;
    } else {
        actually_do_graphics(&flaggs);
    }

    if (Counter.flags & HELPP_FLAG || !(Counter.flags & NOTXT_FLAG) || (Counter.flags & GEOSP_FLAG))
        print_screen_text(Counter.fps);

    if (Counter.flags & PRINT_FLAG) {
        savescreen(Counter.winsizex, Counter.winsizey);
        exit(0);
    }

    spSwapBuffers();
}

/**********************************************************************
 *  print_screen_text()  -
 **********************************************************************/
static void print_screen_text(flot32 fps)

{
    flot32 vel;
    sint32 i;
    schar8 sped[32], fpsc[32];
    uchar8 cl[3];

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (!(Counter.flags & NOTXT_FLAG)) {
        cl[0] = (Counter.infoco >> 0) & 0xff;
        cl[1] = (Counter.infoco >> 8) & 0xff;
        cl[2] = (Counter.infoco >> 16) & 0xff;
        glColor3ubv(cl);

        spDrawString(0.01, 0.97, 0.0, Counter.date);

        vel = Counter.S / LIGHTSPEED;

        if (Counter.status != STELL_STAT)
            vel *= PRTOKM;

        if (Counter.flags & AUTOP_FLAG)
            sprintf(sped, "Auto Pilot\n");
        else {
            if (vel >= 1.0) {
                vel = 1.0 + flog10(vel);
                sprintf(sped, "Speed: %.2fw\n", vel);
            } else if (vel >= 0.01)
                sprintf(sped, "Speed: %.3fc\n", vel);
            else
                sprintf(sped, "Speed: %.2fkm/s\n", vel * LIGHTSPEED);
        }

        spDrawString(0.01, 0.94, 0.0, sped);

        sprintf(fpsc, "F/sec: %.2f\n", fps);
        spDrawString(0.01, 0.91, 0.0, fpsc);

        if (Counter.flags & USERM_FLAG) {
            sprintf(fpsc, "User Entry Mode\n");
            spDrawString(0.01, 0.88, 0.0, fpsc);
        }
    }

    if (Counter.flags & HELPP_FLAG) {
        glColor4f(0.5, 0.75, 0.5, 1.0);

        for (vel = 0.95, i = 0; space_help[i] != NULL; vel -= 0.02, i++)
            spDrawString(0.25, vel, 0.0, space_help[i]);
    }

    if (Counter.flags & GEOSP_FLAG) {
        glColor4ubv((unsigned char *)&Counter.infoco);
        for (vel = 0.97, i = 0; sub_geosphere_credit[i] != NULL; vel -= 0.025, i++)
            spDrawString(0.70, vel, 0.0, sub_geosphere_credit[i]);
    }
}

/**********************************************************************
 *  blur_galaxy()  -
 **********************************************************************/
static void blur_galaxy(uint32 qwe[256][256])

{
    flot32 *a, sum, ar[STARSQ][STARSQ], br[STARSQ][STARSQ], biggest;
    sint32 i, j, k, m, delta;
    uint32 psum, col;
    t_galaga *gg;

    delta = 256 / STARSQ;

    for (i = 0; i < 256; i += delta)
        for (j = 0; j < 256; j += delta) {
            for (psum = 0, k = i; k < i + delta; k++)
                for (m = j; m < j + delta; m++) {
                    col = qwe[k][m];
                    psum += (col & 0xff) + ((col >> 8) & 0xff) + ((col >> 16) & 0xff);
                }
            ar[i / delta][j / delta] = psum;
        }

    for (i = 0; i < STARSQ; i++) {
        br[i][0] = 0.0;
        br[i][STARSQ - 1] = 0.0;
        br[0][i] = 0.0;
        br[STARSQ - 1][i] = 0.0;
    }

    for (biggest = 0.0, i = 1; i < STARSQ - 1; i++)
        for (j = 1; j < STARSQ - 1; j++) {
            for (sum = k = 0; k < 3; k++)
                for (m = 0; m < 3; m++)
                    sum += blurray[k][m] * ar[i + k - 1][j + m - 1];

            br[i][j] = sum;

            if (sum > biggest)
                biggest = sum;
        }

    biggest = 1024.0 * fsqrt(1.0 / biggest);

    for (gg = ggaa[0], i = 0; i < STARSQ; i++)
        for (j = 0; j < STARSQ; gg++, j++) {
            gg->count = (flot32)biggest * fsqrt(br[i][j]);
            gg->stars = NULL;
        }
}

/**********************************************************************
 *  make_galaxy_object()  -
 **********************************************************************/
sint32 make_galaxy_object(sint32 flag)

{
    sint32 obj, i, j, col, step;
    T2 t[2];
    P3 p[2];
    flot32 delt, edge, q;

    obj = glGenLists(1);
    glNewList(obj, GL_COMPILE);

    if (flag) {
        delt = 1.0 / 16.0;
        edge = GALAXY_EDGE / 16.0;
        step = STARSQ / 16;
        i = STARSQ;
    } else {
        delt = 1.0 / 32.0;
        edge = GALAXY_EDGE / 32.0;
        step = STARSQ / 32;
        i = STARSQ - step - 1;
    }

    t[0].t = 0.0;
    t[1].t = delt;
    p[0].z = -0.5 * GALAXY_EDGE;
    p[1].z = p[0].z + edge;

    for (; i >= 0; i -= step) {
        glBegin(GL_TRIANGLE_STRIP);

        t[0].s = 0.0;
        t[1].s = 0.0;
        p[0].x = -0.5 * GALAXY_EDGE;
        p[1].x = -0.5 * GALAXY_EDGE;

        j = (flag ? STARSQ : STARSQ - step - 1);
        for (; j >= 0; j -= step) {
            if (flag)
                glTexCoord2fv(&t[1].s);
            else {
                col = ggaa[i][j].count * 255.0 / Counter.stars_per_square;
                if (col > 255.0)
                    col = 255.0;
                glColor4ub(col, col, col, 255);
            }

            p[1].y = edge * exp(-40.0 * ((t[1].s - 0.5) * (t[1].s - 0.5) + (t[1].t - 0.5) * (t[1].t - 0.5)));
            glVertex3fv(&p[1].x);

            if (flag)
                glTexCoord2fv(&t[0].s);
            else {
                col = ggaa[i + step][j].count * 255.0 / Counter.stars_per_square;
                if (col > 255.0)
                    col = 255.0;
                glColor4ub(col, col, col, 255);
            }

            p[0].y = edge * exp(-40.0 * ((t[0].s - 0.5) * (t[0].s - 0.5) + (t[0].t - 0.5) * (t[0].t - 0.5)));
            glVertex3fv(&p[0].x);

            t[0].s += delt;
            t[1].s += delt;
            p[0].x += edge;
            p[1].x += edge;
        }

        glEnd();

        t[0].t += delt;
        t[1].t += delt;
        p[0].z += edge;
        p[1].z += edge;
    }

    glEndList();
    return (obj);
}

/**********************************************************************/
/*  set_window_view()                                                 */
/**********************************************************************/
void set_window_view(flot32 vian)

{
    flot32 s, c, h_view_angle, v_view_angle;

    spGetWindowGeometry();

    control_height = (192 * Counter.winsizey) / 1024;
    Counter.viewangle = vian;

    if (Counter.flags & PANEL_FLAG)
        Counter.aspcratio = (flot32)Counter.winsizex / (flot32)(Counter.winsizey - control_height);
    else
        Counter.aspcratio = (flot32)Counter.winsizex / (flot32)Counter.winsizey;

    Counter.fov = 1.0 / ftan(Counter.viewangle * M_PI / 3600.0);

    v_view_angle = Counter.viewangle * M_PI / 1800.0;
    h_view_angle = v_view_angle * Counter.aspcratio;

    c = fcos(0.5 * h_view_angle);
    s = fsin(0.5 * h_view_angle);

    raww[0].x = c;
    raww[0].y = 0.0;
    raww[0].z = -s;
    raww[0].w = 0.0;

    raww[2].x = -c;
    raww[2].y = 0.0;
    raww[2].z = -s;
    raww[2].w = 0.0;

    c = fcos(0.5 * v_view_angle);
    s = fsin(0.5 * v_view_angle);

    raww[1].x = 0.0;
    raww[1].y = c;
    raww[1].z = -s;
    raww[1].w = 0.0;

    raww[3].x = 0.0;
    raww[3].y = -c;
    raww[3].z = -s;
    raww[3].w = 0.0;

    raww[4].x = 0.0;
    raww[4].y = 0.0;
    raww[4].z = -1.0;
    raww[4].w = 0.0;
}

/**********************************************************************/
/*  print_opening_credit()                                            */
/**********************************************************************/
static void print_opening_credit(void)

{
    flot32 vel;
    sint32 i;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(1.0, 1.0, 1.0, 1.0);

    for (vel = 0.80, i = 0; opening_credit[i] != NULL; vel -= 0.02, i++)
        spDrawString(0.30, vel, 0.0, opening_credit[i]);

    if (GeosphereData)
        for (vel = 0.40, i = 0; geosphere_credit[i] != NULL; vel -= 0.02, i++)
            spDrawString(0.30, vel, 0.0, geosphere_credit[i]);
}

/**********************************************************************/
/*  datatrail()  -                                                    */
/**********************************************************************/
char *datatrail(char *name)

{
    static char filename[256];

    if (Counter.flags & DPATH_FLAG) {
        strcpy(filename, datapath);
        strcat(filename, name);
    } else {
#ifdef _WIN32
        /* On Windows, use current directory */
        GetCurrentDirectory(256, filename);
        strcat(filename, "\\data\\");
#else
        getcwd(filename, 256);
        strcat(filename, "/data/");
#endif
        strcat(filename, name);
    }

    return (filename);
}
