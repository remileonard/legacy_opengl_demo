/*
 * mek_stubs.c - parts of Mekton not ported yet.
 */
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <GL/freeglut.h>

#include <stdio.h>

#include "mekton.h"

void init_doors(void) {}
void update_doors(void) {}
void reset_doors(void) {}
void init_pods(void) {}
void start_pods(void) {}
void reset_pods(void) {}
void handle_pods(void) {}
void init_turrets(void) {}
void start_turrets(void) {}
void reset_turrets(void) {}
void handle_turrets(void) {}
void init_powerplant(void) {}
void init_projectiles(void) {}
void update_projectiles(void) {}
void reset_projectiles(void) {}
void init_explosions(void) {}
void update_explosions(void) {}
void update_plasma_objs(void) {}
void init_instruments(void) {}
void toggle_help(void) {}
void toggle_hud(void) {}
void toggle_radar(void) {}
void init_message_lists(void) {}
void draw_gas_giant(void) {}
void transform(void) {}
void animate_ex_centinel(Mek *m) { (void)m; }

static double game_start;

void init_game_time(void)
{
    game_start = pfGetTime();
}

void update_game_time(void)
{
    game_time = (float)(pfGetTime() - game_start);
}

/* the wreck of a dead Vantage is the first child of its model */
void switch_dead(Mek *m)
{
    if (m->type == VANTAGE)
        pfSwitchVal(pfGetChild(m->obj->model, 0),
                    m->status == 2 ? PFSWITCH_ON : PFSWITCH_OFF);
}

void destroy_mek(Mek *m)
{
    m->status = 2;
    m->mode &= 0xff87;
    switch_dead(m);
}

void enter_key(void)
{
    if (cd_app->state != STATE_SETUP)
        mek_restart();
}

void give_up(void)
{
    if (!shadow && cd_app->state == STATE_PLAY && my_mek->status != 2)
        destroy_mek(my_mek);
}

static void text(int x, int y, const char *s)
{
    glRasterPos2i(x, y);
    while (*s)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *s++);
}

void mek_post_draw(void)
{
    char buf[256];
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, win_w, 0, win_h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1, 1, 0);
    switch (cd_app->state) {
    case STATE_INTRO:
        text(win_w / 2 - 40, win_h / 2, "MEKTON");
        text(win_w / 2 - 100, win_h / 2 - 40, "press space");
        break;
    case STATE_WAIT:
        text(win_w / 2 - 100, win_h / 2, "help - press space");
        break;
    case STATE_DEAD:
        text(win_w / 2 - 160, win_h / 2, "1: Vantage   2: Ex-Centinel");
        break;
    case 4:
        text(win_w / 2 - 200, win_h / 2, "start position 0..9, space: random");
        break;
    case STATE_PLAY:
        sprintf(buf, "%s  pos %.0f %.0f %.0f  anim %d frame %.1f  tps %.0f",
                my_mek->name, my_mek->cpos[0], my_mek->cpos[1], my_mek->cpos[2],
                my_mek->anim, my_mek->frame, tps);
        text(10, 10, buf);
        break;
    }
}
