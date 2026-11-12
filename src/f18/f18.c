/*
 * f18.c - the F-18 demo (IRIS Performer 2.2 and Coryphaeus EasyScene)
 *
 * Runs an EasyScene script of the demo (data/f18/scripts/*.set): the
 * window of its pipe, its channels, its objects, its keys and its modules.
 * The functions of the demo that are not rebuilt yet (the flight, the HUD,
 * the weather, the smoke, the sound...) are reported missing by the
 * interpreter, and the eye is then moved with the mouse.
 *
 * usage: f18 [-D symbol] [script.set]
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

#include "escene.h"
#include "flight.h"

static int win_w = 1024, win_h = 768;
static int wireframe, textures = 1, show_stats, show_help;
static double last_time, fps;
static int frames;

/* the eye, moved with the mouse while no module of the demo flies it */
static int free_eye = 1;
static float eye_speed;
static int drag_button = -1, drag_x, drag_y;

/* --- builtin functions of the scripts --------------------------------------- */

static void wireframeToggle(const char *a) { (void)a; wireframe = !wireframe; }
static void textureToggle(const char *a) { (void)a; textures = !textures; }
static void statsToggle(const char *a) { (void)a; show_stats = !show_stats; }
static void help(const char *a) { (void)a; show_help = !show_help; }

static void register_functions(void)
{
    esRegisterFunction("wireframeToggle", wireframeToggle);
    esRegisterFunction("textureToggle", textureToggle);
    esRegisterFunction("statsToggle", statsToggle);
    esRegisterFunction("help", help);
    flight_register();
}

/* --- devices ------------------------------------------------------------------ */

/* the IRIS GL device names of the scripts for the GLUT keys */
static const char *ascii_device(unsigned char k, char *buf)
{
    static const char *digits[10] = {
        "ZERO", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE"
    };
    if (k >= 'a' && k <= 'z') k = (unsigned char)(k - 'a' + 'A');
    if (k >= 'A' && k <= 'Z') {
        sprintf(buf, "%cKEY", k);
        return buf;
    }
    if (k >= '0' && k <= '9') {
        sprintf(buf, "%sKEY", digits[k - '0']);
        return buf;
    }
    switch (k) {
    case ' ': return "SPACEKEY";
    case 27: return "ESCKEY";
    case 13: return "RETKEY";
    case 9: return "TABKEY";
    case 8: return "BACKSPACEKEY";
    case 127: return "DELKEY";
    case ',': return "COMMAKEY";
    case '.': return "PERIODKEY";
    case '/': return "VIRGULEKEY";
    case ';': return "SEMICOLONKEY";
    case '-': return "MINUSKEY";
    case '=': return "EQUALKEY";
    }
    return NULL;
}

static const char *special_device(int k, char *buf)
{
    if (k >= GLUT_KEY_F1 && k <= GLUT_KEY_F12) {
        sprintf(buf, "F%dKEY", k - GLUT_KEY_F1 + 1);
        return buf;
    }
    switch (k) {
    case GLUT_KEY_UP: return "UPARROWKEY";
    case GLUT_KEY_DOWN: return "DOWNARROWKEY";
    case GLUT_KEY_LEFT: return "LEFTARROWKEY";
    case GLUT_KEY_RIGHT: return "RIGHTARROWKEY";
    case GLUT_KEY_PAGE_UP: return "PAGEUPKEY";
    case GLUT_KEY_PAGE_DOWN: return "PAGEDOWNKEY";
    case GLUT_KEY_HOME: return "HOMEKEY";
    case GLUT_KEY_END: return "ENDKEY";
    case GLUT_KEY_INSERT: return "INSERTKEY";
#ifdef GLUT_KEY_ALT_L
    case GLUT_KEY_ALT_L: return "LEFTALTKEY";
    case GLUT_KEY_ALT_R: return "RIGHTALTKEY";
    case GLUT_KEY_SHIFT_L: return "LEFTSHIFTKEY";
    case GLUT_KEY_SHIFT_R: return "RIGHTSHIFTKEY";
    case GLUT_KEY_CTRL_L: return "LEFTCTRLKEY";
    case GLUT_KEY_CTRL_R: return "RIGHTCTRLKEY";
#endif
    }
    return NULL;
}

/* calls the function bound to a device, 0 when none is */
static int device(const char *dev)
{
    esKeyBinding *k = dev ? esFindKey(dev) : NULL;
    if (!k || !k->func)
        return 0;
    k->func(NULL);
    return 1;
}

/* --- the eye ------------------------------------------------------------------ */

static esObject *eye_object(void)
{
    int i;
    for (i = 0; i < es.nchannels; i++)
        if (es.channels[i].attach)
            return es.channels[i].attach;
    return NULL;
}

/* above the middle of the terrain, looking north and down */
static void place_eye(void)
{
    esObject *eye = eye_object(), *ground = NULL;
    float z = 500;
    int i;
    if (!eye)
        return;
    for (i = 0; i < es.nobjects; i++)
        if (es.objects[i]->is_terrain || strcmp(es.objects[i]->name, "ground") == 0)
            ground = es.objects[i];
    if (ground && ground->radius > 0) {
        z = ground->center[2] + 1500;
        esObjectPos(eye, ground->center[0], ground->center[1] - 6000, z, 0, -10, 0);
    } else {
        esObjectPos(eye, 0, -50, 20, 0, -10, 0);
    }
    eye_speed = 0;
}

static void move_eye(float dt)
{
    esObject *eye = eye_object();
    float sh, ch, sp, cp;
    if (!eye || !free_eye || eye_speed == 0)
        return;
    pfSinCos(eye->hpr[0], &sh, &ch);
    pfSinCos(eye->hpr[1], &sp, &cp);
    esObjectPos(eye, eye->xyz[0] - sh * cp * eye_speed * dt, eye->xyz[1] + ch * cp * eye_speed * dt,
                eye->xyz[2] + sp * eye_speed * dt, eye->hpr[0], eye->hpr[1], eye->hpr[2]);
}

/* --- drawing ------------------------------------------------------------------ */

static void key_name(const char *dev, char *out);

static void text(int x, int y, const char *s)
{
    glRasterPos2i(x, y);
    while (*s)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s++);
}

/* the pipes (screens) side by side in the window, from left to right by
 * the heading of their channels (the 3 pipe scripts turn the side ones by
 * 53.33 degrees) */
static int pipe_slot(const esPipe *p)
{
    int i, j, slot = 0;
    float hi = 0, hj;
    for (i = 0; i < es.nchannels; i++)
        if (es.channels[i].pipe == p) {
            hi = es.channels[i].viewrot[0];
            break;
        }
    for (j = 0; j < es.npipes; j++) {
        const esPipe *q = &es.pipes[j];
        if (q == p)
            continue;
        hj = 0;
        for (i = 0; i < es.nchannels; i++)
            if (es.channels[i].pipe == q) {
                hj = es.channels[i].viewrot[0];
                break;
            }
        if (hj > hi || (hj == hi && q < p))
            slot++;
    }
    return slot;
}

static void draw_channel(const esChannel *c)
{
    float lpos[4] = { 0.3f, -0.4f, 0.85f, 0 };
    float white[4] = { 1, 1, 1, 1 }, amb[4] = { 0.4f, 0.4f, 0.4f, 1 };
    float m[16], fh, fv, aspect;
    int n = es.npipes > 0 ? es.npipes : 1, slot = c->pipe ? pipe_slot(c->pipe) : 0;
    float pw = (float)win_w / n;
    int x0 = (int)(pw * (slot + c->viewport[0])), x1 = (int)(pw * (slot + c->viewport[1]));
    int y0 = (int)(c->viewport[2] * win_h), y1 = (int)(c->viewport[3] * win_h);
    if (x1 <= x0 || y1 <= y0)
        return;
    glViewport(x0, y0, x1 - x0, y1 - y0);
    aspect = (float)(x1 - x0) / (y1 - y0);
    fh = c->fov_h > 0 ? c->fov_h : 45;
    fv = c->fov_v > 0 ? c->fov_v : (float)(2 * PF_RAD2DEG(atan(tan(PF_DEG2RAD(fh / 2)) / aspect)));
    if (c->fov_h <= 0 && c->fov_v > 0)
        aspect = (float)(tan(PF_DEG2RAD(fh / 2)) / tan(PF_DEG2RAD(fv / 2)));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fv, aspect, c->near_clip, c->far_clip);
    glMatrixMode(GL_MODELVIEW);
    esChannelViewMatrix(c, m);
    glLoadMatrixf(m);

    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    glEnable(GL_LIGHT0);

    esRunPhase(ES_PRECULL);
    esRunPhase(ES_POSTCULL);
    esRunPhase(ES_PREDRAW);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    pfEnable(PFEN_LIGHTING);
    if (textures) pfEnable(PFEN_TEXTURE); else pfDisable(PFEN_TEXTURE);
    pfDrawScene(es.root, pfGetTime());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    esRunPhase(ES_POSTDRAW);
    /* the HUD channel of the main channel */
    if (es.current_channel == 0)
        esDrawHuds();
}

static void overlay(void)
{
    char buf[512];
    int i, y;
    glViewport(0, 0, win_w, win_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, win_w, 0, win_h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);
    if (show_stats) {
        esObject *eye = eye_object();
        sprintf(buf, "%.1f fps  %d objects  %d channels", fps, es.nobjects, es.nchannels);
        text(10, win_h - 20, buf);
        if (eye) {
            sprintf(buf, "eye %.0f %.0f %.0f  hpr %.0f %.0f %.0f  speed %.0f m/s", eye->xyz[0],
                    eye->xyz[1], eye->xyz[2], eye->hpr[0], eye->hpr[1], eye->hpr[2], eye_speed);
            text(10, win_h - 38, buf);
        }
    }
    if (show_help) {
        y = win_h - 110;
        text(10, y, es.script);
        y -= 20;
        if (!free_eye) {
            text(10, y, "Arrows        stick: left / right roll, up dives, down climbs");
            y -= 16;
            text(10, y, "A / Z         faster / slower (or right / middle button, or wheel)");
            y -= 16;
            text(10, y, f18_mouse.no_steer ? "M             the mouse flies the airplane (off)"
                                          : "M             the mouse flies the airplane (on)");
            y -= 16;
        }
        for (i = 0; i < es.nkeys; i++) {
            char name[64];
            int k;
            /* A and Z are the speed keys */
            if (!free_eye && (strcmp(es.keys[i].device, "AKEY") == 0 || strcmp(es.keys[i].device, "ZKEY") == 0))
                continue;
            /* one line for the keys bound to the same function */
            for (k = 0; k < i; k++)
                if (es.keys[k].function && es.keys[i].function &&
                    strcmp(es.keys[k].function, es.keys[i].function) == 0)
                    break;
            if (k < i)
                continue;
            key_name(es.keys[i].device, name);
            for (k = i + 1; k < es.nkeys; k++)
                if (es.keys[k].function && es.keys[i].function &&
                    strcmp(es.keys[k].function, es.keys[i].function) == 0 && strlen(name) < 40) {
                    char other[32];
                    key_name(es.keys[k].device, other);
                    strcat(name, ", ");
                    strcat(name, other);
                }
            sprintf(buf, "%-13s %s%s", name, es.keys[i].function ? es.keys[i].function : "",
                    es.keys[i].func ? "" : "  (not available yet)");
            text(10, y, buf);
            y -= 16;
        }
        y -= 8;
        text(10, y, free_eye ? "mouse: left drag turn the eye, right drag or wheel speed, middle drag slide, Esc quit"
                             : "mouse: from the middle of the window, roll and pitch (M); H this help; Esc quit");
    }
    glEnable(GL_DEPTH_TEST);
}

static void display(void)
{
    static double app_time = -1;
    double t = pfGetTime();
    int i, ticks = 0;
    if (last_time > 0) {
        move_eye((float)(t - last_time));
        frames++;
    }
    /* the app modules run at the frame rate of the script, as the demo
     * moves its objects by a step per frame */
    if (app_time < 0)
        app_time = t;
    while (app_time <= t && ticks < 8) {
        esRunPhase(ES_PREAPP);
        esRunPhase(ES_POSTAPP);
        app_time += 1.0 / es.framerate;
        ticks++;
    }
    if (app_time < t)
        app_time = t;
    if (frames >= 20) {
        static double t0;
        if (t0 > 0)
            fps = frames / (t - t0);
        t0 = t;
        frames = 0;
    }
    last_time = t;
    pfFrameTime(t);

    glClearColor(0.45f, 0.6f, 0.85f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (i = 0; i < es.nchannels; i++) {
        es.current_channel = i;
        draw_channel(&es.channels[i]);
    }
    esRunPhase(ES_POSTRENDER);
    overlay();
    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    win_w = w;
    win_h = h > 0 ? h : 1;
    f18_mouse.w = win_w;
    f18_mouse.h = win_h;
}

static void passive(int x, int y)
{
    f18_mouse.x = x;
    f18_mouse.y = y;
    f18_mouse.inside = 1;
}

static void idle(void) { glutPostRedisplay(); }

/* keys added to the ones of the scripts: A and Z hold the speed up and
 * down (before the bindings of the scripts), 1 and 2 stand for the Alt
 * keys, which do not reach every program */
static void keyboard(unsigned char k, int x, int y)
{
    char buf[32];
    (void)x; (void)y;
    if (!free_eye && (k == 'a' || k == 'A')) {
        f18_mouse.faster = 1;
        return;
    }
    if (!free_eye && (k == 'z' || k == 'Z')) {
        f18_mouse.slower = 1;
        return;
    }
    if (!free_eye && (k == 'm' || k == 'M')) {
        f18_mouse.no_steer = !f18_mouse.no_steer;
        return;
    }
    if (device(ascii_device(k, buf)))
        return;
    if ((k == '+' || k == '=') && device("UPARROWKEY"))
        return;
    if (k == '-' && device("DOWNARROWKEY"))
        return;
    if (k == '1' && device("LEFTALTKEY"))
        return;
    if (k == '2' && device("RIGHTALTKEY"))
        return;
    if (k == 27)
        exit(0);
}

static void keyboard_up(unsigned char k, int x, int y)
{
    (void)x; (void)y;
    if (k == 'a' || k == 'A')
        f18_mouse.faster = 0;
    if (k == 'z' || k == 'Z')
        f18_mouse.slower = 0;
}

/* the IRIS GL name of a device as a key name for the help */
static void key_name(const char *dev, char *out)
{
    static const char *names[][2] = {
        { "UPARROWKEY", "Page Up, +" }, { "DOWNARROWKEY", "Page Down, -" }, { "LEFTARROWKEY", "Left" },
        { "RIGHTARROWKEY", "Right" }, { "LEFTALTKEY", "Left Alt or 1" }, { "RIGHTALTKEY", "Right Alt or 2" },
        { "SPACEKEY", "Space" }, { "ESCKEY", "Esc" }, { "RETKEY", "Enter" }, { "PAGEUPKEY", "Page Up" },
        { "PAGEDOWNKEY", "Page Down" }, { "LEFTSHIFTKEY", "Left Shift" }, { "RIGHTSHIFTKEY", "Right Shift" },
        { "LEFTCTRLKEY", "Left Ctrl" }, { "RIGHTCTRLKEY", "Right Ctrl" }
    };
    size_t i, n = strlen(dev);
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        if (strcmp(dev, names[i][0]) == 0) {
            strcpy(out, names[i][1]);
            return;
        }
    if (n > 3 && strcmp(dev + n - 3, "KEY") == 0 && n - 3 < 30) {
        memcpy(out, dev, n - 3);
        out[n - 3] = 0;
    } else {
        strcpy(out, dev);
    }
}

/* the arrows fly the airplane (the scripts bind them to the range of the
 * radar, on Page Up and Page Down here) */
static int *arrow(int k)
{
    switch (k) {
    case GLUT_KEY_UP: return &f18_mouse.up;
    case GLUT_KEY_DOWN: return &f18_mouse.down;
    case GLUT_KEY_LEFT: return &f18_mouse.kleft;
    case GLUT_KEY_RIGHT: return &f18_mouse.kright;
    }
    return NULL;
}

static void special(int k, int x, int y)
{
    char buf[32];
    int *a = arrow(k);
    (void)x; (void)y;
    if (!free_eye && a) {
        *a = 1;
        return;
    }
    if (!free_eye && k == GLUT_KEY_PAGE_UP && device("UPARROWKEY"))
        return;
    if (!free_eye && k == GLUT_KEY_PAGE_DOWN && device("DOWNARROWKEY"))
        return;
    device(special_device(k, buf));
}

static void special_up(int k, int x, int y)
{
    int *a = arrow(k);
    (void)x; (void)y;
    if (a)
        *a = 0;
}
static void mouse(int b, int state, int x, int y)
{
    passive(x, y);
    if (b == GLUT_LEFT_BUTTON) f18_mouse.left = state == GLUT_DOWN;
    if (b == GLUT_MIDDLE_BUTTON) f18_mouse.middle = state == GLUT_DOWN;
    if (b == GLUT_RIGHT_BUTTON) f18_mouse.right = state == GLUT_DOWN;
    if (!free_eye) {
        if (state == GLUT_DOWN && b == 3) f18_mouse.wheel++;
        if (state == GLUT_DOWN && b == 4) f18_mouse.wheel--;
        return;
    }
    if (state == GLUT_DOWN) {
        if (b == 3) { eye_speed += 20; return; }
        if (b == 4) { eye_speed -= 20; return; }
        drag_button = b;
        drag_x = x;
        drag_y = y;
    } else {
        drag_button = -1;
    }
}

static void motion(int x, int y)
{
    esObject *eye = eye_object();
    int dx = x - drag_x, dy = y - drag_y;
    passive(x, y);
    drag_x = x;
    drag_y = y;
    if (!eye || !free_eye)
        return;
    if (drag_button == GLUT_LEFT_BUTTON) {
        float p = eye->hpr[1] - dy * 0.3f;
        if (p > 89) p = 89;
        if (p < -89) p = -89;
        esObjectPos(eye, eye->xyz[0], eye->xyz[1], eye->xyz[2], eye->hpr[0] - dx * 0.3f, p, 0);
    } else if (drag_button == GLUT_RIGHT_BUTTON) {
        eye_speed -= dy * 2.0f;
    } else if (drag_button == GLUT_MIDDLE_BUTTON) {
        float sh, ch, s = 2;
        pfSinCos(eye->hpr[0], &sh, &ch);
        esObjectPos(eye, eye->xyz[0] - ch * dx * s, eye->xyz[1] - sh * dx * s, eye->xyz[2] + dy * s,
                    eye->hpr[0], eye->hpr[1], eye->hpr[2]);
    }
}

static void entry(int state)
{
    /* the airplane flies straight while the pointer is out */
    if (state == GLUT_LEFT)
        f18_mouse.inside = 0;
}

/* the "configure" statement: the pipes are defined, the window is opened */
static void configure(void)
{
    const esPipe *p = &es.pipes[0];
    int w = p->viewport[1] - p->viewport[0] + 1, h = p->viewport[3] - p->viewport[2] + 1;
    int sw = glutGet(GLUT_SCREEN_WIDTH), sh = glutGet(GLUT_SCREEN_HEIGHT);
    if (w < 64 || h < 64) {
        w = 1280;
        h = 1024;
    }
    w *= es.npipes;
    /* the window fits in the screen with the proportions of the pipe */
    if (sw > 0 && sh > 0 && (w > sw * 9 / 10 || h > sh * 9 / 10)) {
        float s = PF_MIN(sw * 0.9f / w, sh * 0.9f / h);
        w = (int)(w * s);
        h = (int)(h * s);
    }
    win_w = w;
    win_h = h;
    f18_mouse.w = w;
    f18_mouse.h = h;
    glutInitWindowSize(w, h);
    glutCreateWindow(p->name);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboard_up);
    glutSpecialUpFunc(special_up);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(passive);
    glutEntryFunc(entry);
    glutIdleFunc(idle);
}

int main(int argc, char **argv)
{
    const char *script = NULL;
    static const char *defaults[] = {
        "data/f18/scripts/F18_1pipe.set", "../data/f18/scripts/F18_1pipe.set",
        "scripts/F18_1pipe.set"
    };
    int i;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-D") == 0 && i + 1 < argc)
            esDefine(argv[++i]);
        else if (strncmp(argv[i], "-D", 2) == 0 && argv[i][2])
            esDefine(argv[i] + 2);
        else
            script = argv[i];
    }
    for (i = 0; !script && i < 3; i++) {
        FILE *fp = fopen(defaults[i], "rb");
        if (fp) {
            fclose(fp);
            script = defaults[i];
        }
    }
    if (!script) {
        fprintf(stderr, "usage: f18 [-D symbol] [script.set]\n");
        return 1;
    }
    register_functions();
    if (!esLoadScript(script, configure) || !es.configured) {
        fprintf(stderr, "f18: %s did not configure a window\n", script);
        return 1;
    }
    /* no module of the demo flies the eye yet */
    free_eye = esFindFunction("initFlight") == NULL;
    if (free_eye) {
        place_eye();
        show_help = 1;
        show_stats = 1;
    }
    glutMainLoop();
    return 0;
}
