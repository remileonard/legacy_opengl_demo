
#include "porting/iris2ogl.h"
#include "cycles.h"
#include "sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

fmfonthandle times_bold, bigfont, mediumfont, smallfont;
extern CYCLE *good, bike[CYCLES];
extern int solo, in_win, robot[CYCLES], trail_col[COLOURS];
extern int audio, demo_mode;
extern Matrix idmat;
extern clock_t last_sent;

/* local prototypes */
void plan_o_bike(void);
void shaded_bg(void);
void big_title_author(void);
void small_title_author(void);
short get_char(short c, char *, int);
void draw_text_port_thing(char *, char *);
void draw_title_screen(int);
void draw_score_screen(int, int, int);
void draw_buttons(int, int);
int over_area(void);
void draw_tic(void);

/* robin's wish is for a faster font library... or a faster 4D20 */

void init_fonts(void) {
    fminit();
    if (!(times_bold = fmfindfont("Times-Bold"))) {
        printf("couldn't find font family: Times-Bold\n");
        exit(1);
    }
}

void scale_fonts_to_win(void) {
    long x, y;
    static int last_y = 0;

    getsize(&x, &y);
    if (last_y != y) {
        bigfont = fmscalefont(GLUT_STROKE_ROMAN, 0.32 * y);
        mediumfont = fmscalefont(GLUT_STROKE_ROMAN, 0.18 * y);
        smallfont = fmscalefont(GLUT_STROKE_ROMAN, 0.09 * y);
        last_y = y;
    }
}

void shaded_bg(void) {
    float v[3];

    bgnpolygon();
    v[2] = -2.5;
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(BLUE);
#endif
    v[0] = -2.0;
    v[1] = 1.0;
    v3f(v);
#ifdef RGB_MODE
    cpack(0x0000ff);
#endif
    v[0] = 2.0;
    v[1] = 1.0;
    v3f(v);
#ifdef RGB_MODE
    cpack(0xff0000);
#endif
    v[0] = 2.0;
    v[1] = -1.0;
    v3f(v);
#ifdef RGB_MODE
    cpack(0x00ff00);
#endif
    v[0] = -2.0;
    v[1] = -1.0;
    v3f(v);
    endpolygon();
}

void big_title_author(void) {
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    cmov(-0.8, -0.5, -0.5);
    fmsetfont(bigfont);
    fmprstr("Cycles");

#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    fmsetfont(smallfont);
    cmov(-1.3, -0.75, -0.5);
    fmprstr("by Robin Humble, Alan Lipton and Nick Fitton");
}

void small_title_author(void) {
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    cmov(0.6, -0.3, -0.5);
    fmsetfont(mediumfont);
    fmprstr("Cycles");

#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    fmsetfont(smallfont);
    cmov(0.1, -0.5, -0.5);
    fmprstr("by Robin Humble, Alan Lipton");
    cmov(0.6, -0.65, -0.5);
    fmprstr("and Nick Fitton");
}

/*
 * draw plan view style thing of bike
 */
void plan_o_bike(void) {
    pushmatrix();

    translate(0.0, 0.0, -10.0);

    pushmatrix();
    rot(90.0, 'y');
    /* these are: id, rot, colour, lo_res, robot, robot flag colour */
    draw_cycle(0, 0.0, 5, 0, 0, 0);
    popmatrix();

    pushmatrix();
    translate(3.0, 0.0, 0.0);
    rot(210.0, 'y');
    draw_cycle(0, 0.0, 4, 0, 0, 0);
    popmatrix();

    popmatrix();
}

/*
 * more or less from 4Dgifts prompt.c:
 * get a char from the keyboard and add it to the name string
 */
short get_char(short c, char *name, int maxlen) {
    int ptr;

    ptr = strlen(name);
    if (ptr == 0)
        name[0] = '\0';

    switch (c) {
    case '\027': /* ^W or ^U sets cursor back to start */
    case '\025':
        name[0] = '\0';
        break;
    case '\n':
    case '\r':
        return (c);
    case '\b':
        if (ptr)
            name[--ptr] = '\0';
        break;
    default:
        if (ptr < (maxlen - 1)) {
            name[ptr++] = c;
            name[ptr] = '\0';
        }
        break;
    }
    return (c);
}

void draw_text_port_thing(char *prompt, char *data) {
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    pushmatrix();
    translate(0.0, 0.0, -0.5);
    //rectf(-1.9, -0.95, 0.0, -0.75);
#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    linewidth(2);
    rect(-1.9, -0.95, 0.0, -0.75);
    linewidth(1);
    popmatrix();

#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    fmsetfont(smallfont);
    cmov(-1.8, -0.88, -0.5);
    fmprstr(prompt);
#ifdef RGB_MODE
    cpack(0xff0000);
#else
    color(BLUE);
#endif
    cmov(-1.4, -0.88, -0.5);
    fmprstr(data);
}

void draw_tic(void) {
    float v[3];
#ifdef RGB_MODE
    cpack(0x0000FF);
#else
    color(RED);
#endif
    
    bgntmesh();
    v[0] = -0.9;
    v[1] = -0.1;
    v2f(v);
    v[0] = -0.7;
    v[1] = 0.4;
    v2f(v);
    v[0] = 0.0;
    v[1] = -0.7;
    v2f(v);
    v[0] = 0.0;
    v[1] = 0.0;
    v2f(v);
    v[0] = 1.0;
    v[1] = 0.9;
    v2f(v);
    v[0] = 0.8;
    v[1] = 1.0;
    v2f(v);
    v[0] = 1.4;
    v[1] = 1.3;
    v2f(v);
    endtmesh();
}

float but[10][2] = {{0.9, 1.1},                                                             /* network */
                    {1.7, 0.4},                                                             /* colours random button */
                    {1.3, 0.6},                                                             /* colours... */
                    {1.5, 0.6}, {1.7, 0.6}, {1.3, 0.8}, {1.5, 0.8}, {1.7, 0.8}, {0.9, 0.9}, /* demo mode button */
                    {0.9, 1.3}};                                                            /* audio mode button */

void draw_buttons(int colour_choice, int num_robots) {
    float v[2];
    int i;
    char junk[32];

    pushmatrix();
    translate(-2.0, -1.0, -0.5);
    /* coords 0->2 in x, 0->1 in y */

    linewidth(2);
    /* network button */
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    circf(but[0][0], but[0][1], 0.08);
#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    circ(but[0][0], but[0][1], 0.08);
    if (!solo) {
        /* draw tic */
        pushmatrix();
        translate(but[0][0] - 0.02, but[0][1] - 0.02, 0.1);
        scale(0.06, 0.06, 1.0);
        draw_tic();
        popmatrix();
    }

    /* audio button */
#ifdef AUDIO
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    circf(but[9][0], but[9][1], 0.08);
#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    circ(but[9][0], but[9][1], 0.08);
    if (audio) {
        /* draw tic */
        pushmatrix();
        translate(but[9][0] - 0.02, but[9][1] - 0.02, 0.0);
        scale(0.06, 0.06, 1.0);
        draw_tic();
        popmatrix();
    }
#endif

    /* demo mode button */
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    circf(but[8][0], but[8][1], 0.08);
#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    circ(but[8][0], but[8][1], 0.08);
    if (demo_mode) {
        /* draw tic */
        pushmatrix();
        translate(but[8][0] - 0.02, but[8][1] - 0.02, 0.1);
        scale(0.06, 0.06, 1.0);
        draw_tic();

        popmatrix();
    }

    /* left arrow */
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    bgntmesh();
    v[0] = 0.1;
    v[1] = 0.4;
    v2f(v);
    v[0] = 0.3;
    v[1] = 0.3;
    v2f(v);
    v[0] = 0.3;
    v[1] = 0.5;
    v2f(v);
    endtmesh();
#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    bgnclosedline();
    v[0] = 0.1;
    v[1] = 0.4;
    v2f(v);
    v[0] = 0.3;
    v[1] = 0.3;
    v2f(v);
    v[0] = 0.3;
    v[1] = 0.5;
    v2f(v);
    endclosedline();

    /* right arrow */
#ifdef RGB_MODE
    cpack(0xffff00);
#else
    color(WHITE);
#endif
    bgntmesh();
    v[0] = 0.9;
    v[1] = 0.4;
    v2f(v);
    v[0] = 0.7;
    v[1] = 0.3;
    v2f(v);
    v[0] = 0.7;
    v[1] = 0.5;
    v2f(v);
    endtmesh();
#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    bgnclosedline();
    v[0] = 0.9;
    v[1] = 0.4;
    v2f(v);
    v[0] = 0.7;
    v[1] = 0.3;
    v2f(v);
    v[0] = 0.7;
    v[1] = 0.5;
    v2f(v);
    endclosedline();
    /* box between */
#ifdef RGB_MODE
    cpack(GREY50);
#else
    color(GREY50);
#endif
    //rectf(0.33, 0.28, 0.67, 0.52);

    /* colour shape */
    for (i = 0; i < COLOURS; i++) {
#ifdef RGB_MODE
        cpack(trail_col[i]);
#else
        color(trail_col[i]);
#endif
        circf(but[i + 2][0], but[i + 2][1], 0.08);
        if (i == colour_choice)
#ifdef RGB_MODE
            cpack(0xffffff);
        else
            cpack(0);
#else
            color(WHITE);
        else
            color(BLACK);
#endif
        circ(but[i + 2][0], but[i + 2][1], 0.08);
    }
    /* and random box */
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    circf(but[1][0], but[1][1], 0.08);
#ifdef RGB_MODE
    cpack(0);
#else
    color(BLACK);
#endif
    circ(but[1][0], but[1][1], 0.08);
    if (colour_choice == -1) {
        pushmatrix();
        translate(but[1][0] - 0.02, but[1][1] - 0.02, 0.0);
        scale(0.06, 0.06, 1.0);
        draw_tic();
        popmatrix();
    }

    linewidth(1);

    /* text */
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    fmsetfont(smallfont);
    cmov2(0.25, 1.05);
    fmprstr("network");
#ifdef AUDIO
    cmov2(0.25, 1.25);
    fmprstr("audio");
#endif
    cmov2(0.1, 0.85);
    fmprstr("demo mode");
    cmov2(0.2, 0.55);
    fmprstr("your robots");
    
    cmov2(1.1, 0.35);
    fmprstr("random");
    cmov2(1.3, 1.0);
    fmprstr("colours");
    fmsetfont(mediumfont);
    cmov2(0.35, 0.31);
    sprintf(junk, "%d", num_robots);
    fmprstr(junk);
#ifdef RGB_MODE
    cpack(0xff0000);
#else
    color(BLUE);
#endif
    cmov2(0.15, 1.6);
    fmprstr("Options");

    popmatrix();
}

/*
 * area 0: solo/network toggle
 * area 1: rand colours
 * area 2-7: colours
 * area 8: demo button
 * area 9: audio on/off
 * area 10: robots++
 * area 11: robots--
 */
int over_area(void) {
    int i;
    long xm, ym, xo, yo, xs, ys;
    float x, y, dx, dy;
    

    xm = getvaluator(MOUSEX);
    ym = getvaluator(MOUSEY);

    /* scale x, y to window */
    getsize(&xs, &ys);
    
    printf("DEBUG: xm=%ld, ym=%ld, xs=%ld, ys=%ld\n", 
           xm, ym, xs, ys);
    
    // CORRECTION: Les coordonnées de souris sont déjà relatives à la fenêtre
    // Pas besoin de getorigin() !
    x = (float)xm / (float)xs;
    y = (float)ym / (float)ys;
    
    y = 1.0f - y;  // Inverser l'axe Y pour correspondre au système de coordonnées OpenGL
    x *= 4.0;
    y *= 2.0;
    
    printf("Mouse: xm=%ld, ym=%ld, normalized=(%.2f, %.2f), final=(%.2f, %.2f)\n", 
           xm, ym, (float)xm / (float)xs, 1.0f - (float)ym / (float)ys, x, y);

    /* do circular buttons */
    for (i = 0; i < 10; i++) {
        dx = but[i][0] - x;
        dy = but[i][1] - y;
        if (dx * dx + dy * dy < 0.08 * 0.08) {
            printf("Hit button %d at (%.2f, %.2f)\n", i, but[i][0], but[i][1]);
            return (i);
        }
    }

    /* left arrow */
    if (x >= 0.1 && x <= 0.3 && y >= 0.3 && y <= 0.5) {
        printf("In left arrow zone\n");
        return (10);
    }
    
    /* right arrow */
    if (x >= 0.7 && x <= 0.9 && y >= 0.3 && y <= 0.5) {
        printf("In right arrow zone\n");
        return (11);
    }

    return (-1);
}

void draw_title_screen(int screen_num) {
    zclear();

    zbuffer(FALSE);
    if (screen_num == 0 || screen_num == 1)
        shaded_bg();
    else {
#ifdef RGB_MODE
        cpack(GREY50);
#else
        color(GREY50);
#endif
        clear();
    }

    /* draw a bike to start with */
    if (screen_num != 4) {
        zbuffer(TRUE);
        pushmatrix();
        if (screen_num == 3) {
            translate(0.75, 0.15, 0.0);
            scale(0.15, 0.15, 0.15);
        } else if (screen_num == 2) {
            translate(1.3, 0.6, 0.0);
            scale(0.07, 0.07, 0.07);
        } else
            scale(0.2, 0.2, 0.2);
        plan_o_bike();
        popmatrix();
        zbuffer(FALSE);
    }

    /* draw text if the fonts have scaled... */
    if (screen_num == 0) {
#ifdef RGB_MODE
        cpack(0);
#else
        color(BLACK);
#endif
        cmov(1.0, -0.9, -0.5);
        charstr("initialising...");
    } else if (screen_num == 1) {
        big_title_author();
#ifdef RGB_MODE
        cpack(0xffffff);
#else
        color(WHITE);
#endif
        cmov(-1.0, -0.9, -0.5);
        fmprstr("Return to Continue     ESC to exit");
    } else if (screen_num == 2) {
#ifdef RGB_MODE
        cpack(0xffffff);
#else
        color(WHITE);
#endif
        fmsetfont(bigfont);
        cmov(-1.0, 0.6, -0.5);
        fmprstr("Cycles");
#ifdef RGB_MODE
        cpack(0);
#else
        color(BLACK);
#endif
        fmsetfont(smallfont);
        cmov(-1.5, 0.3, -0.5);
        fmprstr("You are riding your cycle against humans and");
        cmov(-1.5, 0.15, -0.5);
        fmprstr("hunter killer robots. Avoid hitting all bike trails.");
        cmov(-1.5, 0.0, -0.5);
        fmprstr("Points are awarded for excessive speed and making");
        cmov(-1.5, -0.15, -0.5);
        fmprstr("others crash near you or run into your trail.");
        cmov(-1.5, -0.3, -0.5);
        fmprstr("           Really politically correct huh?");
        cmov(-1.5, -0.45, -0.5);
        fmprstr("Look for holes through to other levels, however");
        cmov(-1.5, -0.6, -0.5);
        fmprstr("only you can see your holes ... and they move!");

#ifdef RGB_MODE
        cpack(0xffffff);
#else
        color(WHITE);
#endif
        cmov(-1.0, -0.9, -0.5);
        fmprstr("Return to Continue     ESC to exit");
    } else if (screen_num == 3) {
        small_title_author();
#ifdef RGB_MODE
        cpack(0xffffff);
#else
        color(WHITE);
#endif
        cmov(0.2, -0.85, -0.5);
        fmprstr("Enter Data and Press Return");
    } else if (screen_num == 4) {
#ifdef RGB_MODE
        cpack(0xff0000);
#else
        color(BLUE);
#endif
        fmsetfont(mediumfont);
        cmov(-0.5, 0.7, -0.5);
        fmprstr("Help");
#ifdef RGB_MODE
        cpack(0xffff00);
#else
        color(CYAN);
#endif
        /* duplicated from instructions() */
        fmsetfont(smallfont);
        cmov(-1.2, 0.3, -0.5);
        fmprstr("turn  - left and right mouse buttons");
        cmov(-1.2, 0.15, -0.5);
        fmprstr("speed - hold down A or middle mouse to");
        cmov(-1.2, 0.0, -0.5);
        fmprstr("             accelerate. lift off to slow down");
        cmov(-1.2, -0.15, -0.5);
        fmprstr("jump  - space bar\n");
        cmov(-1.2, -0.3, -0.5);
        fmprstr("look  - use left and right arrows to look around");
        cmov(-1.2, -0.45, -0.5);
        fmprstr("help  - H toggles instructions");
        cmov(-1.2, -0.6, -0.5);
        fmprstr("quit  - ESC key");

        cmov(0.0, -0.8, -0.5);
#ifdef RGB_MODE
        cpack(0xffffff);
#else
        color(WHITE);
#endif
        fmprstr("Press Return to Continue");
    } else if (screen_num == 5) {
        cmov(0.0, -0.8, -0.5);
#ifdef RGB_MODE
        cpack(0x0000ff);
#else
        color(RED);
#endif
        fmprstr("Networking to Game...");
    }
    zbuffer(TRUE);
}

/*
 * title screens:
 *
 *  screen 0 is bikes + waiting
 *  screen 1 is bikes + titles + press key
 *  screen 2 is instructions
 *  screen 3 is small bikes + intro + data entry
 *  screen 4 is help
 *  screen 5 is screen 4 + starting network message
 */
void title_screen(int screen_num, char *name, int *colour_choice, int *num_robots) {
    int exit_now, key;
    short val, c;

    exit_now = 0;
    qdevice(RETKEY);

    if (screen_num == 0 || screen_num == 5)
        exit_now = 1; /* no looping if screen 0 */
    else if (screen_num == 3)
        qdevice(KEYBD);

    loadmatrix(idmat);
    ortho(-2.0, 2.0, -1.0, 1.0, 0.1, 3.0);

    draw_title_screen(screen_num);
    if (screen_num == 3) {
        draw_buttons(*colour_choice, *num_robots);
        draw_text_port_thing("Name:", name);
    }
    swapbuffers();
    /* draw again for textport */
    if (screen_num == 3) {
        draw_title_screen(screen_num);
        draw_buttons(*colour_choice, *num_robots);
        draw_text_port_thing("Name:", name);
        swapbuffers();
    }

    do {
        if (screen_num != 0 && screen_num != 5) {
            switch (qread(&val)) {
            case INPUTCHANGE:
                in_win = val % 256; /* % for dgl stuff */
                break;
            case REDRAW:
                reshapeviewport();
                scale_fonts_to_win();
                draw_title_screen(screen_num);
                if (screen_num == 3) {
                    draw_buttons(*colour_choice, *num_robots);
                    draw_text_port_thing("Name:", name);
                }
                swapbuffers();
                /* draw again for textport */
                if (screen_num == 3) {
                    draw_title_screen(screen_num);
                    draw_buttons(*colour_choice, *num_robots);
                    draw_text_port_thing("Name:", name);
                    swapbuffers();
                }
                break;
            case ESCKEY:
                exit(0);
                break;
            case KEYBD:
                if (screen_num == 3 && val) {
                    c = get_char(val, name, NAME_SIZE);
                    if (c == '\n' || c == '\r')
                        exit_now = 1;
                    draw_buttons(*colour_choice, *num_robots);
                    draw_text_port_thing("Name:", name);
                    swapbuffers();
                }
                break;
            case RETKEY:
                if (val && screen_num != 3)
                    exit_now = 1;
                break;
            case LEFTMOUSE:
                if (val && screen_num != 3)
                    exit_now = 1;
                else if (val && screen_num == 3) {
                    switch (key = over_area()) {
                    case 0:
                        if (solo)
                            solo = 0;
                        else
                            solo = 1;
                        break;
                    case 1: /* random button */
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        *colour_choice = key - 2;
                        break;
                    case 8:
                        if (demo_mode)
                            demo_mode = 0;
                        else
                            demo_mode = 1;
                        break;
                    case 9:
#ifdef AUDIO
                        if (audio)
                            audio = 0;
                        else
                            audio = 1;
#else
                        audio = 0;
#endif
                        break;
                    case 10:
                        (*num_robots)--;
                        if (*num_robots < 0)
                            *num_robots = 0;
                        break;
                    case 11:
                        (*num_robots)++;
                        if (*num_robots > CYCLES - 1)
                            *num_robots = CYCLES - 1;
                        break;
                    }
                    draw_buttons(*colour_choice, *num_robots);
                    draw_text_port_thing("Name:", name);
                    swapbuffers();
                }
                break;
            default:
                break;
            }
        }
    } while (!exit_now);

    unqdevice(RETKEY);
    if (screen_num == 3)
        unqdevice(KEYBD);

    /* restore coords */
    set_win_coords();
}

/*
 * draw a current score screen
 */
void draw_score_screen(int pts, int big_kills, int trail_kills) {
    char junk[64];

#ifdef RGB_MODE
    cpack(GREY50);
#else
    color(GREY25);
#endif
    zclear();
    clear();

    /* draw bikes as background */
    pushmatrix();
    translate(0.6, 0.2, 0.0);
    scale(0.1, 0.1, 0.1);
    plan_o_bike();
    popmatrix();

    zbuffer(FALSE);

    /* write up our score */
#ifdef RGB_MODE
    cpack(0xff0000);
#else
    color(BLUE);
#endif
    fmsetfont(mediumfont);
    cmov2(0.02, 0.8);
    fmprstr(good->name);

#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    cmov2(0.02, 0.2);
    sprintf(junk, "games %d  average score %d", good->games, (int)((float)good->pts / (float)good->games));
    fmprstr(junk);
#ifdef RGB_MODE
    cpack(0x00ff00);
#else
    color(GREEN);
#endif
    cmov2(0.02, 0.4);
    sprintf(junk, "game points %d", pts);
    fmprstr(junk);

#ifdef RGB_MODE
    cpack(0x0000ff);
#else
    color(RED);
#endif
    cmov2(0.02, 0.6);
    sprintf(junk, "kills %d trails %d", big_kills, trail_kills);
    fmprstr(junk);

    fmsetfont(smallfont);
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    cmov2(0.2, 0.05);
    fmprstr("Press Return to Continue     ESC to exit");

    pushmatrix();
    translate(0.75, 0.55, 0.0);
    scale(0.15 / DIM, -0.3 / DIM, 0.0);
    draw_all_2d();
    popmatrix();

    write_player_list(0.4, 0.95, -0.03);

    zbuffer(TRUE);
}

void score_screen(int pts, int big_kills, int trail_kills) {
    int i, exit_now;
    short val;
    struct tms t;

    scale_fonts_to_win();
    loadmatrix(idmat);
    ortho2(0.0, 1.0, 0.0, 1.0);

    exit_now = 0;
    qdevice(RETKEY);

    scale_fonts_to_win();
    draw_score_screen(pts, big_kills, trail_kills);
    swapbuffers();

    do {
        if (qtest()) {
            switch (qread(&val)) {
            case INPUTCHANGE:
                in_win = val % 256; /* % for dgl stuff */
                break;
            case REDRAW:
                reshapeviewport();
                scale_fonts_to_win();
                break;
            case ESCKEY:
                good->quit = 1; /* NOTE: this is in main loop and search_for_exit also */
                for (i = 0; i < CYCLES; i++)
                    if (robot[i])
                        bike[i].quit = 1;
                if (!solo)
                    send_update_mcast();
#ifdef AUDIO
                if (audio)
                    close_audio();
#endif
                exit(0);
                break;
            case RETKEY:
            case LEFTMOUSE:
                if (val)
                    exit_now = 1;
                break;
            default:
                break;
            }
        }

        set_speed_fac(0);

        for (i = 0; i < CYCLES; i++)
            if (robot[i] && bike[i].falling == 0)
                move_cycles(&bike[i]);

        move_our_robots();

        /* handle network events */
        if (!solo) {
            if (get_and_sort_mcasts() || last_sent + NET_TIMEOUT * HZ < times(&t))
                send_all_full_mcast();
            else
                send_update_mcast();
            last_sent = times(&t);

            /* kill off any cycles that aren't talking any more */
            kill_dead_cycle();
        }

#ifdef DEBUG
        printf("\n");
        printf("screens: robot: \n");
        {
            int i;
            for (i = 0; i < CYCLES; i++)
                printf("%d ", robot[i]);
        }
        printf("\n");
        printf("screens: used: \n");
        {
            int i;
            extern int used[CYCLES];
            for (i = 0; i < CYCLES; i++)
                printf("%d ", used[i]);
        }
        printf("\n");
        printf("screens: players: \n");
        {
            int i;
            extern int used[CYCLES];
            for (i = 0; i < CYCLES; i++)
                if (used[i])
                    printf("%d: %s aliv? %d fall %g   ", bike[i].id, bike[i].name, bike[i].alive, bike[i].fall);
        }
        printf("\n");
#endif

        draw_score_screen(pts, big_kills, trail_kills);
        swapbuffers();

    } while (!exit_now);

    unqdevice(RETKEY);

    /* restore coords */
    set_win_coords();
}
