/*
 * mek_hud.c - what Mekton draws over the 3D view: the intro with the
 * logo and the credits, the text screens before the game, the head up
 * display, the instrument panel with its gauges and radars, and the
 * console painted in the overlay planes.
 *
 * The original drew in a 1280x1024 window: the channel is the upper
 * 1280x749 part and the console covers the lower part. The coordinates
 * below are those of that window, scaled to the real window size.
 */
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

/* --- screen layout (sd->f628 ...) --------------------------------------- */

int center_x = 512, center_y = 480;
int chan_x, chan_y, chan_w = 1024, chan_h = 599;
static float scale_x = 0.8f, scale_y = 0.8f;
static void *font = GLUT_BITMAP_HELVETICA_12;

static void make_fonts(void)
{
    if (win_w >= 1125 && win_h >= 900)
        font = GLUT_BITMAP_HELVETICA_18;
    else if (win_w >= 750 && win_h >= 600)
        font = GLUT_BITMAP_HELVETICA_12;
    else
        font = GLUT_BITMAP_HELVETICA_10;
}

/* the channel is the window above the console, 749 of 1024 lines */
void hud_layout(int w, int h)
{
    scale_x = w / 1280.0f;
    scale_y = h / 1024.0f;
    chan_x = 0;
    chan_y = (int)(275 * scale_y + 0.5f);
    chan_w = w;
    chan_h = h - chan_y;
    center_x = chan_x + chan_w / 2;
    center_y = chan_y + chan_h / 2;
    make_fonts();
}

static void scaled_viewport(int l, int r, int b, int t)
{
    int x0 = (int)(l * scale_x), x1 = (int)(r * scale_x);
    int y0 = (int)(b * scale_y), y1 = (int)(t * scale_y);
    glViewport(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
    glScissor(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

static void centered_viewport(int l, int r, int b, int t)
{
    int x0 = (int)(l * scale_x) + center_x, x1 = (int)(r * scale_x) + center_x;
    int y0 = (int)(b * scale_y) + center_y, y1 = (int)(t * scale_y) + center_y;
    glViewport(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
    glScissor(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

static void chan_viewport(void)
{
    glViewport(chan_x, chan_y, chan_w, chan_h);
    glScissor(chan_x, chan_y, chan_w, chan_h);
}

static void clear_viewport(float r, float g, float b)
{
    glEnable(GL_SCISSOR_TEST);
    glClearColor(r, g, b, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

static void ortho(double l, double r, double b, double t)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(l, r, b, t);
    glMatrixMode(GL_MODELVIEW);
}

/* --- text ----------------------------------------------------------------- */

static int string_width(const char *s)
{
    return glutBitmapLength(font, (const unsigned char *)s);
}

/* width in the units of the 1280 wide screen */
static int text_width(const char *s)
{
    return (int)(string_width(s) / scale_x);
}

static void ogl_charstr(const char *s)
{
    while (*s)
        glutBitmapCharacter(font, *s++);
}

/* --- the line drawings (.gset files) -------------------------------------- */

typedef struct HudGSet {
    int nprims;
    int *lengths;
    float *v;
    float color[4];
} HudGSet;

/* reads the line strips of a .gset file into g, returns their number */
static int read_gsets(const char *file, HudGSet **g, int max)
{
    char path[4096], line[256], kw[32];
    FILE *f;
    HudGSet *cur = NULL;
    int n = 0, nv = 0, nl = 0, k;

    if (!pfFindFile(file, path, 4)) {
        fprintf(stderr, "read_gsets(): could not find readable \"%s\".\n", file);
        return 0;
    }
    if ((f = fopen(path, "r")) == NULL)
        return 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%31s", kw) != 1)
            continue;
        if (strcmp(kw, "G") == 0) {
            if (n == max)
                break;
            cur = g[n++] = (HudGSet *)calloc(1, sizeof(HudGSet));
            cur->color[0] = cur->color[1] = cur->color[2] = cur->color[3] = 1;
            nv = nl = 0;
        } else if (!cur) {
            continue;
        } else if (strcmp(kw, "c") == 0) {
            sscanf(line, "%*s %f %f %f %f", &cur->color[0], &cur->color[1],
                   &cur->color[2], &cur->color[3]);
        } else if (strcmp(kw, "V") == 0) {
            sscanf(line, "%*s %d", &k);
            cur->v = (float *)calloc(k * 3, sizeof(float));
        } else if (strcmp(kw, "v") == 0) {
            sscanf(line, "%*s %f %f %f", &cur->v[nv * 3], &cur->v[nv * 3 + 1],
                   &cur->v[nv * 3 + 2]);
            nv++;
        } else if (strcmp(kw, "L") == 0) {
            sscanf(line, "%*s %d", &cur->nprims);
            cur->lengths = (int *)calloc(cur->nprims, sizeof(int));
        } else if (strcmp(kw, "l") == 0 && nl < cur->nprims) {
            sscanf(line, "%*s %d", &cur->lengths[nl++]);
        }
    }
    fclose(f);
    return n;
}

static void draw_gset(const HudGSet *g)
{
    int i, j, k = 0;
    if (!g)
        return;
    glColor4fv(g->color);
    for (i = 0; i < g->nprims; i++) {
        glBegin(GL_LINE_STRIP);
        for (j = 0; j < g->lengths[i]; j++, k++)
            glVertex3fv(&g->v[k * 3]);
        glEnd();
    }
}

static HudGSet *intro_gsets[7], *dead_gsets[2], *complete_gsets[2], *help_gsets[3];
static HudGSet *h_radar[1], *v_radar[1], *numbers[12], *hud_static[5];
static HudGSet *hud_ladder[33], *hud_heading[13];

/* --- console in the overlay planes (console.ov) --------------------------- */

static unsigned char *console;
static int console_w, console_h;
static GLuint console_tex;

/* run length coded colour indices, index 0 is transparent */
static void ov_read(const char *file)
{
    char path[4096];
    unsigned char hdr[784], *data;
    FILE *f;
    int size, x, y, p = 0;

    if (!pfFindFile(file, path, 4) || (f = fopen(path, "rb")) == NULL) {
        fprintf(stderr, "ov_read(): could not open \"%s\"\n", file);
        return;
    }
    if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
        fclose(f);
        return;
    }
#define BE32(b) ((int)((b)[0] << 24 | (b)[1] << 16 | (b)[2] << 8 | (b)[3]))
    console_w = BE32(hdr);
    console_h = BE32(hdr + 4);
    size = BE32(hdr + 8);
    data = (unsigned char *)malloc(size);
    if (fread(data, 1, size, f) != (size_t)size) {
        free(data);
        fclose(f);
        return;
    }
    fclose(f);
    console = (unsigned char *)calloc(console_w * console_h, 4);
    for (y = 0; y < console_h && p < size; y++) {
        unsigned char *row = console + y * console_w * 4;
        if (data[p] == 0) {
            p++;
            continue;
        }
        for (x = 0; x < console_w && p + 1 < size; p += 2) {
            int n = data[p], c = data[p + 1];
            for (; n > 0 && x < console_w; n--, x++) {
                if (!c)
                    continue;
                memcpy(row + x * 4, hdr + 16 + c * 3, 3);
                row[x * 4 + 3] = 255;
            }
        }
    }
    free(data);
}

static void draw_console(void)
{
    if (!console)
        return;
    if (!console_tex) {
        glGenTextures(1, &console_tex);
        glBindTexture(GL_TEXTURE_2D, console_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, console_w, console_h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, console);
    }
    glBindTexture(GL_TEXTURE_2D, console_tex);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(1280, 0);
    glTexCoord2f(1, 1); glVertex2f(1280, 1024);
    glTexCoord2f(0, 1); glVertex2f(0, 1024);
    glEnd();
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

/* --- messages ------------------------------------------------------------- */

static const char *start_location_text[] = {
    "PICK YOUR STARTING LOCATION", " ", "0-9) For a specific location.",
    "space) For a ranndom location.", ""
};
static const char *pick_mekton_text[] = {
    "PICK YOUR MEKTON", " ", "1) Vantage", "2) Ex-Centinel", ""
};
static const char *time_limit_text[] = {
    "Select time limit", " ", "  1)  1 minute", "  2)  3 minutes",
    "  3)  5 minutes", "  4)  8 minutes", "  5) 10 minutes", "  6) 12 minutes",
    "  7) 15 minutes", "  8) 20 minutes", ""
};
static const char *shadow_text[] = {
    "   Shadow Mode Commands", " ", "space - shadow new mekton",
    "ESC   - quit", "Enter - Start UI", ""
};
static char status_buf[32];
static const char *status_text[] = { status_buf, "" };

static const char *credits[] = {
    "Author/Director: ", "Rob Mace, SGI", "",
    "Art Director: ", "Mike Pondsmith, RTG", "",
    "Original Art: ", "Don Choy, RTG", "Benjamin Wright, RTG",
    "J. Waltrip, RTG", "",
    "Content creation: ", "Buckley Collum, AW",
    "Kevin \"Bubba\" Lombardi, AW", "",
    "Contributors: ", "David Cooper, CSI", "David Ligon, SGI",
    "Michael McDonald, RTG", "",
    "Special Thanks to: ", "Alan Gerrard, CSI", "Andy Bushnell, CSI",
    "Bruce Sinclair", "Cindy Ferguson", "Dan Vivoli, SGI",
    "Doug Whetter, AW", "Ed Immenschuh, SGI", "H. B. Siegel, SGI",
    "Ian Clazie, SGI", "Jayne Capelin, CSI", "Jim Helman, SGI",
    "John Rohlf, SGI", "Justin Howell", "Kowsik Guruswami, CSI",
    "Lisa Pondsmith, RTG", "Megan Jewel Mace", "Michael Jones, SGI",
    "Paula Cutts, SGI", "Randy Fox, CSI", "Ritva Garlick, CSI",
    "Sharon Clay, SGI", "Warren Pratt, SGI", "",
    NULL
};

char team_names[3][22] = { "Rouge", "Terran Defence League", "Barnard Pirate" };

static const char score_line[] = "------------------------------------------";

/* messages from the other players */
typedef struct Msg {
    int id;
    float time;
    char from[16];
    char text[80];
} Msg;

typedef struct MsgList {
    int count, last, first;
    Msg msg[32];
} MsgList;

static MsgList message_list;
static int msg_ids;

void init_message_lists(void)
{
    memset(&message_list, 0, sizeof(message_list));
    message_list.last = 31;
    cd_app->msgs = &message_list;
}

/* adds a message shown for ten seconds */
void add_message(const char *from, const char *text)
{
    MsgList *l = &message_list;
    Msg *m;
    if (l->count < 32) {
        if (++l->last == 32)
            l->last = 0;
        l->count++;
    } else {
        l->last = l->first;
        if (++l->first == 32)
            l->first = 0;
    }
    m = &l->msg[l->last];
    m->time = game_time;
    m->id = ++msg_ids;
    memset(m->from, 0, sizeof(m->from));
    memset(m->text, 0, sizeof(m->text));
    if (from)
        strncpy(m->from, from, sizeof(m->from) - 1);
    strncpy(m->text, text, sizeof(m->text) - 1);
}

static void update_message_list(void)
{
    MsgList *l = &message_list;
    while (l->count > 0 && fabs(game_time - l->msg[l->first].time) > 10) {
        l->count--;
        if (++l->first == 32)
            l->first = 0;
    }
}

static void draw_comm_messages(int y)
{
    MsgList *l = cd_app->msgs;
    int i, k;
    glColor3ub(255, 255, 255);
    if (cd_app->shadow) {
        glRasterPos2i(400, y);
        if (number_mektons) {
            ogl_charstr("Shadowing: ");
            ogl_charstr(cd_app->mek->name);
        } else {
            ogl_charstr("No one to shadow.");
        }
    }
    if (!l)
        return;
    for (i = 0, k = l->first; i < l->count; i++, y -= 20) {
        glRasterPos2i(-630, y);
        if (l->msg[k].from[0]) {
            ogl_charstr("Message from ");
            ogl_charstr(l->msg[k].from);
            ogl_charstr(": ");
        }
        ogl_charstr(l->msg[k].text);
        if (++k == 32)
            k = 0;
    }
}

static void draw_message(HudGSet **g, int n)
{
    int i;
    ortho(-640, 640, -374.5, 374.5);
    glLoadIdentity();
    for (i = 0; i < n; i++)
        draw_gset(g[i]);
}

/* centered lines of text, from y down or centered around 0 if y is -10000 */
static void draw_text_message(const char **text, int y)
{
    int i, w = 0, n = 0, x;
    ortho(-640, 640, -374.5, 374.5);
    glLoadIdentity();
    glColor3ub(255, 255, 0);
    for (i = 0; text[i][0]; i++, n++)
        if (w < string_width(text[i]))
            w = string_width(text[i]);
    x = (int)(-(w / 2) / scale_x);
    if (y == -10000)
        y = n * 10;
    for (i = 0; text[i][0]; i++, y -= 20) {
        glRasterPos2i(x, y);
        ogl_charstr(text[i]);
    }
}

static void draw_score(int y)
{
    int w = string_width(score_line), i;
    int x_name, x_team, x_score, x_line;
    ChanData *cd = cd_app;
    char buf[32];
    if (y == -10000)
        y = cd->nmeks * 10 + 20;
    x_line = -w / 2;
    x_name = -w / 2;
    x_team = -w / 8;
    x_score = w * 3 / 8;
    glRasterPos2i(x_line, y);
    ogl_charstr("Name");
    glRasterPos2i(x_team, y);
    ogl_charstr("Team");
    glRasterPos2i(x_score, y);
    ogl_charstr("Score");
    y -= 20;
    glRasterPos2i(x_name, y);
    ogl_charstr(score_line);
    y -= 20;
    for (i = 0; i < cd->nmeks; i++, y -= 20) {
        glRasterPos2i(x_name, y);
        ogl_charstr(cd->name[i]);
        glRasterPos2i(x_team, y);
        ogl_charstr(team_names[cd->team[i] < 3 ? cd->team[i] : 0]);
        glRasterPos2i(x_score, y);
        sprintf(buf, "%d", cd->score[i]);
        ogl_charstr(buf);
    }
}

/* --- intro ---------------------------------------------------------------- */

static pfTexture *logo_tex;
static unsigned int *logo_image;
static int logo_xs, logo_ys;

static void init_logo(void)
{
    logo_tex = pfNewTex(NULL);
    if (pfLoadTexFile(logo_tex, "logo.rgb"))
        pfGetTexImage(logo_tex, &logo_image, NULL, &logo_xs, &logo_ys, NULL);
}

/* the authors, titles right aligned and names left aligned on the middle */
static void draw_people_credits(int y)
{
    int i, title = 1;
    glColor3ub(255, 255, 255);
    for (i = 0; credits[i]; i++) {
        if (!credits[i][0]) {
            title = 1;
            continue;
        }
        if (title) {
            /* the title ends where its first name starts */
            y -= 20;
            glRasterPos2i(-text_width(credits[i]), y);
            ogl_charstr(credits[i]);
            title = 0;
            continue;
        }
        glRasterPos2i(0, y);
        ogl_charstr(credits[i]);
        y -= 20;
    }
}

/* the logo, then the credits scrolling up as cd->fade grows */
static void draw_intro(void)
{
    int i;
    float fade = (float)cd_app->fade;
    ortho(-640, 640, -374.5 - fade, 374.5 - fade);
    glPushMatrix();
    glLoadIdentity();
    for (i = 0; i < 7; i++)
        draw_gset(intro_gsets[i]);
    if (logo_image) {
        glPixelZoom(scale_x, scale_y);
        glRasterPos2i(-(logo_xs >> 1), 329 - logo_ys);
        glDrawPixels(logo_xs, logo_ys, GL_RGBA, GL_UNSIGNED_BYTE, logo_image);
        glPixelZoom(1, 1);
    }
    if (cd_app->fade)
        draw_people_credits((int)(-374.5f + 50));
    glPopMatrix();
}

/* --- head up display ------------------------------------------------------ */

static const pfVec2 lo_arrow[3] = { { -5, 7.5f }, { -20, 0 }, { -5, -7.5f } };
static const pfVec2 ro_arrow[3] = { { 15, 7.5f }, { 30, 0 }, { 15, -7.5f } };
static const pfVec2 li_arrow[3] = { { 15, 5.5f }, { 3, 0 }, { 15, -5.5f } };
static float screen_scale = 640;     /* float_1003bfb8 */

static void arrow(const pfVec2 *a)
{
    glBegin(GL_LINE_STRIP);
    glVertex2fv(a[0]);
    glVertex2fv(a[1]);
    glVertex2fv(a[2]);
    glEnd();
}

/* arrow pointing right from x0 to x1 at height y */
static void in_arrow(float x0, float x1, float y)
{
    glBegin(GL_LINE_STRIP);
    glVertex2f(x0, y + 5.5f);
    glVertex2f(x1, y);
    glVertex2f(x0, y - 5.5f);
    glEnd();
}

static void bar(float x, float y)
{
    glBegin(GL_LINE_STRIP);
    glVertex2f(x, 0);
    glVertex2f(x, y);
    glEnd();
}

/* a number drawn with the line digits of numbers.gset, 10 units each */
static void draw_number(const char *s)
{
    int n = 0;
    while (s[n] == ' ')
        n++;
    s += n;
    if (n)
        glTranslatef(n * 10.0f, 0, 0);
    if (*s == '-') {
        draw_gset(numbers[10]);
        glTranslatef(10, 0, 0);
        s++;
    }
    for (; *s; s++) {
        if ('0' <= *s && *s <= '9')
            draw_gset(numbers[*s - '0']);
        else
            draw_gset(numbers[11]);
        if (s[1])
            glTranslatef(10, 0, 0);
    }
}

static void init_hud(void)
{
    static GLubyte stipple[128];
    memset(stipple, 0x55, sizeof(stipple));
    glPolygonStipple(stipple);
    glLineStipple(3, 0xcccc);
    glLineWidth(1);
    read_gsets("numbers.gset", numbers, 12);
    read_gsets("hud_static.gset", hud_static, 5);
    read_gsets("hud_ladder.gset", hud_ladder, 33);
    read_gsets("hud_heading.gset", hud_heading, 13);
}

static void draw_hud(void)
{
    ChanData *cd = cd_app;
    char buf[32];
    float y;
    int i, k;

    ortho(-640, 640, -374.5, 374.5);
    glPushMatrix();
    glLoadIdentity();
    draw_comm_messages((int)(374.5f - 50));
    draw_gset(hud_static[0]);
    if (cd->aim_x != 0 || cd->aim_y != 0) {
        glPushMatrix();
        glTranslatef(cd->aim_x, cd->aim_y, 0);
        draw_gset(hud_static[2]);
        glPopMatrix();
    }
    if (cd->hud < 2) {
        glPopMatrix();
        return;
    }

    /* the other mechs, enemies and team mates */
    for (i = 1; i < cd->nmeks; i++) {
        if (cd->spos[i][0] == -100000)
            continue;
        glPushMatrix();
        glTranslatef(cd->spos[i][0], cd->spos[i][1], 0);
        if (!cd->team[0] || cd->team[0] != cd->team[i])
            draw_gset(hud_static[3]);
        else
            draw_gset(hud_static[4]);
        glPopMatrix();
    }
    draw_gset(hud_static[1]);

    /* altitude */
    glPushMatrix();
    y = (cd->cpos[2] - 400) * 0.26f;
    if (130 < y)
        y = 130;
    glTranslatef(270, y, 0);
    arrow(lo_arrow);
    if (cd->hud >= 3) {
        sprintf(buf, "%d", (int)cd->cpos[2] + 100);
        draw_number(buf);
    }
    glPopMatrix();

    /* climb rate on the right */
    bar(220, cd->climb_bar);
    in_arrow(205, 217, cd->want_climb_bar);
    glPushMatrix();
    if (cd->hud >= 3) {
        sprintf(buf, "%1.1f", cd->climb);
        glTranslatef(200 - strlen(buf) * 10.0f, cd->climb_bar, 0);
        draw_number(buf);
    } else {
        glTranslatef(190, cd->climb_bar, 0);
    }
    arrow(ro_arrow);
    glPopMatrix();

    /* forward speed on the left */
    bar(-220, cd->fwd_bar);
    in_arrow(-205, -217, cd->want_fwd_bar);
    glPushMatrix();
    glTranslatef(-200, cd->fwd_bar, 0);
    arrow(lo_arrow);
    if (cd->hud >= 3) {
        sprintf(buf, "%1.1f", cd->fwd);
        draw_number(buf);
    }
    glPopMatrix();

    /* side speed */
    bar(-230, cd->side_bar);
    in_arrow(-245, -233, cd->want_side_bar);
    glPushMatrix();
    if (cd->hud >= 3) {
        sprintf(buf, "%1.1f", cd->side);
        glTranslatef(-250 - strlen(buf) * 10.0f, cd->side_bar, 0);
        draw_number(buf);
    } else {
        glTranslatef(-260, cd->side_bar, 0);
    }
    arrow(ro_arrow);
    glPopMatrix();
    (void)li_arrow;

    /* pitch ladder */
    centered_viewport(-130, 130, -130, 130);
    ortho(-25, 25, -25, 25);
    glPushMatrix();
    glRotatef(cd->chpr[2], 0, 0, 1);
    glTranslatef(0, -cd->chpr[1], 0);
    draw_gset(hud_ladder[0]);
    glEnable(GL_LINE_STIPPLE);
    draw_gset(hud_ladder[1]);
    glDisable(GL_LINE_STIPPLE);
    if (cd->hud >= 3) {
        float p = cd->chpr[1];
        k = (int)((0 <= p ? p + 10 : p) / 10) + 7;
        if (k < 2)
            k += 17;
        for (i = 0; i < 6; i++, k++) {
            if (20 <= k)
                k -= 17;
            draw_gset(hud_ladder[k]);
        }
    }
    glPopMatrix();

    /* heading */
    if (cd->hud >= 3) {
        centered_viewport(-130, 130, -205, -135);
        ortho(-45, 45, 31.77, 56);
        draw_gset(hud_heading[0]);
        glPushMatrix();
        glRotatef(-cd->chpr[0], 0, 0, 1);
        k = (int)(-cd->chpr[0] / 30) - 1;
        while (k <= 0)
            k += 12;
        for (i = 0; i < 4; i++, k++) {
            if (13 <= k)
                k -= 12;
            draw_gset(hud_heading[k]);
        }
        glPopMatrix();
    }
    chan_viewport();
    glPopMatrix();
}

/* random black outs of broken optics */
static void draw_damaged_vision(void)
{
    static int vision = 1;
    if (cd_app->damage & 32) {
        vision = 0;
    } else {
        int r = rand() % 5;
        if (r == 0 && vision > 0)
            vision--;
        else if (r == 1 && vision < 2)
            vision++;
    }
    if (vision == 0) {
        clear_viewport(0, 0, 0);
    } else if (vision == 1) {
        ortho(-1, 1, -1, 1);
        glLoadIdentity();
        glEnable(GL_POLYGON_STIPPLE);
        glColor3f(0.8f, 1, 1);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(-1, 1); glVertex2f(-1, -1); glVertex2f(1, 1); glVertex2f(1, -1);
        glEnd();
        glDisable(GL_POLYGON_STIPPLE);
    }
}

static void draw_hit_this_frame(void)
{
    ortho(-1, 1, -1, 1);
    glLoadIdentity();
    glEnable(GL_POLYGON_STIPPLE);
    glColor3ub(255, 0, 0);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(-1, 1); glVertex2f(-1, -1); glVertex2f(1, 1); glVertex2f(1, -1);
    glEnd();
    glDisable(GL_POLYGON_STIPPLE);
}

/* --- instruments ---------------------------------------------------------- */

typedef struct Meter {
    float x0, x1;
} Meter;

static const Meter power_meter = { 180, 205 }, thrust_meter = { 256, 281 };
static const Meter heat_meter = { 664, 689 }, armor_meter = { 829, 854 };

/* a gauge filled from 43 up to 174, red when the level is negative */
static void draw_level_meter(const Meter *m, float level)
{
    int red = level < 0;
    float y;
    if (red)
        level = -level;
    y = 131 * level + 43;
    if (level < 1) {
        glColor3ub(0, 0, 0);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(m->x0, y); glVertex2f(m->x1, y);
        glVertex2f(m->x0, 174); glVertex2f(m->x1, 174);
        glEnd();
    }
    if (0 < level) {
        if (red)
            glColor3ub(255, 0, 0);
        else
            glColor3ub(0, 0, 255);
        glBegin(GL_TRIANGLE_STRIP);
        glVertex2f(m->x0, 43); glVertex2f(m->x1, 43);
        glVertex2f(m->x0, y); glVertex2f(m->x1, y);
        glEnd();
    }
}

static void draw_level_meter_overlay(const Meter *m)
{
    glBegin(GL_LINE_STRIP);
    glVertex2f(m->x0, 43); glVertex2f(m->x1, 43);
    glVertex2f(m->x1, 174); glVertex2f(m->x0, 174);
    glVertex2f(m->x0, 43);
    glEnd();
}

static void centered_label(int x, int y, const char *s)
{
    glRasterPos2i(x - (text_width(s) >> 1), y);
    ogl_charstr(s);
}

static void draw_energy_status(void)
{
    draw_level_meter(&power_meter, 1);
    draw_level_meter(&thrust_meter, cd_app->throttle);
}

static void draw_weapon_status(void)
{
    float h = -cd_app->heat;
    draw_level_meter(&heat_meter, -1 < h ? h : -1);
}

static void draw_damage_status(void)
{
    ChanData *cd = cd_app;
    char buf[32];
    int f = cd->damage;
    if (f) {
        glColor3ub(255, 255, 0);
        if (f & 2) {
            glRasterPos2i(740, 150);
            ogl_charstr("Radar");
        }
        if (f & 4) {
            glRasterPos2i(740, 130);
            sprintf(buf, "Thrust %2d%%", (int)(cd->thrust * 100));
            ogl_charstr(buf);
        }
        if (f & 16) {
            glRasterPos2i(740, 110);
            ogl_charstr(f & 32 ? "Optics" : "Optics 50%");
        }
        if (f & 64) {
            glRasterPos2i(740, 90);
            ogl_charstr("Hud");
        }
        if (f & 128) {
            glRasterPos2i(740, 70);
            ogl_charstr("Plasma");
        }
    }
    draw_level_meter(&armor_meter, (0 < cd->armor ? cd->armor : 0) - 1);
}

static void draw_time_score(void)
{
    ChanData *cd = cd_app;
    char buf[32];
    glColor3ub(255, 255, 0);
    glRasterPos2i(text_width("Players: ") + 20, 110);
    sprintf(buf, "%2d", cd->nmeks);
    ogl_charstr(buf);
    glRasterPos2i(text_width("Score: ") + 20, 80);
    sprintf(buf, "%5d", cd->score[0]);
    ogl_charstr(buf);
    glRasterPos2i(text_width("Time: ") + 20, 50);
    sprintf(buf, "%2d:%02d", cd->time / 60, cd->time % 60);
    ogl_charstr(buf);
}

static const GLubyte blip[12] = { 0x40, 0, 0, 0, 0xe0, 0, 0, 0, 0x40, 0, 0, 0 };

static void radar_blips(pfVec2 *pos)
{
    ChanData *cd = cd_app;
    int i;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glColor3ub(255, 255, 0);
    glRasterPos2fv(pos[0]);
    glBitmap(3, 3, 1, 1, 0, 0, blip);
    for (i = 1; i < cd->nmeks; i++) {
        if (!cd->team[0] || cd->team[0] != cd->team[i])
            glColor3ub(255, 0, 0);
        else
            glColor3ub(0, 255, 0);
        glRasterPos2fv(pos[i]);
        glBitmap(3, 3, 1, 1, 0, 0, blip);
    }
}

/* both radars, the colony seen from its end and from above */
static void draw_c_radar(void)
{
    glPushMatrix();
    glTranslatef(463, 90, 0);
    glScalef(0.012f, 0.012f, 0.012f);
    draw_gset(v_radar[0]);
    radar_blips(cd_app->vpos);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(370, 90 - cd_app->hpos[0][1] * 0.04f, 0);
    glScalef(0.04f, 0.04f, 0.04f);
    draw_gset(h_radar[0]);
    radar_blips(cd_app->hpos);
    glPopMatrix();
}

/* the colony from above, turning with the mech */
static void draw_h_radar(void)
{
    float s = cd_app->radar_zoom * 0.04f;
    scaled_viewport(538, 733, 40, 229);
    ortho(0, 196, 0, 190);
    glPushMatrix();
    glTranslatef(101, 90, 0);
    glRotatef(-cd_app->chpr[0], 0, 0, 1);
    glScalef(s, s, s);
    glTranslatef(-cd_app->hpos[0][0], -cd_app->hpos[0][1], 0);
    draw_gset(h_radar[0]);
    radar_blips(cd_app->hpos);
    glPopMatrix();
}

/* the section of the colony, turning with it */
static void draw_v_radar(void)
{
    float s = cd_app->radar_zoom * 0.04f;
    scaled_viewport(538, 733, 40, 229);
    ortho(0, 196, 0, 190);
    glPushMatrix();
    glTranslatef(101, 50, 0);
    glRotatef(-cd_app->cpos[1] * 0.0116930166f, 0, 0, 1);
    glScalef(s, s, s);
    glTranslatef(-cd_app->vpos[0][0], -cd_app->vpos[0][1], 0);
    draw_gset(v_radar[0]);
    radar_blips(cd_app->vpos);
    glPopMatrix();
}

static void draw_radar(void)
{
    if (cd_app->damage & 2)
        return;
    switch (cd_app->radar) {
    case 0: draw_c_radar(); break;
    case 1: draw_h_radar(); break;
    case 2: draw_v_radar(); break;
    }
}

static void instrument_viewport(void)
{
    scaled_viewport(200, 1074, 40, 243);
    ortho(0, 875, 0, 204);
    glLoadIdentity();
}

static void draw_instruments(void)
{
    instrument_viewport();
    clear_viewport(0.05f, 0.1f, 0.05f);
    if (cd_app->state != STATE_PLAY)
        return;
    draw_energy_status();
    draw_weapon_status();
    draw_damage_status();
    draw_time_score();
    draw_radar();
}

/* the labels drawn in yellow in the overlay planes */
static void draw_instruments_overlay(void)
{
    instrument_viewport();
    glColor3ub(255, 255, 0);
    centered_label(230, 184, "Energy Status");
    centered_label(192, 25, "Power");
    centered_label(268, 25, "Thrust");
    draw_level_meter_overlay(&power_meter);
    draw_level_meter_overlay(&thrust_meter);
    centered_label(638, 184, "Weapon Status");
    centered_label(676, 25, "Heat");
    draw_level_meter_overlay(&heat_meter);
    centered_label(803, 184, "Damage Status");
    centered_label(841, 25, "Armor");
    draw_level_meter_overlay(&armor_meter);
    glRasterPos2i(20, 110);
    ogl_charstr("Players: ");
    glRasterPos2i(20, 80);
    ogl_charstr("Score: ");
    glRasterPos2i(20, 50);
    ogl_charstr("Time: ");
}

/* --- state passed from the game to the drawing ---------------------------- */

void init_instruments(void)
{
    cd_app->hud = 3;
    cd_app->radar = 0;
    cd_app->radar_zoom = 1;
    cd_app->help = 0;
}

void toggle_help(void)
{
    cd_app->help = !cd_app->help;
}

void toggle_hud(void)
{
    if (--cd_app->hud < 0)
        cd_app->hud = 3;
}

void toggle_radar(void)
{
    if (3 <= ++cd_app->radar)
        cd_app->radar = 0;
}

static float clamp130(float v)
{
    return 130 < v ? 130 : v < -130 ? -130 : v;
}

void pass_insrument_data(void)
{
    ChanData *cd = cd_app;
    Mek *m = my_mek;
    const MekType *mt = &mek_types[m->type];
    pfMatrix inv;
    float f, s, c;
    int i;

    cd->mek = m;
    cd->tps = tps;
    cd->status = m->status;
    cd->armor = m->f276;
    cd->damage = m->flags;
    cd->thrust = m->f280;
    cd->heat = m->heat;
    pfCopyVec3(cd->pos, m->pos);
    pfCopyVec3(cd->vel, m->vel);
    pfCopyVec3(cd->cpos, m->cpos);
    pfCopyVec3(cd->cpos2, m->cpos2);
    pfCopyVec3(cd->hpr, m->hpr);
    pfCopyVec3(cd->chpr, m->chpr);

    cd->climb = (m->cpos[2] - m->cpos2[2]) * tps;
    cd->climb_bar = cd->climb * 2.6f;
    f = m->vel[1];
    if (m->vmax[1] < f)
        f = m->vmax[1];
    else if (f < m->vmin[1])
        f = m->vmin[1];
    cd->fwd = f * tps;
    cd->fwd_bar = cd->fwd * 2.6f;
    cd->side = m->vel[0] * tps;
    cd->side_bar = cd->side * 2.6f;
    if (m->mode & MODE_WALK) {
        cd->throttle = 0;
        cd->want_fwd_bar = (0 <= m->in_fwd ? mt->max_fwd : -mt->max_back) * m->in_fwd * 2.6f;
        cd->want_side_bar = mt->strafe * m->in_side * 2.6f;
        cd->want_climb_bar = cd->climb_bar;
    } else if (m->mode & MODE_AIR) {
        cd->throttle = m->throttle;
        cd->want_fwd_bar = (0 <= m->in_fwd ? mt->jet_fwd : mt->jet_back) * m->in_fwd * 2.6f;
        cd->want_side_bar = mt->jet_side * m->in_side * 2.6f;
        cd->want_climb_bar = (0 < m->in_throttle ? mt->jet_up : mt->jet_down) * m->in_throttle * 2.6f;
    } else {
        cd->throttle = m->throttle;
        cd->want_fwd_bar = cd->fwd_bar;
        cd->want_side_bar = cd->side_bar;
        cd->want_climb_bar = cd->climb_bar;
    }
    cd->climb_bar = clamp130(cd->climb_bar);
    cd->want_climb_bar = clamp130(cd->want_climb_bar);
    cd->fwd_bar = clamp130(cd->fwd_bar);
    cd->want_fwd_bar = clamp130(cd->want_fwd_bar);
    cd->side_bar = clamp130(cd->side_bar);
    cd->want_side_bar = clamp130(cd->want_side_bar);

    cd->nmeks = number_mektons;
    cd->team[0] = m->team;
    cd->vpos[0][0] = m->pos[1];
    cd->vpos[0][1] = m->pos[2];
    cd->hpos[0][0] = m->cpos[0];
    cd->hpos[0][1] = m->cpos[1];

    /* where the mechs are on the screen */
    xyz_hpr2inv_mat(camera.xyz, camera.hpr, inv);
    pfSinCos(chan_fov * 0.5f, &s, &c);
    screen_scale = 640 / s * c;
    for (i = 0; i < number_mektons && i < MAX_MEKTONS; i++) {
        Mek *o = mektons[i];
        pfVec3 p;
        memcpy(cd->name[i], o->name, 16);
        cd->team[i] = o->team;
        cd->score[i] = o->score;
        cd->vpos[i][0] = o->pos[1];
        cd->vpos[i][1] = o->pos[2];
        cd->hpos[i][0] = o->cpos[0];
        cd->hpos[i][1] = o->cpos[1];
        pfXformPt3(p, o->pos, inv);
        cd->spos[i][0] = -100000;
        if (0 < p[1]) {
            float k = screen_scale / p[1];
            float x = p[0] * k, y = p[2] * k;
            if (-640 <= x && x <= 640 && -374.5f <= y && y <= 374.5f) {
                cd->spos[i][0] = x;
                cd->spos[i][1] = y;
            }
        }
    }

    /* the aim of the guns */
    if (m->aim[0] != 0) {
        pfSinCos(m->aim[0], &s, &c);
        cd->aim_x = s * (screen_scale / c);
    } else {
        cd->aim_x = 0;
    }
    if (m->aim[1] != 0) {
        pfSinCos(m->aim[1], &s, &c);
        cd->aim_y = s * (screen_scale / c);
    } else {
        cd->aim_y = 0;
    }
}

/* --- drawing -------------------------------------------------------------- */

static int hud_ready;

static void init_hud_drawing(void)
{
    hud_ready = 1;
    read_gsets("v_radar.gset", v_radar, 1);
    read_gsets("h_radar.gset", h_radar, 1);
    read_gsets("intro.gset", intro_gsets, 7);
    read_gsets("dead_message.gset", dead_gsets, 2);
    read_gsets("mission_complete.gset", complete_gsets, 2);
    read_gsets("help.gset", help_gsets, 3);
    init_hud();
    init_logo();
    ov_read("console.ov");
}

/* called after the scene is drawn in the channel */
void mek_post_draw(void)
{
    ChanData *cd = cd_app;

    if (!hud_ready)
        init_hud_drawing();
    update_message_list();
    pass_insrument_data();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    chan_viewport();

    switch (cd->state) {
    case STATE_INTRO:
        draw_intro();
        break;
    case STATE_WAIT:
        if (cd->shadow)
            draw_text_message(shadow_text, -10000);
        else
            draw_message(help_gsets, 3);
        break;
    case STATE_DEAD:
        draw_text_message(pick_mekton_text, -10000);
        break;
    case 4:
        draw_text_message(start_location_text, -210);
        draw_score(-10000);
        break;
    case 5:
        draw_text_message(time_limit_text, -10000);
        break;
    case STATE_PLAY:
        if ((cd->master || cd->slave) && cd->master_state != 2) {
            if (cd->slave)
                clear_viewport(0, 0, 0);
            switch (cd->master_state) {
            case 0: strcpy(status_buf, "Waiting for Start"); break;
            case 1: sprintf(status_buf, "Start in %d seconds", cd->time); break;
            case 3: strcpy(status_buf, "Game Over"); break;
            }
            draw_text_message(status_text, 250);
            draw_score(-10000);
        } else if (cd->status == 2) {
            draw_message(dead_gsets, 2);
        } else {
            if (cd->damage & 16)
                draw_damaged_vision();
            if (cd->damage & 1)
                draw_hit_this_frame();
            if (cd->help)
                draw_message(help_gsets, 3);
            else if (cd->hud && !(cd->damage & 64))
                draw_hud();
        }
        break;
    case 9:
        draw_message(complete_gsets, 2);
        break;
    }
    draw_instruments();

    /* the console and its labels, in the overlay planes in the original */
    glViewport(0, 0, win_w, win_h);
    ortho(0, 1280, 0, 1024);
    glLoadIdentity();
    draw_console();
    draw_instruments_overlay();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
    glViewport(0, 0, win_w, win_h);
}
