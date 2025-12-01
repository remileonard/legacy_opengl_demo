/*
 * Copyright (C) 1992, 1993, 1994, Silicon Graphics, Inc.
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
#include "space.h"

#define MENUCOUNT 17
#define XSCALE ((float)(Counter.winsizex / 1024.0))
#define YSCALE ((float)(Counter.winsizey / 1024.0))

static t_menu menu[MENUCOUNT] = {
    {10, 16, 100, 192 / 2 - 8, SP_IO_o, PRBIT_FLAG, 0xc0c0c0c0, "o", "Planet", "Orbit"},
    {10, 192 / 2 + 8, 100, 192 - 16, SP_IO_i, MRBIT_FLAG, 0xc0c0c0c0, "i", "Moon", "Orbit"},
    {110, 16, 200, 192 / 2 - 8, SP_IO_n, STNAM_FLAG, 0xc0c0c0c0, "n", "Star", "Names"},
    {110, 192 / 2 + 8, 200, 192 - 16, SP_IO_x, NOTXT_FLAG, 0xc0c0c0c0, "x", "No", "Text"},
    {210, 16, 300, 192 / 2 - 8, SP_IO_l, SHADE_FLAG, 0xc0c0c0c0, "l", "Always", "Shade"},
    {210, 192 / 2 + 8, 300, 192 - 16, SP_IO_v, VELOC_FLAG, 0xc0c0c0c0, "v", "Reverse", "Velocity"},
    {310, 16, 400, 192 / 2 - 8, SP_IO_q, PANEL_FLAG, 0xc0c0c0c0, "q", "Panel", "On/Off"},
    {310, 192 / 2 + 8, 400, 192 - 16, SP_IO_h, HELPP_FLAG, 0xc0c0c0c0, "h", "Help", ""},
    {410, 16, 500, 192 / 2 - 8, SP_IO_tid, TMREV_FLAG, 0xc0c0c0c0, "-", "Time", "Reverse"},
    {410, 192 / 2 + 8, 500, 192 - 16, SP_IO_y, 0, 0xc0c0c0c0, "y", "Time", "Reset"},
    {510, 48, 900, 192 / 2 - 8, SP_IO_tco, 0, 0xc0c0c0c0, "r,t", "Time", "Control"},
    {510, 192 / 2 + 8, 600, 192 - 16, SP_IO_a, AUTOP_FLAG, 0xc0c0c0c0, "a", "Auto", "Pilot"},
    {610, 192 / 2 + 8, 700, 192 - 16, SP_IO_s, STATS_FLAG, 0xc0c0c0c0, "s", "Stats", ""},
    {710, 192 / 2 + 8, 800, 192 - 16, SP_IO_z, ZODAC_FLAG, 0xc0c0c0c0, "z", "Zodiac", ""},
    {810, 192 / 2 + 8, 900, 192 - 16, SP_IO_b, HIRES_FLAG, 0xc0c0c0c0, "b", "Hi-Res", ""},
    {910, 16, 1000, 192 / 2 - 8, SP_IO_esc, 0, 0xc0c0c0c0, "Esc", "Quit", ""},
    {910, 192 / 2 + 8, 1000, 192 - 16, SP_IO_pri, PRINT_FLAG, 0xc0c0c0c0, "PrntScrn", "Image", "Snap/Quit"}};

extern long control_height;
extern t_stopwatch Counter;

/**********************************************************************
 *  draw_menu()  -
 ***********************************************************************/
void draw_menu()

{
    t_menu *m;
    sint32 i, save;

#ifdef SOUND
    save = 0;
    if ((Counter.alpha & HW_SOUND) && (Counter.flags & SOUND_FLAG)) {
        Counter.flags &= ~SOUND_FLAG;
        save = 1;
    }
#endif

    if (Counter.flags & PANEL_FLAG) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, Counter.winsizex, control_height);
        glViewport(0, 0, Counter.winsizex, control_height);

        glDrawBuffer(GL_FRONT_AND_BACK);
        glClearColor(0.0, 0.125, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawBuffer(GL_BACK);

        for (m = menu, i = 0; i < MENUCOUNT; m++, i++)
            draw_item(m->butt, m);

        glScissor(0, control_height, Counter.winsizex, Counter.winsizey - control_height);
        glViewport(0, control_height, Counter.winsizex, Counter.winsizey - control_height);
    } else {
        glScissor(0, 0, Counter.winsizex, Counter.winsizey);
        glViewport(0, 0, Counter.winsizex, Counter.winsizey);
        glDisable(GL_SCISSOR_TEST);
    }

#ifdef SOUND
    if (save)
        Counter.flags |= SOUND_FLAG;
#endif
}

/**********************************************************************
 *  check_menu()  -
 ***********************************************************************/
void check_menu(t_boss *flaggs)

{
    t_menu *m;
    sint32 x, y, i;

    x = (Counter.mouse_x - Counter.winorigx) / XSCALE;
    y = (Counter.mouse_y - Counter.winorigy) / YSCALE;

    for (m = menu, i = 0; i < MENUCOUNT; m++, i++)
        if (x >= m->x1 && x <= m->x2 && y >= m->y1 && y <= m->y2) {
            key_press(flaggs, m->butt);
            return;
        }
}

/**********************************************************************
 *  draw_item()  -
 ***********************************************************************/
void draw_item(sint32 but, t_menu *m)

{
    flot32 v[2], xx, yy;
    sint32 i, j;
    schar8 ch[32];
    uchar8 cl[4];
    uint32 col;

    if (m == 0)
        for (i = 0; i < MENUCOUNT; i++)
            if (menu[i].butt == but) {
                m = &menu[i];
                break;
            }
    if (m == 0)
        return;

    glScissor(0, 0, Counter.winsizex, control_height);
    glViewport(0, 0, Counter.winsizex, control_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1024.0, 0.0, 192.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDrawBuffer(GL_FRONT_AND_BACK);

    col = (Counter.flags & m->mask) ? m->col : 0x40404040;
    cl[0] = (col >> 0) & 0xff;
    cl[1] = (col >> 8) & 0xff;
    cl[2] = (col >> 16) & 0xff;
    glColor3ubv(cl);
    glRecti(m->x1 - 1, m->y1 - 1, m->x2 + 1, m->y2 + 1);

    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_STRIP);
    glVertex2i(m->x1 - 1, m->y1 - 1);
    glVertex2i(m->x2 + 1, m->y1 - 1);
    glVertex2i(m->x2 + 1, m->y2 + 1);
    glVertex2i(m->x1 - 1, m->y2 + 1);
    glVertex2i(m->x1 - 1, m->y1 - 1);
    glEnd();

    if (m->butt == SP_IO_tco) {
        glColor4f(0.0, 0.0, 0.0, 0.0);

        for (yy = -3.0; yy < 10.5; yy += 1.0) {
            i = m->x1 + 20.0 * yy + 76.0;

            glColor4ubv((unsigned char *)&m->col);
            if (yy == -3.0)
                spDrawString(i - 16, m->y1 - 16, 0.0, "Stop");
            else {
                sprintf(ch, "%1.0f", yy);
                spDrawString(i, m->y1 - 16, 0.0, ch);
            }
        }

        i = (Counter.mouse_x - Counter.winorigx) / XSCALE;
        j = (Counter.mouse_y - Counter.winorigy) / YSCALE;

        if (i >= m->x1 && i <= m->x2 && j >= m->y1 && j <= m->y2) {
            yy = (i - m->x1 - 76.0) / 20.0;

            if (yy < -3.0) {
                yy = -3.0;
                Counter.timacc = 0.0;
            } else if (yy > 10.0) {
                yy = 10.0;
                Counter.timacc = fexp(yy * flog(10.0));
            } else
                Counter.timacc = fexp(yy * flog(10.0));

            i = m->x1 + 20.0 * yy + 76.0;
        } else {
            xx = Counter.timacc;

            if (xx == 0.0)
                yy = -3.0;
            else
                yy = flog10(xx);

            i = m->x1 + 20.0 * yy + 76.0;
        }

        glScissor(m->x1 * XSCALE, m->y1 * YSCALE, (m->x2 - m->x1) * XSCALE, (m->y2 - m->y1) * YSCALE);

        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINES);
        v[0] = i;
        v[1] = m->y1 + 1;
        glVertex2fv(v);
        v[0] = i;
        v[1] = m->y2 - 2;
        glVertex2fv(v);
        glEnd();

        cl[0] = (m->col >> 0) & 0xff;
        cl[1] = (m->col >> 8) & 0xff;
        cl[2] = (m->col >> 16) & 0xff;
        glColor3ubv(cl);

        spDrawString(m->x2 - 64, m->y2 - 36, 0.0, m->mes0);
        spDrawString(m->x2 - 64, m->y2 - 12, 0.0, m->mes1);
        spDrawString(m->x2 - 64, m->y2 - 24, 0.0, m->mes2);
    } else {
        glScissor(m->x1 * XSCALE, m->y1 * YSCALE, (m->x2 - m->x1) * XSCALE, (m->y2 - m->y1) * YSCALE);

        col = (Counter.flags & m->mask) ? 0x0 : m->col;
        cl[0] = (col >> 0) & 0xff;
        cl[1] = (col >> 8) & 0xff;
        cl[2] = (col >> 16) & 0xff;
        glColor3ubv(cl);

        spDrawString(m->x1 + 8, m->y1 + 4, 0.0, m->mes0);
        spDrawString(m->x1 + 8, ((m->y1 + m->y2) >> 1) + 16, 0.0, m->mes1);
        spDrawString(m->x1 + 8, ((m->y1 + m->y2) >> 1) - 8, 0.0, m->mes2);
    }

    glDrawBuffer(GL_BACK);

    glScissor(0, control_height, Counter.winsizex, Counter.winsizey - control_height);
    glViewport(0, control_height, Counter.winsizex, Counter.winsizey - control_height);

#ifdef SOUND
    if ((Counter.alpha & HW_SOUND) && (Counter.flags & SOUND_FLAG)) {
        if (m->mask & AUTOP_FLAG) {
            if (Counter.flags & AUTOP_FLAG)
                sound_control(1, SND_AUON);
            else
                sound_control(1, SND_AUOF);
        } else
            sound_control(1, SND_BUTT);
    }
#endif
}

/**********************************************************************
 *  draw2_menu()  -
 ***********************************************************************/
void draw2_menu(sint32 count, t_menu *mn)

{
    sint32 i;
    uchar8 cl[4];

    glDrawBuffer(GL_FRONT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1280.0, 0.0, 832.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (i = 0; i < count; mn++, i++) {
        cl[0] = (mn->col >> 0) & 0xff;
        cl[1] = (mn->col >> 8) & 0xff;
        cl[2] = (mn->col >> 16) & 0xff;
        glColor3ubv(cl);
        glRecti(mn->x1 - 1, mn->y1 - 1, mn->x2 + 1, mn->y2 + 1);

        glColor4f(0.0, 0.0, 0.0, 0.0);
        glBegin(GL_LINES);
        glVertex2i(mn->x1 - 1, mn->y1 - 1);
        glVertex2i(mn->x2 + 1, mn->y1 - 1);
        glVertex2i(mn->x2 + 1, mn->y2 + 1);
        glVertex2i(mn->x1 - 1, mn->y2 + 1);
        glVertex2i(mn->x1 - 1, mn->y1 - 1);
        glEnd();

        spDrawString(mn->x1 + 4, mn->y1 + 4, 0.0, mn->mes0);
    }

    glDrawBuffer(GL_BACK);
}

/**********************************************************************
 *  check2_menu()  -
 **********************************************************************/
sint32 check2_menu(sint32 count, t_menu *mn)

{
    sint32 i, x, y;
    t_menu *m;

    while (1) {
        spWaitForLeftButton();

        if (Counter.flags & PANEL_FLAG) {
            x = (Counter.mouse_x - Counter.winorigx) * 1280 / Counter.winsizex;
            y = (Counter.mouse_y - Counter.winorigy - control_height) * 832 / (Counter.winsizey - control_height);
        } else {
            x = (Counter.mouse_x - Counter.winorigx) * 1280 / Counter.winsizex;
            y = (Counter.mouse_y - Counter.winorigy) * 832 / Counter.winsizey;
        }

        for (m = mn, i = 0; i < count; m++, i++)
            if (x >= m->x1 && x <= m->x2 && y >= m->y1 && y <= m->y2)
                return (i);
    }
}

/**********************************************************************
 *  make_new_item()  -
 **********************************************************************/
void make_new_item(t_menu *m, sint32 i, uint32 col, char *ch)

{
    m->x1 = (Counter.rotsizex >> 1) - 116;
    m->y1 = 684 - 20 * i;
    m->x2 = m->x1 + 232;
    m->y2 = m->y1 + 16;
    m->butt = 0;
    m->mask = 0;
    m->col = col;
    strcpy(m->mes0, ch);
}
