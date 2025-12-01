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

P4 feye;
V3 lit;
flot32 vids;
t_body *planet;

char mem_col_nme[32];
char mem_ele_nme[32];
char *mem_col_ptr;
char *mem_ele_ptr;

extern t_stopwatch Counter;
extern V4 plum[NUM_CLIP_PLANES];
extern t_galaga ggaa[STARSQ][STARSQ];
extern flot32 lut[4096];
extern char StarName[NUMBER_OF_STARS][16];
extern t_const constellation;
extern int GeosphereData;

/**********************************************************************
 *  actually_do_graphics()  -
 ***********************************************************************/
void actually_do_graphics(t_boss *flaggs)

{
    flot32 w;

    Counter.flags &= ~GEOSP_FLAG;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLoadMatrixf((float *)Counter.mat);

    switch (Counter.status) {
    case STELL_STAT:
        special_perspective(0.02, 1.0e6);
        spSetLight(&Counter.light_vector);

        display_stellar_background(flaggs);

        if (!(Counter.flags & (PRBIT_FLAG | MRBIT_FLAG) == (PRBIT_FLAG | MRBIT_FLAG)))
            display_foreground(flaggs);
        break;

    case GALAC_STAT:
        special_perspective(0.13, 33000.0);
        display_galactic_background();
        break;

    case COSMC_STAT:
        w = Counter.eye.x * Counter.eye.x + Counter.eye.y * Counter.eye.y + Counter.eye.z * Counter.eye.z;
        if (w > GALAXY_EDGE * GALAXY_EDGE / 4.0) {
            w = fsqrt(w);
            special_perspective(w - 0.5 * GALAXY_EDGE, w + 0.5 * GALAXY_EDGE);
        } else
            special_perspective(1.0, 33000.0);

        display_cosmic_background();
        break;

    default:
        printf("INTERNAL ERROR: actually_do_graphics() - Unsupported status\n");
        exit(0);
        break;
    }
}

/**********************************************************************
 *  display_stellar_background()  -
 **********************************************************************/
static void display_stellar_background(t_boss *flaggs)

{
    sint32 i, k, lim, dd[3];
    t_body *tb, *tc;
    uchar8 cl[4];

    if (Counter.flags & ZODAC_FLAG && Counter.z == SOL_Z_GRID && Counter.x == SOL_X_GRID && Counter.star_current == 0) {
        if (!(Counter.alpha & HW_MULSA) && (Counter.alpha & HW_AALIN)) {
            glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
            glEnable(GL_BLEND);
        }

        glCallList(Counter.constobj);

        if (!(Counter.alpha & HW_MULSA) && (Counter.alpha & HW_AALIN)) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDisable(GL_BLEND);
        }
    }

    if (Counter.flags & STNAM_FLAG)
        display_star_names();

    if (Counter.skyfrac < 160.0) {
        glCallList(Counter.locun);

        if (Counter.alpha & HW_AAPNT)
            glEnable(GL_BLEND);

        glBegin(GL_POINTS);
        for (k = 0; k < flaggs->suncount; k++) {
            for (i = 0; i < flaggs->star[k].ptr->moon_count; i++) {
                tb = (t_body *)flaggs->star[k].next[i];

                if ((i != flaggs->plan_current || k != flaggs->suun_current) || (tb->angsiz < ANGULAR_SIZE)) {
                    cl[0] = (tb->color >> 0) & 0xff;
                    cl[1] = (tb->color >> 8) & 0xff;
                    cl[2] = (tb->color >> 16) & 0xff;
                    cl[3] = (tb->color >> 24) & 0xff;
                    glColor4ubv(cl);
                    glVertex3dv(&tb->posit.x);
                }
            }

            if (flaggs->star[k].angsiz < ANGULAR_SIZE) {
                cl[0] = (flaggs->star[k].ptr->col >> 0) & 0xff;
                cl[1] = (flaggs->star[k].ptr->col >> 8) & 0xff;
                cl[2] = (flaggs->star[k].ptr->col >> 16) & 0xff;
                cl[3] = (flaggs->star[k].ptr->col >> 24) & 0xff;
                glColor4ubv(cl);
                glVertex3dv(&flaggs->star[k].posit.x);
            }
        }
        glEnd();

        if (Counter.alpha & HW_AAPNT)
            glDisable(GL_BLEND);
    }

    /* draw sun(s) */
    if (Counter.flags & RINGW_FLAG)
        ;
    else {
        if (flaggs->suncount == 2 && flaggs->star[1].distan > flaggs->star[0].distan) {
            dd[0] = flaggs->suncount - 1;
            dd[1] = -1;
            dd[2] = -1;
        } else {
            dd[0] = 0;
            dd[1] = flaggs->suncount;
            dd[2] = 1;
        }

        for (k = dd[0]; k != dd[1]; k += dd[2])
            if (flaggs->star[k].angsiz >= ANGULAR_SIZE) {
                if ((Counter.flags & ECLIP_FLAG) && !(Counter.flags & (PRBIT_FLAG | MRBIT_FLAG))) {
                    if (Counter.feclipse > 0.9)
                        total_solar_eclipse(flaggs);
                    if (Counter.feclipse < 1.0)
                        draw_me(&flaggs->star[k], flaggs);
                } else
                    draw_me(&flaggs->star[k], flaggs);
            }
    }

    /* draw away moons */
    if (flaggs->plan_current >= 0 && !(Counter.flags & MRBIT_FLAG)) {
        tb = (t_body *)flaggs->star[flaggs->suun_current].next[flaggs->plan_current];

        if (Counter.alpha & HW_AAPNT)
            glEnable(GL_BLEND);

        /* draw point moons */
        glBegin(GL_POINTS);
        for (i = 0; i < tb->ptr->moon_count; i++) {
            tc = (t_body *)tb->next[i];

            if (tc->angsiz < ANGULAR_SIZE && tc->distan > tb->distan) {
                cl[0] = (tc->color >> 0) & 0xff;
                cl[1] = (tc->color >> 8) & 0xff;
                cl[2] = (tc->color >> 16) & 0xff;
                cl[3] = (tc->color >> 24) & 0xff;
                glColor4ubv(cl);
                glVertex3dv(&tc->posit.x);
            }
        }
        glEnd();

        if (Counter.alpha & HW_AAPNT)
            glDisable(GL_BLEND);

        /* draw sphere moons */
        for (i = 0; i < tb->ptr->moon_count; i++) {
            tc = (t_body *)tb->next[i];

            if (tc->angsiz >= ANGULAR_SIZE && tc->distan > tb->distan)
                draw_me(tc, flaggs);
        }
    }

    if (Counter.flags & (PRBIT_FLAG | MRBIT_FLAG))
        polyline_orbits(flaggs);
}

/**********************************************************************
 *  display_galactic_background()  -
 **********************************************************************/
static void display_galactic_background(void)

{
    if (Counter.flags & STNAM_FLAG)
        display_star_names();

    draw_all_them_stars(1);
}

/**********************************************************************
 *  display_cosmic_background()  -
 **********************************************************************/
static void display_cosmic_background(void)

{
    flot32 scsps[8];

    glShadeModel(GL_SMOOTH);
    glPushMatrix();
    glTranslatef(-Counter.eye.x, -Counter.eye.y, -Counter.eye.z);

    if (Counter.flags & TEXTR_FLAG) {
        glEnable(GL_DEPTH_TEST);
        spFlipTex(1, Counter.galaxy);

        glColor4f(1.0, 1.0, 1.0, 1.0);

        glCullFace(GL_FRONT);
        glCallList(Counter.galobj[1]);
        glCullFace(GL_BACK);
        glScalef(1.0, -1.0, 1.0);
        glCallList(Counter.galobj[1]);

        spFlipTex(0, 0);
        glDisable(GL_DEPTH_TEST);
    } else if (Counter.flags & SLOWZ_FLAG) {
        glCullFace(GL_FRONT);
        glScalef(1.0, 0.0, 1.0);
        glCallList(Counter.galobj[0]);
        glCullFace(GL_BACK);
    } else {
        glEnable(GL_DEPTH_TEST);

        glCullFace(GL_FRONT);
        glCallList(Counter.galobj[0]);
        glCullFace(GL_BACK);
        glScalef(1.0, -1.0, 1.0);
        glCallList(Counter.galobj[0]);

        glDisable(GL_DEPTH_TEST);
    }

    glPopMatrix();
    glShadeModel(GL_FLAT);
}

/**********************************************************************
 *  display_foreground()  -
 **********************************************************************/
static void display_foreground(t_boss *flaggs)

{
    sint32 i, draw_st, draw_pl;
    flot32 w, z, min, max;
    flot64 theta;
    t_body *tb, *tc;
    uchar8 cl[4];

    if (flaggs->plan_current < 0)
        return;

    tb = (t_body *)flaggs->star[flaggs->suun_current].next[flaggs->plan_current];

    if (((tb->ptr->tess >> 4) & 0x0f) == FRAC_RINGWO) {
        display_ringworld_foreground(flaggs);
        return;
    }

    if (!(Counter.flags & PRBIT_FLAG)) {
        draw_st = 0;
        draw_pl = 0;

        /* check space station drawing */
        if (Counter.star_current == 0 && flaggs->plan_current == 2 && flaggs->stat.angsiz >= 2.0 * ANGULAR_SIZE)
            if (sphere_clipp(&flaggs->stat.posit, flaggs->stat.cliprad))
                draw_st = 1;

        i = (tb->ptr->tess & 0x00f0) >> 4;
        if (tb->ptr->r2 != 0.0)
            draw_pl |= 0x1;
        if (i == FRAC_PLANET || i == ELEV_PLANET)
            draw_pl |= 0x2;

        if (tb->ptr->r2 != 0.0) {
            max = 1.05 * (tb->distan + tb->ptr->r2);
            min = 0.90 * (tb->distan - tb->ptr->r2);
        } else {
            max = 1.05 * (tb->distan + tb->ptr->rad);
            min = 0.90 * (tb->distan - 1.25 * tb->ptr->rad);
        }

        if (draw_st)
            min = 10.0;
        else if (min < 1.0)
            min = 1.0;

        if (draw_st || draw_pl & 0x1)
            glEnable(GL_DEPTH_TEST);
        else if (draw_pl & 0x2) {
            if (Counter.flags & SLOWZ_FLAG) {
                if (vids < 2.5)
                    glEnable(GL_DEPTH_TEST);
            } else
                glEnable(GL_DEPTH_TEST);
        }

        special_perspective(min, max);

        /* draw space station */
        if (draw_st) {
            spLightMaterial(1, ~0);

            glPushMatrix();
            glTranslatef(flaggs->stat.posit.x, flaggs->stat.posit.y, flaggs->stat.posit.z);

            theta = 24.0 * 60.0 * Counter.D;
            i = theta;
            theta = 360.0 * (theta - i);
            glRotatef(theta, 0.0, 1.0, 0.0);

            glShadeModel(GL_SMOOTH);
            spFlipTex(1, flaggs->stat.texdf);

            if (flaggs->stat.angsiz >= 0.08)
                glCallList(flaggs->stat.bodyobj[0]);
            else if (flaggs->stat.angsiz >= 0.015)
                glCallList(flaggs->stat.bodyobj[1]);
            else
                glCallList(flaggs->stat.bodyobj[2]);

            spFlipTex(0, 0);
            glShadeModel(GL_FLAT);

            glPopMatrix();

            spLightMaterial(0, ~0);
        }

        /* draw planet */
        if (tb->angsiz >= ANGULAR_SIZE)
            draw_me(tb, flaggs);

        glDisable(GL_DEPTH_TEST);
    }

    /* draw near moons */
    if (!(Counter.flags & MRBIT_FLAG)) {
        special_perspective(0.02, 1.0e6);

        /* draw point moons */
        if (Counter.alpha & HW_AAPNT)
            glEnable(GL_BLEND);

        glBegin(GL_POINTS);
        for (i = 0; i < tb->ptr->moon_count; i++) {
            tc = (t_body *)tb->next[i];

            if (tc->angsiz < ANGULAR_SIZE && tc->distan <= tb->distan) {
                cl[0] = (tc->color >> 0) & 0xff;
                cl[1] = (tc->color >> 8) & 0xff;
                cl[2] = (tc->color >> 16) & 0xff;
                cl[3] = (tc->color >> 24) & 0xff;
                glColor4ubv(cl);
                glVertex3dv(&tc->posit.x);
            }
        }
        glEnd();

        if (Counter.alpha & HW_AAPNT)
            glDisable(GL_BLEND);

        /* draw sphere moons */
        for (i = 0; i < tb->ptr->moon_count; i++) {
            tc = (t_body *)tb->next[i];

            if (tc->angsiz >= ANGULAR_SIZE && tc->distan <= tb->distan)
                draw_me(tc, flaggs);
        }
    }
}

/**********************************************************************
 *  display_ringworld_foreground()  -
 **********************************************************************/
static void display_ringworld_foreground(t_boss *flaggs)

{
    sint32 i;
    flot32 w, z, arr[4];
    flot64 theta;
    t_body *tb;
    D3 *p;
    ;

    tb = (t_body *)flaggs->star[flaggs->suun_current].next[flaggs->plan_current];

    w = flaggs->star[flaggs->suun_current].distan;
    z = 1.05 * tb->ptr->orb;

    if (w < z)
        special_perspective(1.0e2, z + z);
    else
        special_perspective(w - z, w + z);

    glEnable(GL_DEPTH_TEST);

    if (!(Counter.flags & PRBIT_FLAG))
        draw_me(tb, flaggs);

    if (flaggs->star[flaggs->suun_current].angsiz >= ANGULAR_SIZE) {
        glDepthFunc(GL_ALWAYS);

        theta = 1.5 * Counter.D / tb->ptr->yer;
        i = theta;
        theta = 360.0 * (theta - i);

        arr[0] = Counter.sky_clear_color[0];
        arr[1] = Counter.sky_clear_color[1];
        arr[2] = 0.06 + 0.94 * Counter.sky_clear_color[2];
        arr[3] = Counter.sky_clear_color[3];
        glColor4fv(arr);

        p = &flaggs->star[flaggs->suun_current].posit;

        glPushMatrix();
        glTranslatef(p->x, p->y, p->z);
        glScalef(20000000.0, 20000000.0, 20000000.0);
        glRotatef((flot32)theta, 0.0, 1.0, 0.0);
        glCullFace(GL_FRONT);
        glCallList(Counter.shadowsqobj);
        glCullFace(GL_BACK);
        glPopMatrix();

        draw_me(&flaggs->star[flaggs->suun_current], flaggs);

        arr[0] = Counter.sky_clear_color[0];
        arr[1] = Counter.sky_clear_color[1];
        arr[2] = 0.02 + 0.98 * Counter.sky_clear_color[2];
        arr[3] = Counter.sky_clear_color[3];
        glColor4fv(arr);

        glPushMatrix();
        glTranslatef(p->x, p->y, p->z);
        glScalef(20000000.0, 20000000.0, 20000000.0);
        glRotatef((flot32)theta, 0.0, 1.0, 0.0);
        glCallList(Counter.shadowsqobj);
        glPopMatrix();

        glDepthFunc(GL_LEQUAL);
    }

    glDisable(GL_DEPTH_TEST);
}

/**********************************************************************
 *  draw_me()  -
 **********************************************************************/
static void draw_me(t_body *body, t_boss *flaggs)

{
    flot64 theta;
    flot32 whop;
    sint32 i, bobj, style, seed, *q = (sint32 *)&whop;
    t_stars *s;
    Matrix m;
    uchar8 cl[3];

    if (sphere_clipp(&body->posit, body->cliprad)) {
        if (body->angsiz > 0.03)
            bobj = body->bodyobj[0];
        else if (body->angsiz > 0.003)
            bobj = body->bodyobj[1];
        else
            bobj = body->bodyobj[2];

        glPushMatrix();
        switch ((style = (body->ptr->tess & 0x00f0) >> 4)) {

        /* Flat Sphere */
        case FLAT_SPHERE:
            glTranslatef(body->posit.x, body->posit.y, body->posit.z);
            glScalef(body->ptr->rad, body->ptr->rad, body->ptr->rad);
            spTranMatrix((flot32 *)m, (flot32 *)Counter.mat);
            glMultMatrixf((float *)m);

            cl[0] = (body->ptr->col >> 0) & 0xff;
            cl[1] = (body->ptr->col >> 8) & 0xff;
            cl[2] = (body->ptr->col >> 16) & 0xff;
            glColor3ubv(cl);
            glCallList(bobj);
            break;

        /* Lit Sphere */
        case LIGT_SPHERE:
            glTranslatef(body->posit.x, body->posit.y, body->posit.z);
            glScalef(body->ptr->rad, body->ptr->rad, body->ptr->rad);
            spTranMatrix((flot32 *)m, (flot32 *)Counter.mat);
            glMultMatrixf((float *)m);

            glShadeModel(GL_SMOOTH);

            spLightMaterial(1, body->ptr->col);
            glCallList(bobj);
            spLightMaterial(0, ~0);

            glShadeModel(GL_FLAT);
            break;

        /* Textured Sphere */
        case TEXX_SPHERE:
            glTranslatef(body->posit.x, body->posit.y, body->posit.z);
            glScalef(body->ptr->rad, body->ptr->rad, body->ptr->rad);

            glRotatef(-body->ptr->apo, 0.0, 0.0, 1.0);
            theta = Counter.D / (body->ptr->day - 1.0 / body->ptr->yer);
            i = theta;
            theta = 360.0 * (theta - i);
            glRotatef((flot32)theta, 0.0, 1.0, 0.0);

            glShadeModel(GL_SMOOTH);
            spLightMaterial(1, body->ptr->col);

            if (!strcmp(body->ptr->name, "Freedom")) {
                glDisable(GL_CULL_FACE);
#if 0
                           Ydraw();
#endif
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);
            } else {
                spFlipTex(1, body->texdf);
                glColor4f(1.0, 1.0, 1.0, 1.0);
                glCallList(bobj);
                spFlipTex(0, 0);
            }

            spLightMaterial(0, ~0);
            glShadeModel(GL_FLAT);
            break;

        /* Fractal/Elevation Planet */
        case FRAC_PLANET:
        case ELEV_PLANET:
            planet = body;
            glTranslatef(body->posit.x, body->posit.y, body->posit.z);
            glScalef(body->ptr->rad, body->ptr->rad, body->ptr->rad);

            glRotatef(-body->ptr->apo, 0.0, 0.0, 1.0);
            theta = Counter.D / (body->ptr->day - 1.0 / body->ptr->yer);
            i = theta;
            theta = 360.0 * (theta - i);
            glRotatef((flot32)theta, 0.0, 1.0, 0.0);

            if (Counter.flags & REGEN_FLAG) {
                destroy_fractsphere();
                if (style == ELEV_PLANET)
                    read_data_file(body);
                s = &ggaa[Counter.z][Counter.x].stars[Counter.star_current];
                whop = s->x * s->y * s->z + flaggs->suun_current;
                generate_fractsphere(4, *q);
                Counter.flags &= ~REGEN_FLAG;
            }

            prepare_for_fractal_planet(flaggs, flaggs->plan_current);
            display_fractsphere();

            if (strcmp(body->ptr->name, "Terra") == 0 && GeosphereData)
                Counter.flags |= GEOSP_FLAG;

            break;

        case FRAC_RINGWO:
        case ELEV_RINGWO:
            planet = body;
            glTranslatef(flaggs->star[0].posit.x, flaggs->star[0].posit.y, flaggs->star[0].posit.z);
            glScalef(body->ptr->orb, body->ptr->orb, body->ptr->orb);

            theta = Counter.D / body->ptr->yer;
            i = theta;
            theta = 360.0 * (theta - i);
            glRotatef((flot32)theta, 0.0, 1.0, 0.0);

            if (Counter.flags & REGEN_FLAG) {
                destroy_ringworld();
                s = &ggaa[Counter.z][Counter.x].stars[Counter.star_current];
                seed = (flot32)s->x * s->y * s->z * 1234.0;
                generate_ringworld(4, seed);
                Counter.flags &= ~REGEN_FLAG;
            }

            prepare_for_fractal_ringworld(flaggs, flaggs->plan_current);
            display_ringworld();
            break;

        default:
            printf("Cannot draw this option\n");
            exit(0);
            break;
        }
        glPopMatrix();

        if (body->ptr->r1 != 0.0 && body->ptr->r2 != 0.0) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);
            glDisable(GL_CULL_FACE);

            glPushMatrix();
            glTranslatef(body->posit.x, body->posit.y, body->posit.z);
            glRotatef(-body->ptr->apo, 0.0, 0.0, 1.0);
            glRotatef(Counter.light_angle, 0.0, 1.0, 0.0);

            spFlipTex(1, body->texrn);

            if (body->angsiz > 0.15)
                glCallList(body->ringobj[0]);
            else if (body->angsiz > 0.01)
                glCallList(body->ringobj[1]);
            else
                glCallList(body->ringobj[2]);

            spFlipTex(0, 0);

            glPopMatrix();

            glEnable(GL_CULL_FACE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDisable(GL_BLEND);
        }
    }
}

/**********************************************************************
 *  sphere_clipp()  -
 **********************************************************************/
static sint32 sphere_clipp(D3 *p, flot32 rad)

{
    flot32 x, y, z, o;
    V4 *c = Counter.clop;

    x = p->x;
    y = p->y;
    z = p->z;
    o = -rad;

    if (c->x * x + c->y * y + c->z * z + c->w > o) {
        c++;
        if (c->x * x + c->y * y + c->z * z + c->w > o) {
            c++;
            if (c->x * x + c->y * y + c->z * z + c->w > o) {
                c++;
                if (c->x * x + c->y * y + c->z * z + c->w > o) {
                    c++;
                    if (c->x * x + c->y * y + c->z * z + c->w > o)
                        return (1);
                }
            }
        }
    }

    return (0);
}

/**********************************************************************
 *  prepare_for_fractal_planet()  -
 **********************************************************************/
static void prepare_for_fractal_planet(t_boss *flaggs, sint32 i)

{
    flot64 x, y, z, c1, s1, w, u;
    Matrix ma, mb;
    sint32 j, b;
    t_system *planet;
    t_body *tb;
    V4 *clop = Counter.clop;

    tb = (t_body *)flaggs->star[flaggs->suun_current].next[i];
    planet = tb->ptr;

    x = -tb->posit.x / planet->rad;
    y = -tb->posit.y / planet->rad;
    z = -tb->posit.z / planet->rad;

    /*** rot((flot32)theta,'y') ; ***/
    w = -2.0 * M_PI * Counter.D / (planet->day - 1.0 / planet->yer);
    c1 = cos(w);
    s1 = sin(w);
    ma[0][0] = c1;
    ma[0][1] = 0.0;
    ma[0][2] = -s1;
    ma[0][3] = 0.0;
    ma[1][0] = 0.0;
    ma[1][1] = 1.0;
    ma[1][2] = 0.0;
    ma[1][3] = 0.0;
    ma[2][0] = s1;
    ma[2][1] = 0.0;
    ma[2][2] = c1;
    ma[2][3] = 0.0;
    ma[3][0] = 0.0;
    ma[3][1] = 0.0;
    ma[3][2] = 0.0;
    ma[3][3] = 1.0;

    /*** rot(body->ptr->apo,'z') ; ***/
    w = DTOR * planet->apo;
    c1 = cos(w);
    s1 = sin(w);
    mb[0][0] = c1;
    mb[0][1] = s1;
    mb[0][2] = 0.0;
    mb[0][3] = 0.0;
    mb[1][0] = -s1;
    mb[1][1] = c1;
    mb[1][2] = 0.0;
    mb[1][3] = 0.0;
    mb[2][0] = 0.0;
    mb[2][1] = 0.0;
    mb[2][2] = 1.0;
    mb[2][3] = 0.0;
    mb[3][0] = 0.0;
    mb[3][1] = 0.0;
    mb[3][2] = 0.0;
    mb[3][3] = 1.0;
    spMultMatrix((flot32 *)ma, (flot32 *)mb, (flot32 *)ma);

    feye.x = ma[0][0] * x + ma[1][0] * y + ma[2][0] * z;
    feye.y = ma[0][1] * x + ma[1][1] * y + ma[2][1] * z;
    feye.z = ma[0][2] * x + ma[1][2] * y + ma[2][2] * z;
    feye.w = fsqrt(feye.x * feye.x + feye.y * feye.y + feye.z * feye.z);

    x = flaggs->star[flaggs->suun_current].posit.x - tb->posit.x;
    y = flaggs->star[flaggs->suun_current].posit.y - tb->posit.y;
    z = flaggs->star[flaggs->suun_current].posit.z - tb->posit.z;

    lit.x = ma[0][0] * x + ma[1][0] * y + ma[2][0] * z;
    lit.y = ma[0][1] * x + ma[1][1] * y + ma[2][1] * z;
    lit.z = ma[0][2] * x + ma[1][2] * y + ma[2][2] * z;
    w = sqrt(lit.x * lit.x + lit.y * lit.y + lit.z * lit.z);
    lit.x /= w;
    lit.y /= w;
    lit.z /= w;

    w = 256.0 * (2.0 - feye.w);
    if (w < 0.0)
        w = 0.0;
    if (w > 255.0)
        w = 255.0;

    Counter.skyfrac = 0.0;
    u = (lit.x * feye.x + lit.y * feye.y + lit.z * feye.z) / feye.w;

    if (u > -0.25) {
        if (Counter.flags & ECLIP_FLAG)
            w *= 1.0 - Counter.feclipse;

        Counter.skyfrac = w * sqrt(0.8 * (u + 0.25));
    }

    Counter.sky_clear_color[0] = Counter.skyfrac / 512.0;
    Counter.sky_clear_color[1] = Counter.skyfrac / 512.0;
    Counter.sky_clear_color[2] = Counter.skyfrac / 255.0;
    Counter.sky_clear_color[3] = 1.0;

    w = 1.0 / tb->angsiz;
    vids = w - 1.0;

    for (j = 0; j < NUM_CLIP_PLANES; j++) {
        plum[j].x = ma[0][0] * clop[j].x + ma[1][0] * clop[j].y + ma[2][0] * clop[j].z;
        plum[j].y = ma[0][1] * clop[j].x + ma[1][1] * clop[j].y + ma[2][1] * clop[j].z;
        plum[j].z = ma[0][2] * clop[j].x + ma[1][2] * clop[j].y + ma[2][2] * clop[j].z;
        plum[j].w = -(plum[j].x * feye.x + plum[j].y * feye.y + plum[j].z * feye.z);
    }
}

/**********************************************************************
 *  calc_posit()  -
 **********************************************************************/
void calc_posit(D3 *p, t_system *planet, D3 *center, flot32 tilt)

{
    flot64 M, v, E, F, l, e, r, sp, cp, ll;
    sint32 loop;
    Matrix ma;

    if (strcmp(planet->name, "Luna") == 0) {
        calculate_luna(p, center);
        return;
    }

    e = planet->ecc;

    if (e > 0.80) {
        M = 2.0 * M_PI * (Counter.D - planet->ee) / planet->yer;

        loop = (flot32)e * 20.0;

        for (E = M; loop > 0; loop--)
            E = E - (M - E + e * sin(E)) / (e * cos(E) - 1.0);

        v = 2.0 * atan(sqrt((1 + e) / (1 - e)) * tan(0.5 * E));
    } else {
        M = (planet->ee - planet->ww) / 360.0 + Counter.D / planet->yer;
        M = M - (sint32)M;
        M = 2.0 * M_PI * M;

        if (e != 0.0) {
            loop = (flot32)e * 20.0;

            for (E = M; loop > 0; loop--)
                E = M + e * sin(E);

            v = 2.0 * atan(sqrt((1 + e) / (1 - e)) * tan(0.5 * E));
        }

        else
            v = E = M;
    }

    l = DTOR * planet->ww + v;
    r = planet->orb * (1.0 - e * cos(E));

    F = l - DTOR * planet->omega;
    ll = atan2(cos(DTOR * planet->inc) * sin(F), cos(F)) + DTOR * planet->omega;

    sp = sin(F) * sin(DTOR * planet->inc);
    cp = sqrt(1.0 - sp * sp);

    if (tilt == 0.0) {
        p->x = center->x + r * cp * sin(ll);
        p->y = center->y + r * sp;
        p->z = center->z + r * cp * cos(ll);
    } else {
        E = r * cp * sin(ll);
        F = r * sp;
        M = r * cp * cos(ll);

        cp = cos(-DTOR * tilt);
        sp = sin(-DTOR * tilt);
        ma[0][0] = cp;
        ma[0][1] = sp;
        ma[0][2] = 0.0;
        ma[1][0] = -sp;
        ma[1][1] = cp;
        ma[1][2] = 0.0;
        ma[2][0] = 0.0;
        ma[2][1] = 0.0;
        ma[2][2] = 1.0;

        p->x = center->x + ma[0][0] * E + ma[1][0] * F + ma[2][0] * M;
        p->y = center->y + ma[0][1] * E + ma[1][1] * F + ma[2][1] * M;
        p->z = center->z + ma[0][2] * E + ma[1][2] * F + ma[2][2] * M;
    }
}

/**********************************************************************
 *  polyline_orbits()  -
 **********************************************************************/
static void polyline_orbits(t_boss *flaggs)

{
    sint32 i, k;
    t_body *tb, *tc;

    if (Counter.star_current < 0)
        return;

    if (!(Counter.alpha & HW_MULSA) && (Counter.alpha & HW_AALIN))
        glEnable(GL_BLEND);

    if (Counter.flags & PRBIT_FLAG) {
        for (k = 0; k < flaggs->suncount; k++) {
            glColor4f(0.0, 1.0, 0.0, 1.0);
            glPushMatrix();
            glTranslatef(flaggs->star[k].posit.x, flaggs->star[k].posit.y, flaggs->star[k].posit.z);

            for (i = 0; i < flaggs->star[k].ptr->moon_count; i++) {
                tb = (t_body *)flaggs->star[k].next[i];

                if (tb->orbsiz < 0.02)
                    glCallList(tb->orbtobj[2]);
                else if (tb->orbsiz < 0.35)
                    glCallList(tb->orbtobj[1]);
                else
                    glCallList(tb->orbtobj[0]);
            }

            glPopMatrix();
            glColor4f(1.0, 1.0, 1.0, 1.0);

            for (i = 0; i < flaggs->star[k].ptr->moon_count; i++) {
                tb = (t_body *)flaggs->star[k].next[i];

                if (tb->orbsiz >= 0.002)
                    spDrawString(tb->posit.x, tb->posit.y, tb->posit.z, tb->ptr->name);
            }
        }
    }

    if (Counter.flags & MRBIT_FLAG && flaggs->plan_current != -1) {
        tb = (t_body *)flaggs->star[flaggs->suun_current].next[flaggs->plan_current];

        glColor4f(0.0, 0.0, 1.0, 1.0);
        glPushMatrix();
        glTranslatef(tb->posit.x, tb->posit.y, tb->posit.z);

        for (i = 0; i < tb->ptr->moon_count; i++) {
            tc = (t_body *)tb->next[i];

            if (tc->orbsiz < 0.02)
                glCallList(tc->orbtobj[2]);
            else if (tc->orbsiz < 0.35)
                glCallList(tc->orbtobj[1]);
            else
                glCallList(tc->orbtobj[0]);
        }

        glPopMatrix();
        glColor4f(1.0, 1.0, 1.0, 1.0);

        for (i = 0; i < tb->ptr->moon_count; i++) {
            tc = (t_body *)tb->next[i];

            if (tc->orbsiz > 0.002)
                spDrawString(tc->posit.x, tc->posit.y, tc->posit.z, tc->ptr->name);
        }
    }

    if (!(Counter.alpha & HW_MULSA) && (Counter.alpha & HW_AALIN))
        glDisable(GL_BLEND);
}

/**********************************************************************
 *  calculate_luna()  -
 ***********************************************************************/
static void calculate_luna(D3 *luna, D3 *earth)

{
    flot64 T, l, m, o, L, M, lL, X;
    flot64 lambda, beta, r;
    sint32 i;

    T = (Counter.D + 2447891.5 - 2415020.0) / 36525.0;

    l = DTOR * (270.434164 + T * (481267.883142 - 0.001133 * T));
    i = (I2PI * l);
    l = l - 2.0 * M_PI * i;

    m = DTOR * (296.104608 + T * (477198.849108 + 0.009192 * T));
    i = (I2PI * m);
    m = m - 2.0 * M_PI * i;

    o = DTOR * (259.183275 + T * (-1934.142008 + 0.002078 * T));
    i = (I2PI * o);
    o = o - 2.0 * M_PI * i;

    L = DTOR * (279.696678 + T * (36000.768925 + 0.000303 * T));
    i = (I2PI * L);
    L = L - 2.0 * M_PI * i;

    M = DTOR * (358.475833 + T * (35999.049750 - 0.000150 * T));
    i = (I2PI * M);
    M = M - 2.0 * M_PI * i;

    lL = 2.0 * (l - L);

    lambda = 0.0;
    lambda += 22640.0 * fsin(m) + 769.0 * fsin(2.0 * m) + 36.0 * fsin(3.0 * m); /* equation of centre */
    lambda += -125.0 * fsin(l - L);                                             /* parallectic inequality */
    lambda += 2370.0 * fsin(lL);                                                /* variation */
    lambda += -668.0 * fsin(M);                                                 /* annual equation */
    lambda += -412.0 * fsin(2.0 * (l - o));                                     /* orbit/ecliptic */
    lambda += 212.0 * fsin(2.0 * (l - L - m));
    lambda += 4586.0 * fsin(lL - m); /* evection */
    lambda += 192.0 * fsin(lL + m);
    lambda += 165.0 * fsin(lL - M);
    lambda += 206.0 * fsin(lL - m - M);
    lambda += -110.0 * fsin(m + M);
    lambda += 148.0 * fsin(m - M);
    lambda = l + DTOR * lambda / 3600.0;

    X = 2.0 * L - l - o;

    beta = 0.0;
    beta += -18520.0 * fsin(lambda - o + DTOR * 0.114 * fsin(2.0 * (l - o)) + DTOR * 0.150 * fsin(M));
    beta += -526.0 * fsin(X);
    beta += 44.0 * fsin(X + m);
    beta += -31.0 * fsin(X - m);
    beta += -23.0 * fsin(X + M);
    beta += 11.0 * fsin(X - M);
    beta += -25.0 * fsin(l - o - m - m);
    beta += 21.0 * fsin(l - o - m);
    beta *= DTOR / 3600.0;

    r = 3423.0;
    r += 187.0 * fcos(m);
    r += 10.0 * fcos(m + m);
    r += 34.0 * fcos(lL - m);
    r += 28.0 * fcos(lL);
    r *= DTOR / 3600.0;
    r = 6378.14 / fsin(r);

    luna->x = earth->x + r * fcos(beta) * fsin(lambda);
    luna->y = earth->y + r * fsin(beta);
    luna->z = earth->z + r * fcos(beta) * fcos(lambda);
}

/**********************************************************************
 *  find_closest_star()  -
 **********************************************************************/
sint32 find_closest_star(sint32 *index, t_galaga *ga)

{
    flot32 x, y, z, dmin, r, xc, yc, zc;
    sint32 i;
    t_stars *st;

    *index = -1;
    xc = Counter.eye.x + 0.5 * GALAXY_EDGE - (Counter.x + 0.5) * EDGESQ;
    yc = Counter.eye.y;
    zc = Counter.eye.z + 0.5 * GALAXY_EDGE - (Counter.z + 0.5) * EDGESQ;

    for (dmin = 1.0e12, st = ga->stars, i = 0; i < ga->count; st++, i++) {
        x = st->x - xc;
        y = st->y - yc;
        z = st->z - zc;

        r = x * x + y * y + z * z;

        if (r < SOLSYS_EDGE) {
            *index = i;
            return (STELL_STAT);
        } else if (r < dmin) {
            dmin = r;
            *index = i;
        }
    }

    return (GALAC_STAT);
}

/**********************************************************************
 *  display_star_names()  -
 **********************************************************************/
static void display_star_names(void)

{
    sint32 i, count;
    t_stars *s;
    char *q;
    flot32 x, y, z, X, Y, Z, d, num;

    if (Counter.status == STELL_STAT) {
        s = &ggaa[Counter.z][Counter.x].stars[Counter.star_current];
        X = (Counter.x - SOL_X_GRID) * EDGESQ + Counter.eye.x * KMTOPR + s->x;
        Y = Counter.eye.y * KMTOPR + s->y;
        Z = (Counter.z - SOL_Z_GRID) * EDGESQ + Counter.eye.z * KMTOPR + s->z;
    } else {
        D3 pp;

        pp.x = -0.5 * GALAXY_EDGE + (SOL_X_GRID + 0.5) * EDGESQ;
        pp.y = 0.0;
        pp.z = -0.5 * GALAXY_EDGE + (SOL_Z_GRID + 0.5) * EDGESQ;
        if (!sphere_clipp(&pp, EDGESQ * EDGESQ))
            return;
        X = Counter.eye.x - pp.x;
        Y = Counter.eye.y - pp.y;
        Z = Counter.eye.z - pp.z;
    }

    glPushMatrix();
    glTranslatef(-X, -Y, -Z);

    glColor4f(0.625, 0.625, 0.625, 1.0);

    count = ggaa[SOL_Z_GRID][SOL_X_GRID].count;
    s = &ggaa[SOL_Z_GRID][SOL_X_GRID].stars[count - 1];

    for (num = 0.86, q = StarName[count - 1], i = count - 1; i > 0; s--, q -= 16, i--)
        if (i != Counter.star_current) {
            x = s->x - X;
            y = s->y - Y;
            z = s->z - Z;

            d = x * Counter.enorm.x + y * Counter.enorm.y + z * Counter.enorm.z;

            if (d * d > num * (x * x + y * y + z * z))
                spDrawString(s->x, s->y, s->z, q);
        }

    if (Counter.star_current != 0) {
        glColor4f(0.75, 0.75, 0.0, 1.0);
        spDrawString(s->x, s->y, s->z, StarName[0]);
    }

    glPopMatrix();
}

/**********************************************************************
 *  total_solar_eclipse()  -
 **********************************************************************/
static void total_solar_eclipse(t_boss *flaggs)

{
    T2 t[4];
    P3 p[4];
    V3 vr;
    D3 *sp;
    flot32 f, w, DD;
    uint32 col, r, g, b;

    sp = &flaggs->star[flaggs->suun_current].posit;

    if (!(sphere_clipp(sp, flaggs->star[flaggs->suun_current].cliprad)))
        return;

    DD = flaggs->star[flaggs->suun_current].ptr->rad * (15.0 / 7.0);

    vr.x = -Counter.light_vector.z;
    vr.y = 0.0;
    vr.z = Counter.light_vector.x;
    w = fsqrt(vr.x * vr.x + vr.y * vr.y + vr.z * vr.z);
    vr.x /= w;
    vr.y /= w;
    vr.z /= w;

    t[0].s = 0.0;
    t[0].t = 0.0;
    p[0].x = sp->x - DD * vr.x;
    p[0].y = sp->y - DD;
    p[0].z = sp->z - DD * vr.z;

    t[1].s = 1.0;
    t[1].t = 0.0;
    p[1].x = sp->x + DD * vr.x;
    p[1].y = sp->y - DD;
    p[1].z = sp->z + DD * vr.z;

    t[2].s = 1.0;
    t[2].t = 1.0;
    p[2].x = sp->x + DD * vr.x;
    p[2].y = sp->y + DD;
    p[2].z = sp->z + DD * vr.z;

    t[3].s = 0.0;
    t[3].t = 1.0;
    p[3].x = sp->x - DD * vr.x;
    p[3].y = sp->y + DD;
    p[3].z = sp->z - DD * vr.z;

    f = 10.0 * (Counter.feclipse - 0.9);
    if (f < 0.0)
        f = 0.0;
    if (f > 1.0)
        f = 1.0;
    f /= 256.0;

    col = flaggs->star[flaggs->suun_current].ptr->col;
    r = (col >> 0) & 0xff;
    g = (col >> 8) & 0xff;
    b = (col >> 16) & 0xff;
    glColor4f(r * f, g * f, b * f, 1.0);

    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
    glEnable(GL_BLEND);

    glBegin(GL_TRIANGLES);
    glTexCoord2fv(&t[0].s);
    glVertex3fv(&p[0].x);
    glTexCoord2fv(&t[1].s);
    glVertex3fv(&p[1].x);
    glTexCoord2fv(&t[2].s);
    glVertex3fv(&p[2].x);

    glTexCoord2fv(&t[2].s);
    glVertex3fv(&p[2].x);
    glTexCoord2fv(&t[3].s);
    glVertex3fv(&p[3].x);
    glTexCoord2fv(&t[0].s);
    glVertex3fv(&p[0].x);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_BLEND);
}

/**********************************************************************
 *  prepare_for_fractal_ringworld()  -
 **********************************************************************/
static void prepare_for_fractal_ringworld(t_boss *flaggs, sint32 i)

{
    flot64 x, y, z, c1, s1, w, u, daylight, epsi;
    sint32 j, b;
    t_system *planet;
    t_body *tb;
    Matrix ma;
    V4 *clop = Counter.clop;

    tb = (t_body *)flaggs->star[flaggs->suun_current].next[i];
    planet = tb->ptr;

    x = -flaggs->star[flaggs->suun_current].posit.x / planet->orb;
    y = -flaggs->star[flaggs->suun_current].posit.y / planet->orb;
    z = -flaggs->star[flaggs->suun_current].posit.z / planet->orb;

    /*** rot((flot32)theta,'y') ; ***/
    w = -2.0 * M_PI * Counter.D / planet->yer;
    c1 = cos(w);
    s1 = sin(w);
    ma[0][0] = c1;
    ma[0][1] = 0.0;
    ma[0][2] = -s1;
    ma[0][3] = 0.0;
    ma[1][0] = 0.0;
    ma[1][1] = 1.0;
    ma[1][2] = 0.0;
    ma[1][3] = 0.0;
    ma[2][0] = s1;
    ma[2][1] = 0.0;
    ma[2][2] = c1;
    ma[2][3] = 0.0;
    ma[3][0] = 0.0;
    ma[3][1] = 0.0;
    ma[3][2] = 0.0;
    ma[3][3] = 1.0;

    feye.x = ma[0][0] * x + ma[1][0] * y + ma[2][0] * z;
    feye.y = ma[0][1] * x + ma[1][1] * y + ma[2][1] * z;
    feye.z = ma[0][2] * x + ma[1][2] * y + ma[2][2] * z;
    feye.w = fsqrt(feye.x * feye.x + feye.y * feye.y + feye.z * feye.z);

    x = flaggs->star[flaggs->suun_current].posit.x - tb->posit.x;
    y = flaggs->star[flaggs->suun_current].posit.y - tb->posit.y;
    z = flaggs->star[flaggs->suun_current].posit.z - tb->posit.z;

    epsi = 0.1;
    vids = 0.0;

    if (feye.y > -RINGWEDGE && feye.y < RINGWEDGE && feye.w > 0.998 && feye.w < 1.0) {
        s1 = 0.5 * Counter.D / 9.037;
        s1 = 366.0 - 360.0 * (s1 - (long)s1);
        c1 = RTOD * (M_PI + atan2(feye.x, feye.z)) + s1;
        c1 /= 12.0;
        b = c1;
        c1 -= (long)c1;

        if (b & 1) {
            if (c1 < epsi)
                daylight = 0.5 * c1 / epsi + 0.5;
            else if (c1 > 1.0 - epsi)
                daylight = 0.5 * (1.0 - c1) / epsi + 0.5;
            else
                daylight = 1.0;
        } else {
            if (c1 < epsi)
                daylight = 0.5 - 0.5 * c1 / epsi;
            else if (c1 > 1.0 - epsi)
                daylight = 0.5 - 0.5 * (1.0 - c1) / epsi;
            else
                daylight = 0.0;
        }

        Counter.skyfrac = 500.0 * (feye.w - 0.998) * daylight;
        Counter.sky_clear_color[0] = 0.5 * Counter.skyfrac;
        Counter.sky_clear_color[1] = 0.5 * Counter.skyfrac;
        Counter.sky_clear_color[2] = 1.0 * Counter.skyfrac;
        Counter.sky_clear_color[3] = 1.0;
    } else {
        Counter.skyfrac = 0.0;
        Counter.sky_clear_color[0] = 0.0;
        Counter.sky_clear_color[1] = 0.0;
        Counter.sky_clear_color[2] = 0.0;
        Counter.sky_clear_color[3] = 0.0;
    }

    for (j = 0; j < NUM_CLIP_PLANES; j++) {
        plum[j].x = ma[0][0] * clop[j].x + ma[1][0] * clop[j].y + ma[2][0] * clop[j].z;
        plum[j].y = ma[0][1] * clop[j].x + ma[1][1] * clop[j].y + ma[2][1] * clop[j].z;
        plum[j].z = ma[0][2] * clop[j].x + ma[1][2] * clop[j].y + ma[2][2] * clop[j].z;
        plum[j].w = -(plum[j].x * feye.x + plum[j].y * feye.y + plum[j].z * feye.z);
    }
}

/**********************************************************************
 *  draw_all_them_stars()  -
 **********************************************************************/
void draw_all_them_stars(sint32 galflag)

{
    t_stars *st;
    sint32 i, j, k, stepp, mm[4], *ff, flag[18][18], cheater, *p;
    flot32 x, y, z, cut, one = 1.0, xc, yc, zc, mult;
    flot32 r;
    char *ch;
    P3 eye;
    t_galaga *ga;

    glEnable(GL_BLEND);
    if (Counter.flags & NDPNT_FLAG)
        glDisable(GL_DITHER);

    if (galflag) {
        glPushMatrix();
        glTranslatef(-Counter.eye.x, -Counter.eye.y, -Counter.eye.z);
        glCallList(Counter.starobj);
        glPopMatrix();

        eye.x = Counter.eye.x;
        eye.y = Counter.eye.y;
        eye.z = Counter.eye.z;
    } else {
        st = &ggaa[Counter.z][Counter.x].stars[Counter.star_current];
        eye.x = st->x + -0.5 * GALAXY_EDGE + (Counter.x + 0.5) * EDGESQ;
        eye.y = st->y;
        eye.z = st->z + -0.5 * GALAXY_EDGE + (Counter.z + 0.5) * EDGESQ;
    }

    p = (sint32 *)&r;
    cheater = ((Counter.z == SOL_Z_GRID && Counter.x == SOL_X_GRID) ? 1 : 0);
    stepp = (cheater ? 0 : 8);

    mm[0] = Counter.z - stepp;
    if (mm[0] < 0)
        mm[0] = 0;
    mm[1] = Counter.z + stepp;
    if (mm[1] >= STARSQ)
        mm[1] = STARSQ - 1;
    mm[2] = Counter.x - stepp;
    if (mm[2] < 0)
        mm[2] = 0;
    mm[3] = Counter.x + stepp;
    if (mm[3] >= STARSQ)
        mm[3] = STARSQ - 1;

    if (!cheater)
        star_square_clip_check(mm, flag, &eye);

    yc = eye.y;
    zc = 0.5 * GALAXY_EDGE - (mm[0] + 0.5) * EDGESQ + eye.z;

    for (k = mm[0]; k <= mm[1]; zc -= EDGESQ, k++) {
        xc = 0.5 * GALAXY_EDGE - (mm[2] + 0.5) * EDGESQ + eye.x;
        ff = flag[k - mm[0]];
        ga = &ggaa[k][mm[2]];

        for (j = mm[2]; j <= mm[3]; xc -= EDGESQ, ff++, ga++, j++)

            if (Counter.x == j && Counter.z == k) {
                glPushMatrix();
                glTranslatef(-xc, -yc, -zc);
                st = &ga->stars[ga->count - 1];
                cut = Counter.cutoff;

                glBegin(GL_POINTS);
                for (i = ga->count - Counter.star_current - 1; i > 0; i--, st--) {
                    x = st->x - xc;
                    y = st->y - yc;
                    z = st->z - zc;
                    r = x * x + y * y + z * z;

                    ch = (char *)lut + ((*p >> 19) << 2);
                    st->a = st->fny_mag * *(flot32 *)ch;
                    if (st->a > one)
                        st->a = one;

                    if (st->a > cut) {
                        glColor4fv(&st->r);
                        glVertex3fv(&st->x);
                    }
                }
                for (st--, i = Counter.star_current; i > 0; i--, st--) {
                    x = st->x - xc;
                    y = st->y - yc;
                    z = st->z - zc;
                    r = x * x + y * y + z * z;

                    ch = (char *)lut + ((*p >> 19) << 2);
                    st->a = st->fny_mag * *(flot32 *)ch;
                    if (st->a > one)
                        st->a = one;

                    if (st->a > cut) {
                        glColor4fv(&st->r);
                        glVertex3fv(&st->x);
                    }
                }
                glEnd();

                glPopMatrix();
            }

            else if (!(*(ff + 0) & *(ff + 1) & *(ff + 18) & *(ff + 19))) {
                glPushMatrix();
                glTranslatef(-xc, -yc, -zc);
                st = &ga->stars[ga->count - 1];
                cut = Counter.cutoff;

                r = xc * xc + yc * yc + zc * zc;
                ch = (char *)lut + ((*p >> 19) << 2);
                mult = *(flot32 *)ch;

                glBegin(GL_POINTS);
                for (i = ga->count; i > 0; i--, st--) {
                    st->a = st->fny_mag * mult;
                    if (st->a > one)
                        st->a = one;

                    if (st->a > cut) {
                        glColor4fv(&st->r);
                        glVertex3fv(&st->x);
                    }
                }
                glEnd();

                glPopMatrix();
            }
    }

    if (Counter.flags & NDPNT_FLAG)
        glEnable(GL_DITHER);
    glDisable(GL_BLEND);
}

/**********************************************************************
 *  read_data_file()  -
 **********************************************************************/
static void read_data_file(t_body *body)

{
    FILE *fd;
    sint32 bytes;

    if (strcmp(body->ptr->elev, mem_ele_nme)) {
        if (body->ptr->elev[0] != '\0' && (fd = fopen(datatrail(body->ptr->elev), "r"))) {
            bytes = (((body->ptr->lformat & 0xf) + 1) >> 1) * body->ptr->lxsize * body->ptr->lysize;

            if (bytes > MAX_ELEV_BYTES) {
                printf("INTERNAL ERROR: Data file too large: %s\n", datatrail(body->ptr->elev));
                exit(0);
            }

            stall_message("Please Wait: Loading Data File", 0.4, 0.5);

            body->land = mem_ele_ptr;
            fread(body->land, bytes, body->ptr->lxsize * body->ptr->lysize, fd);
            fclose(fd);
        } else {
            printf("USER ERROR: Data file not found: %s\n", datatrail(body->ptr->elev));
            exit(0);
        }

        strcpy(mem_ele_nme, body->ptr->elev);
    }

    if (strcmp(body->ptr->colr, mem_col_nme)) {
        if (body->ptr->colr[0] != '\0' && (fd = fopen(datatrail(body->ptr->colr), "r"))) {
            bytes = (((body->ptr->cformat & 0xf) + 1) >> 1) * body->ptr->cxsize * body->ptr->cysize;

            if (bytes > MAX_COLR_BYTES) {
                printf("INTERNAL ERROR: Data file too large: %s\n", datatrail(body->ptr->colr));
                exit(0);
            }

            stall_message("Please Wait: Loading Data File", 0.4, 0.5);

            body->colr = mem_col_ptr;
            fread(body->colr, bytes, body->ptr->cxsize * body->ptr->cysize, fd);
            fclose(fd);
        }

        strcpy(mem_col_nme, body->ptr->colr);
    }
}

/**********************************************************************
 *  stall_message()  -
 **********************************************************************/
static void stall_message(char *st, flot32 x, flot32 y)

{
    flot32 dx = 1.0 / Counter.winsizex;
    flot32 dy = 1.0 / Counter.winsizey;
    sint32 ln = strlen(st);

    Counter.flags |= FREEZ_FLAG;

    spSwapBuffers();
    glPushMatrix();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(1.0, 0.0, 0.0, 1.0);
    glRectf(x - 4.0 * dx, y - 4.0 * dy, x + (9.0 * ln + 4.0) * dx, y + 14.0 * dy);

    glColor4f(1.0, 1.0, 1.0, 1.0);
    spDrawString(x, y, 0.0, st);

    glPopMatrix();
    spSwapBuffers();
}

/**********************************************************************
 *  special_perspective()  -
 **********************************************************************/
void special_perspective(flot32 min, flot32 max)

{
    float fov, top, bot, rig, lef, dx, dy, dz;

    if (Counter.flags & ACCUM_FLAG) {
        fov = Counter.viewangle * M_PI / 3600.0;
        top = min * fsin(fov) / fcos(fov);
        bot = -top;
        rig = top * Counter.aspcratio;
        lef = -rig;

        dx = -Counter.acdx * (rig - lef) / 1280.0;
        dy = -Counter.acdy * (top - bot) / 1024.0;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(lef + dx, rig + dx, bot + dy, top + dy, min, max);
        glMatrixMode(GL_MODELVIEW);
    } else {
        glMatrixMode(GL_PROJECTION);
        spPerspMatrix(min, max);
        glMatrixMode(GL_MODELVIEW);
    }
}

/**********************************************************************
 *  accumulation()  -
 **********************************************************************/
void accumulation(t_boss *flaggs)

{}
