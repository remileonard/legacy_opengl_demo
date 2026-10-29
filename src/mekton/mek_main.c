/*
 * mek_main.c - main loop, game states and input of Mekton.
 *
 * The original ran under IRIS Performer with X input; this port uses
 * GLUT. The game states, key bindings and the order of the updates in
 * mek_loop follow the original.
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <GL/freeglut.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mekton.h"

Mek *mektons[MAX_MEKTONS];
Mek *my_mek;
int number_mektons;
Input mek_input_state;
float tps = 30, tps_add;
int int_tps = 20;
int ticks, check_ticks, total_frames;
static ChanData chan_data;
ChanData *cd_app = &chan_data;
int user_mek_type, user_team, start_pos = -1;
char user_name[16] = "player";
short shadow, comm, network, mekton_ui;
int use_stick;
float game_time, intro_start_time;
pfScene *scene;
float fov = 80, chan_fov = 80;
int win_w = 960, win_h = 768;
float time_limit;
int quit_game;
extern float g_acc;

static double last_check;
static int intro_frames;

void set_tps(float t)
{
    tps = t;
    g_acc = 9.75361919f / tps / tps;
}

void init_time(int n)
{
    int_tps = n;
    last_check = pfGetTime();
    set_tps((float)int_tps);
    ticks = -1;
    check_ticks = 5;
    tps_add = 0;
}

/* adapts the ticks per second to the measured frame rate */
void check_time(void)
{
    double t = pfGetTime(), dt = t - last_check;
    int measured;
    set_tps((float)int_tps);
    last_check = t;
    if (dt <= 0)
        return;
    measured = (int)(ticks / dt + 0.5);
    if (measured == int_tps) {
        tps_add = 0;
        return;
    }
    if (int_tps < measured)
        tps_add = (float)((measured - int_tps) / 3 + 1) / check_ticks;
    else
        tps_add = (float)((measured - int_tps) / 3 - 1) / check_ticks;
    int_tps += (int)(tps_add * check_ticks);
    if (int_tps < 3)
        int_tps = 3;
    if (120 < int_tps)
        int_tps = 120;
}

void mek_start(void)
{
    my_mek = mektons[0];
    memcpy(my_mek->name, user_name, sizeof(my_mek->name) - 1);
    number_mektons = 1;
    init_mek(my_mek, user_mek_type);
    mek_start_pos(my_mek, start_pos);
    init_message_lists();
    init_camera();
    init_instruments();
    init_powerplant();
    start_pods();
    start_turrets();
    cd_app->state = STATE_INTRO;
    intro_start_time = 0;
    cd_app->time = 0;
    intro_frames = 0;
    init_time(20);
    init_game_time();
}

void mek_restart(void)
{
    reset_projectiles();
    reset_pods();
    reset_turrets();
    mek_start();
}

static void mark_keys_used(void)
{
    mek_input_state.a &= ~2; mek_input_state.d &= ~2; mek_input_state.e &= ~2; mek_input_state.j &= ~2;
    mek_input_state.q &= ~2; mek_input_state.s &= ~2; mek_input_state.w &= ~2;
    mek_input_state.space &= ~2;
}

/* one tick of the game */
static void mek_loop(void)
{
    if (cd_app->state == STATE_PLAY) {
        if (++ticks == check_ticks) {
            check_time();
            ticks = 0;
        }
        if (tps_add != 0)
            set_tps(tps + tps_add);
        update_game_time();
        update_colony();
        if (!shadow) {
            mek_input(my_mek);
            mek_dynamics(my_mek);
            adjust_origin();
        }
        update_projectiles();
        handle_pods();
        handle_turrets();
        update_explosions();
        update_camera(my_mek);
        update_db();
        mark_keys_used();
        total_frames++;
    } else if (cd_app->state == STATE_INTRO) {
        /* the logo fades in after three seconds */
        if (intro_frames < 4) {
            cd_app->fade = 0;
            intro_frames++;
        } else {
            float t;
            if (intro_start_time == 0)
                intro_start_time = (float)pfGetTime();
            t = (float)pfGetTime() - intro_start_time;
            if (t < 3) {
                cd_app->fade = 0;
            } else if (t < 32) {
                cd_app->fade = (int)((t - 3) * 50);
            } else {
                intro_start_time = 0;
                cd_app->state = STATE_WAIT;
            }
        }
    }
    my_mek->flags &= ~1;
}

/* --- drawing ------------------------------------------------------------ */

void draw_background_scene(int state);

static void set_view(void)
{
    pfMatrix m, inv;
    double vfov;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* Performer's field of view is horizontal */
    vfov = 2 * atan(tan(PF_DEG2RAD(chan_fov) / 2) * chan_h / (double)chan_w);
    gluPerspective(PF_RAD2DEG(vfov), (double)chan_w / chan_h, 1.0, 20100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    /* Performer looks along +Y with +Z up */
    glRotatef(-90, 1, 0, 0);
    pfMakeCoordMat(m, &camera);
    pfInvertOrthoMat(inv, m);
    glMultMatrixf(&inv[0][0]);
}

static void display(void)
{
    glViewport(0, 0, win_w, win_h);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(chan_x, chan_y, chan_w, chan_h);
    if (cd_app->state == STATE_PLAY) {
        float amb[4] = { 0.05f, 0.05f, 0.05f, 1 };
        set_view();
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
        draw_background_scene(cd_app->state);
        pfDrawScene(scene, pfGetFrameTime());
    }
    mek_post_draw();
    glutSwapBuffers();
}

static void idle(void)
{
    pfFrameTime(pfGetTime());
    mek_loop();
    if (quit_game)
        exit(0);
    glutPostRedisplay();
}

static void reshape(int w, int h)
{
    win_w = w;
    win_h = h > 0 ? h : 1;
    hud_layout(win_w, win_h);
}

/* --- input -------------------------------------------------------------- */

void fkey_down(void);
void transform(void);
void give_up(void);
void enter_key(void);

static void modifiers(void)
{
    int m = glutGetModifiers();
    mek_input_state.shift = (m & GLUT_ACTIVE_SHIFT) != 0;
    mek_input_state.ctrl = (m & GLUT_ACTIVE_CTRL) != 0;
    mek_input_state.alt = (m & GLUT_ACTIVE_ALT) != 0;
}

/* the screens before the game */
static void other_key(unsigned char k)
{
    switch (cd_app->state) {
    case STATE_INTRO:
        if (k == ' ') {
            cd_app->state = STATE_WAIT;
            intro_start_time = 0;
        }
        break;
    case STATE_WAIT:
        if (k == ' ')
            cd_app->state = STATE_DEAD;
        break;
    case STATE_DEAD:
        if (k == ' ' || k == '1' || k == '2') {
            if (k == '1') user_mek_type = VANTAGE;
            if (k == '2') user_mek_type = EX_CENTINEL;
            cd_app->state = 4;
            init_mek(my_mek, user_mek_type);
        }
        break;
    case 4:
        if (k == ' ' || (k >= '0' && k <= '9')) {
            cd_app->state = STATE_PLAY;
            start_pos = k == ' ' ? -1 : k - '0';
            mek_start_pos(my_mek, start_pos);
            mek_input_state.x = center_x;
            mek_input_state.y = center_y;
            glutWarpPointer(center_x, win_h - 1 - center_y);
        }
        break;
    case 9:
        if (k == ' ') {
            cd_app->state = STATE_INTRO;
            intro_start_time = 0;
        }
        break;
    }
}

static int *letter(unsigned char k)
{
    switch (k) {
    case 'a': case 'A': return &mek_input_state.a;
    case 'd': case 'D': return &mek_input_state.d;
    case 'e': case 'E': return &mek_input_state.e;
    case 'j': case 'J': return &mek_input_state.j;
    case 'q': case 'Q': return &mek_input_state.q;
    case 's': case 'S': return &mek_input_state.s;
    case 'w': case 'W': return &mek_input_state.w;
    }
    return NULL;
}

static void keyboard(unsigned char k, int x, int y)
{
    int *l;
    (void)x; (void)y;
    modifiers();
    if (k == 27) {
        quit_game = 1;
        return;
    }
    if (cd_app->state != STATE_PLAY) {
        other_key(k);
        return;
    }
    if ((l = letter(k)) != NULL) {
        *l = 3;
        return;
    }
    switch (k) {
    case ' ':
        if (my_mek->status == 2)
            cd_app->state = STATE_DEAD;
        else
            mek_input_state.space = 3;
        break;
    case 13: enter_key(); break;
    case 127: give_up(); break;
    case 9: mek_input_state.tab = 1; break;
    case 'f': case 'F': fkey_down(); break;
    case 'h': case 'H': if (mek_input_state.shift) toggle_help(); else toggle_hud(); break;
    case 'r': case 'R': toggle_radar(); break;
    case 't': case 'T': transform(); break;
    case 'v': case 'V': light_adjust(); break;
    case 'z': case 'Z': cc_zoom(); break;
    case 'x': case 'X': cc_spin(); break;
    case 'c': case 'C': cc_height(); break;
    }
}

static void keyboard_up(unsigned char k, int x, int y)
{
    int *l;
    (void)x; (void)y;
    modifiers();
    if ((l = letter(k)) != NULL) {
        *l &= ~1;
        return;
    }
    switch (k) {
    case ' ': mek_input_state.space &= ~1; break;
    case 9: mek_input_state.tab = 0; break;
    case 'z': case 'Z': cc_zoom_stop(); break;
    case 'x': case 'X': cc_spin_stop(); break;
    case 'c': case 'C': cc_height_stop(); break;
    }
}

static void special(int k, int x, int y)
{
    static const int cams[7] = { 0, 2, 1, 5, 4, 6, 7 };
    (void)x; (void)y;
    modifiers();
    if (cd_app->state == STATE_PLAY && k >= GLUT_KEY_F1 && k <= GLUT_KEY_F7)
        cm_set(cams[k - GLUT_KEY_F1]);
}

static void special_up(int k, int x, int y)
{
    (void)k; (void)x; (void)y;
    modifiers();
}

static void motion(int x, int y)
{
    if (mek_input_state.tab)
        return;
    mek_input_state.x = x;
    mek_input_state.y = win_h - 1 - y;
}

static void mouse(int b, int state, int x, int y)
{
    int down = state == GLUT_DOWN;
    modifiers();
    if (b == GLUT_LEFT_BUTTON) mek_input_state.left = down;
    if (b == GLUT_MIDDLE_BUTTON) mek_input_state.middle = down;
    if (b == GLUT_RIGHT_BUTTON) mek_input_state.right = down;
    motion(x, y);
}

int main(int argc, char **argv)
{
    const char *data = "data/mekton";
    char path[4096];
    int i;

    glutInit(&argc, argv);
    hud_layout(win_w, win_h);
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-ex") == 0)
            user_mek_type = EX_CENTINEL;
        else if (strcmp(argv[i], "-pos") == 0 && i + 1 < argc)
            start_pos = atoi(argv[++i]);
        else if (strcmp(argv[i], "-time") == 0 && i + 1 < argc)
            time_limit = 60 * (float)atof(argv[++i]);
        else if (strcmp(argv[i], "-name") == 0 && i + 1 < argc)
            strncpy(user_name, argv[++i], 15);
        else
            data = argv[i];
    }
    sprintf(path, "%.1000s/models:%.1000s/textures:%.1000s", data, data, data);
    pfFilePath(path);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(win_w, win_h);
    glutCreateWindow("Mekton");
    glutIgnoreKeyRepeat(1);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboard_up);
    glutSpecialFunc(special);
    glutSpecialUpFunc(special_up);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    glutIdleFunc(idle);

    scene = pfNewScene();
    srand(1);
    init_db();
    if (!vantage || !pfGetNumChildren(vantage->model)) {
        fprintf(stderr, "mekton: no data found in %s\n", data);
        return 1;
    }
    init_lighting();
    set_tps(30);
    mek_start();
    mek_input_state.x = center_x;
    mek_input_state.y = center_y;
    glutMainLoop();
    return 0;
}
