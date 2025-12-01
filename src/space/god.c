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

extern char StarName[NUMBER_OF_STARS][16];

extern char mem_col_nme[32];
extern char mem_ele_nme[32];
extern char *mem_col_ptr;
extern char *mem_ele_ptr;
extern t_stopwatch Counter;

static sint32 already_count = 0;

static struct {
    char name[32];
    sint32 texdf;
} already[64];

#include "sol.h"

static long check_data_file(char *);

/**********************************************************************
 *  create_solar_system()  -
 ***********************************************************************/
t_system *create_solar_system(sint32 index, t_boss *flaggs, t_galaga *ga)

{
    t_system *planet, *moon;
    sint32 i, j, k;
    t_body *tb, *tc;

    flaggs->plan_current = -1;
    flaggs->moon_current = -1;
    flaggs->suun_current = -1;

    flaggs->plan_old = -1;
    flaggs->moon_old = -1;
    flaggs->suun_old = -1;

    generate_solar_system(flaggs, index, &ga->stars[index]);

    for (k = 0; k < flaggs->suncount; k++) {
        for (planet = flaggs->star[k].ptr->moons, i = 0; i < flaggs->star[k].ptr->moon_count; planet++, i++) {
            flaggs->star[k].next[i] = tb = (void *)malloc(sizeof(t_body));

            tb->ptr = planet;
            create_one(tb, 0.0);

            for (moon = planet->moons, j = 0; j < planet->moon_count; moon++, j++) {
                tb->next[j] = tc = (void *)malloc(sizeof(t_body));

                tc->ptr = moon;
                create_one(tc, tb->ptr->apo);
            }
        }
    }

    Counter.flags |= REGEN_FLAG;
    return NULL;
} /**********************************************************************
   *  destroy_solar_system()  -
   ***********************************************************************/
void destroy_solar_system(t_boss *flaggs)

{
    sint32 i, j, k;
    t_body *tb, *tc;

    printf("Destroy Star\n");

    flaggs->plan_current = -1;
    flaggs->moon_current = -1;
    flaggs->suun_current = -1;

    flaggs->plan_old = -1;
    flaggs->moon_old = -1;
    flaggs->suun_old = -1;

    for (k = 0; k < flaggs->suncount; k++) {
        for (i = 0; i < flaggs->star[k].ptr->moon_count; i++) {
            tb = (t_body *)flaggs->star[k].next[i];

            for (j = 0; j < tb->ptr->moon_count; j++) {
                tc = (t_body *)tb->next[j];
                destroy_one(tc);
                free(tc);
            }

            destroy_one(tb);
            free(tb);
        }

        destroy_one(&flaggs->star[k]);
    }
}

/**********************************************************************
 *  scan_star_system()  -
 **********************************************************************/
void scan_star_system(t_boss *flaggs)

{
    flot32 d, D, dmin, angle;
    sint32 i, k, r, g, b;
    t_body *tb, *tc, *sptr;
    D3 p;
    V3 v;

    Counter.flags &= ~FRACT_FLAG;
    Counter.flags &= ~RINGW_FLAG;
    Counter.flags &= ~ECLIP_FLAG;

    flaggs->plan_old = flaggs->plan_current;
    flaggs->moon_old = flaggs->moon_current;
    flaggs->suun_old = flaggs->suun_current;

    flaggs->plan_current = -1;
    flaggs->moon_current = -1;
    flaggs->suun_current = -1;

    for (dmin = 1.0e20, k = 0; k < flaggs->suncount; k++) {
        tb = &flaggs->star[k];

        p.x = 0.0;
        p.y = 0.0;
        p.z = 0.0;
        calc_posit(&flaggs->star[k].posit, tb->ptr, &p, 0.0);
        flaggs->star[k].posit.x -= Counter.eye.x;
        flaggs->star[k].posit.y -= Counter.eye.y;
        flaggs->star[k].posit.z -= Counter.eye.z;

        d = sqrt(flaggs->star[k].posit.x * flaggs->star[k].posit.x + flaggs->star[k].posit.y * flaggs->star[k].posit.y +
                 flaggs->star[k].posit.z * flaggs->star[k].posit.z);

        flaggs->star[k].distan = d;
        flaggs->star[k].angsiz = flaggs->star[k].ptr->rad / d;

        if (d <= flaggs->star[k].ptr->rad && Counter.click != 0) {
            spRingBell();
            printf("You Crashed Into The Sun And Vaporized!!\n");
            if ((Counter.alpha & HW_SOUND) && (Counter.flags & SOUND_FLAG))
                sound_control(1, SND_BUTT);
            exit(0);
        }

        if (d < dmin) {
            dmin = d;
            flaggs->suun_current = k;
        }
    }

    sptr = &flaggs->star[flaggs->suun_current];

    for (dmin = 1.0e20, k = 0; k < flaggs->suncount; k++) {
        for (i = 0; i < flaggs->star[k].ptr->moon_count; i++) {
            tb = (t_body *)flaggs->star[k].next[i];

            calc_posit(&p, tb->ptr, &flaggs->star[k].posit, 0.0);

            tb->posit = p;

            d = fsqrt(p.x * p.x + p.y * p.y + p.z * p.z);

            tb->distan = d;
            tb->orbsiz = tb->ptr->orb / d;

            if (((tb->ptr->tess >> 4) & 0x0f) == FRAC_RINGWO)
                tb->angsiz = tb->ptr->orb / d;
            else
                tb->angsiz = tb->ptr->rad / d;

            if (d <= tb->ptr->rad && Counter.click != 0) {
                spRingBell();
                Counter.eye.x -= (12.0 + tb->ptr->rad - d) * p.x / d;
                Counter.eye.y -= (12.0 + tb->ptr->rad - d) * p.y / d;
                Counter.eye.z -= (12.0 + tb->ptr->rad - d) * p.z / d;
            }

            if (tb->angsiz < ANGULAR_SIZE) {
                v.x = p.x - sptr->posit.x;
                v.y = p.y - sptr->posit.y;
                v.z = p.z - sptr->posit.z;
                D = fsqrt(v.x * v.x + v.y * v.y + v.z * v.z);
                angle = (v.x * p.x + v.y * p.y + v.z * p.z) / (d * D);
                angle = 0.5 * (1.0 + angle);

                r = (tb->ptr->col >> 0) & 0xff;
                g = (tb->ptr->col >> 8) & 0xff;
                b = (tb->ptr->col >> 16) & 0xff;
                r = (float)r * angle;
                g = (float)g * angle;
                b = (float)b * angle;
                tb->color = 0xff000000 | (b << 16) | (g << 8) | r;
            }

            if (k == flaggs->suun_current && d < dmin) {
                dmin = d;
                flaggs->plan_current = i;
            }
        }
    }

    if (flaggs->plan_current < 0)
        return;

    tb = (t_body *)sptr->next[flaggs->plan_current];

    Counter.light_vector.x = sptr->posit.x - tb->posit.x;
    Counter.light_vector.y = sptr->posit.y - tb->posit.y;
    Counter.light_vector.z = sptr->posit.z - tb->posit.z;
    Counter.light_angle = 180.0 + RTOD * fatan2(Counter.light_vector.x, Counter.light_vector.z);

    if (flaggs->plan_old != flaggs->plan_current || flaggs->suun_old != flaggs->suun_current)
        Counter.flags |= REGEN_FLAG;

    switch ((tb->ptr->tess >> 4) & 0x0f) {
    case FRAC_PLANET:
        Counter.flags |= FRACT_FLAG;
        Counter.flags &= ~RINGW_FLAG;
        break;
    case ELEV_PLANET:
        Counter.flags &= ~FRACT_FLAG;
        Counter.flags &= ~RINGW_FLAG;
        break;
    case FRAC_RINGWO:
        Counter.flags |= FRACT_FLAG;
        Counter.flags |= RINGW_FLAG;
        break;
    case ELEV_RINGWO:
        Counter.flags &= ~FRACT_FLAG;
        Counter.flags |= RINGW_FLAG;
        break;
    default:
        break;
    }

    if (do_da_eclipse(flaggs, &tb->posit, tb->ptr->rad))
        Counter.infoco = ECLPS_COLR;

    for (dmin = 1e20, i = 0; i < tb->ptr->moon_count; i++) {
        tc = (t_body *)tb->next[i];

        calc_posit(&p, tc->ptr, &tb->posit, tb->ptr->apo);

        tc->posit = p;

        d = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);

        tc->distan = d;
        tc->angsiz = tc->ptr->rad / d;
        tc->orbsiz = tc->ptr->orb / d;

        if (d <= tc->ptr->rad) {
            spRingBell();
            Counter.eye.x -= (12.0 + tc->ptr->rad - d) * p.x / d;
            Counter.eye.y -= (12.0 + tc->ptr->rad - d) * p.y / d;
            Counter.eye.z -= (12.0 + tc->ptr->rad - d) * p.z / d;
        }

        if (tc->angsiz < ANGULAR_SIZE) {
            v.x = p.x - sptr->posit.x;
            v.y = p.y - sptr->posit.y;
            v.z = p.z - sptr->posit.z;
            D = fsqrt(v.x * v.x + v.y * v.y + v.z * v.z);
            angle = (v.x * p.x + v.y * p.y + v.z * p.z) / (d * D);
            angle = 0.5 * (1.0 + angle);

            r = (tc->ptr->col >> 0) & 0xff;
            g = (tc->ptr->col >> 8) & 0xff;
            b = (tc->ptr->col >> 16) & 0xff;
            D = angle * tc->angsiz / ANGULAR_SIZE;
            r = (float)r * D;
            g = (float)g * D;
            b = (float)b * D;
            tc->color = 0xff000000 | (b << 16) | (g << 8) | r;
        }

        if (d < dmin) {
            dmin = d;
            flaggs->moon_current = i;
        }
    }

    if (flaggs->moon_current < 0)
        return;

    tc = (t_body *)tb->next[flaggs->moon_current];

    if (do_da_eclipse(flaggs, &tc->posit, tc->ptr->rad))
        Counter.infoco = ECLPS_COLR;

    if (Counter.star_current == 0 && flaggs->plan_current == 2) {
        p.x = 0.5 * (tb->posit.x + tc->posit.x);
        p.y = 0.5 * (tb->posit.y + tc->posit.y);
        p.z = 0.5 * (tb->posit.z + tc->posit.z);
        d = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);

        flaggs->stat.posit.x = p.x;
        flaggs->stat.posit.y = p.y;
        flaggs->stat.posit.z = p.z;

        flaggs->stat.distan = d;
        flaggs->stat.angsiz = flaggs->stat.cliprad / d;
    }
}

/**********************************************************************
 *  create_one()  -
 ***********************************************************************/
static void create_one(t_body *body, flot32 tilt)

{
    FILE *fd;
    flot32 texps[8];
    uint32 txt[256 * 256];
    sint32 a, b, i, j, obj, bytes;
    char filename[256];

    body->posit.x = 0.0;
    body->posit.y = 0.0;
    body->posit.z = 0.0;
    body->cliprad = 0.0;
    body->color = 0;
    body->texdf = 0;
    body->texrn = 0;
    body->orbtobj[0] = 0;
    body->orbtobj[1] = 0;
    body->orbtobj[2] = 0;
    body->bodyobj[0] = 0;
    body->bodyobj[1] = 0;
    body->bodyobj[2] = 0;
    body->ringobj[0] = 0;
    body->ringobj[1] = 0;
    body->ringobj[2] = 0;

    a = (body->ptr->tess >> 4) & 0x0f;
    b = (body->ptr->tess) & 0x0f;

    if (a == TEXX_SPHERE) {
        if (!check_data_file(body->ptr->colr)) {
            printf("**ERROR**: Texture file not found: %s :Lit sphere substituted\n", datatrail(body->ptr->colr));
            a = LIGT_SPHERE;
            body->ptr->tess = b | (a << 4);
        }
    } else if (a == ELEV_PLANET) {
        if (!check_data_file(body->ptr->elev)) {
            printf("**ERROR**: Data file not found: %s :Lit sphere substituted\n", datatrail(body->ptr->elev));
            a = LIGT_SPHERE;
            body->ptr->tess = b | (a << 4);
        }

        if (!check_data_file(body->ptr->colr)) {
            printf("**ERROR**: Data file not found: %s :Lit sphere substituted\n", datatrail(body->ptr->colr));
            a = LIGT_SPHERE;
            body->ptr->tess = b | (a << 4);
        }
    }

    if (body->ptr->r1 != 0.0 && body->ptr->r2 != 0.0) {
        if (body->ptr->ring[0] != '\0' && (fd = fopen(datatrail(body->ptr->ring), "r"))) {
            fread(txt, 2, 256, fd);
            fclose(fd);
            body->texrn = spTexDef(2, 256, 1, txt, 1);
        } else {
            printf("USER ERROR: Texture file not found: %s\n", datatrail(body->ptr->ring));
            exit(0);
        }
    }

    switch (a) {

    /* v3f sphere */
    case FLAT_SPHERE:
        body->bodyobj[0] = generate_sphere(b, FLAT_SPHERE, 0);
        body->bodyobj[1] = generate_sphere(b - 1, FLAT_SPHERE, 0);
        body->bodyobj[2] = generate_sphere(b - 2, FLAT_SPHERE, 0);

        if (body->ptr->r1 == 0.0 && body->ptr->r2 == 0.0)
            body->cliprad = body->ptr->rad;
        else
            body->cliprad = body->ptr->r2;
        break;

    /* n3f v3f sphere */
    case LIGT_SPHERE:
        body->bodyobj[0] = generate_sphere(b, LIGT_SPHERE, 0);
        body->bodyobj[1] = generate_sphere(b - 1, LIGT_SPHERE, 0);
        body->bodyobj[2] = generate_sphere(b - 2, LIGT_SPHERE, 0);

        if (body->ptr->r1 == 0.0 && body->ptr->r2 == 0.0)
            body->cliprad = body->ptr->rad;
        else
            body->cliprad = body->ptr->r2;
        break;

    /* t2f n3f v3f sphere */
    case TEXX_SPHERE:
        body->bodyobj[0] = generate_sphere(b, TEXX_SPHERE, body->ptr->texsz);
        body->bodyobj[1] = generate_sphere(b - 1, TEXX_SPHERE, body->ptr->texsz);
        body->bodyobj[2] = generate_sphere(b - 2, TEXX_SPHERE, body->ptr->texsz);

        if (body->ptr->r1 == 0.0 && body->ptr->r2 == 0.0)
            body->cliprad = body->ptr->rad;
        else
            body->cliprad = body->ptr->r2;

        for (i = 0; i < already_count; i++)
            if (strcmp(already[i].name, body->ptr->colr) == 0)
                body->texdf = already[i].texdf;

        if (body->texdf == 0) {
            if (body->ptr->colr[0] != '\0' && (fd = fopen(datatrail(body->ptr->colr), "r"))) {
                fread(txt, 4, body->ptr->texsz * body->ptr->texsz, fd);
                fclose(fd);

                body->texdf = spTexDef(4, body->ptr->texsz, body->ptr->texsz, txt, 0);
                strcpy(already[already_count].name, body->ptr->colr);
                already[already_count++].texdf = body->texdf;
            } else {
                printf("USER ERROR: Texture file not found: %s\n", datatrail(body->ptr->colr));
                exit(0);
            }
        }
        break;

    /* fractal planet */
    case FRAC_PLANET:
        if (body->ptr->r1 == 0.0 && body->ptr->r2 == 0.0)
            body->cliprad = 1.25 * body->ptr->rad;
        else
            body->cliprad = body->ptr->r2;
        break;

    /* elevation field v3f planet */
    case ELEV_PLANET:
        if (body->ptr->r1 == 0.0 && body->ptr->r2 == 0.0)
            body->cliprad = 1.25 * body->ptr->rad;
        else
            body->cliprad = body->ptr->r2;

        if (strcmp(body->ptr->elev, mem_ele_nme) == 0)
            body->land = mem_ele_ptr;
        if (strcmp(body->ptr->colr, mem_col_nme) == 0)
            body->colr = mem_col_ptr;
        break;

    /* fractal ringworld */
    case FRAC_RINGWO:
        body->cliprad = 2.01 * body->ptr->orb;
        break;

    /* elevation field v3f ringworld */
    case ELEV_RINGWO:
        printf("Not Supported Option\n");
        exit(0);
        break;

    default:
        printf("Not Supported Option\n");
        exit(0);
        break;
    }

    object_planet_orbit(body, tilt);

    if (body->ptr->r1 != 0.0 && body->ptr->r2 != 0.0) {
        body->ringobj[0] = object_planet_rings(body->ptr, 256);
        body->ringobj[1] = object_planet_rings(body->ptr, 64);
        body->ringobj[2] = object_planet_rings(body->ptr, 16);
    }
}

/**********************************************************************
 *  destroy_one()  -
 ***********************************************************************/
static void destroy_one(t_body *body)

{
    sint32 i;

    for (i = 0; i < 3; i++) {
        if (glIsList(body->orbtobj[i]))
            glDeleteLists(body->orbtobj[i], 1);

        if (glIsList(body->ringobj[i]))
            glDeleteLists(body->ringobj[i], 1);
    }
}

/**********************************************************************
 *  p_t_syst()  -
 ***********************************************************************/
void p_t_syst(t_system *system)

{
    printf("name   : %s\n", system->name);
    printf("orb    : %f\n", system->orb);
    printf("ecc    : %f\n", system->ecc);
    printf("inc    : %f\n", system->inc);
    printf("rad    : %f\n", system->rad);
    printf("mas    : %f\n", system->mas);
    printf("apo    : %f\n", system->apo);
    printf("yer    : %f\n", system->yer);
    printf("day    : %f\n", system->day);
    printf("ee     : %f\n", system->ee);
    printf("ww     : %f\n", system->ww);
    printf("omega  : %f\n", system->omega);
    printf("r1     : %f\n", system->r1);
    printf("r2     : %f\n", system->r2);
    printf("ring   : %s\n", system->ring);

    printf("moons  : %d\n", system->moon_count);
    printf("address: %p\n", (void *)system->moons);
    printf("\n");
    fflush(stdout);
} /**********************************************************************
   *  p_t_body()  -
   ***********************************************************************/
void p_t_body(t_body *body)

{
    sint32 i;

    printf("Address: %p\n", (void *)body->ptr);
    printf("Name   : %s\n", body->ptr->name);
    printf("Where  : %f %f %f\n", body->posit.x, body->posit.y, body->posit.z);
    printf("Angsize: %f\n", body->angsiz);
    printf("Cliprad: %f\n", body->cliprad);
    printf("Color  : 0x%08x\n", body->color);
    printf("Texture: %d\n", body->texdf);
    printf("Texture: %d\n", body->texrn);
    printf("B Obj  : %d %d %d\n", body->bodyobj[0], body->bodyobj[1], body->bodyobj[2]);
    printf("O Obj  : %d %d %d\n", body->orbtobj[0], body->orbtobj[1], body->orbtobj[2]);
    printf("R Obj  : %d %d %d\n", body->ringobj[0], body->ringobj[1], body->ringobj[2]);
    printf("land   : %p\n", (void *)body->land);
    printf("colr   : %p\n", (void *)body->colr);
    printf("\n");
    fflush(stdout);
} /**********************************************************************
   *  p_t_boss()  -
   ***********************************************************************/
void p_t_boss(t_boss *boss)

{
    printf("Address: %p\n", (void *)boss);
    printf("plan_current : %d\n", boss->plan_current);
    printf("plan_old     : %d\n", boss->plan_old);
    printf("moon_current : %d\n", boss->moon_current);
    printf("moon_old     : %d\n", boss->moon_old);
    fflush(stdout);

    p_t_body(&boss->stat);

    p_t_body(&boss->star[0]);
    p_t_syst(boss->star[0].ptr);
}

/**********************************************************************
 *  generate_solar_system()  -
 ***********************************************************************/
static void generate_solar_system(t_boss *flaggs, sint32 index, t_stars *s)

{
    t_system *xxx, *yyy, *zzz, *y, *z;
    flot32 temp;
    sint32 i, j, k, *pp = (sint32 *)&temp, suction;
    flot32 apo[2], rad[2], q, f, temperature;
    flot64 mas[2];
    char ch[32];
    C3 b;

    flaggs->suncount = 1;
    flaggs->star[0].ptr = NULL;
    flaggs->star[1].ptr = NULL;

    if (Counter.z == SOL_Z_GRID && Counter.x == SOL_X_GRID && index == 0) {
        flaggs->star[0].ptr = &solsys;
        create_one(&flaggs->star[0], 0.0);
        return;
    } else if ((index % 100) == 14) {
        flaggs->star[0].ptr = generate_ringworld_system(index, s);
        create_one(&flaggs->star[0], 0.0);
        return;
    }

    temp = s->x + s->y + s->z;
    srand(*pp);
    rand();
    suction = rand() & 1;

    flaggs->suncount = ((s->abs_mag2 == 0.0 && s->scass2 == 0.0) ? 1 : 2);

    for (k = 0; k < flaggs->suncount; k++) {
        if ((xxx = (t_system *)malloc(sizeof(t_system))) == 0) {
            printf("ERROR: solar system generation malloc failed\n");
            exit(0);
        }
        flaggs->star[k].ptr = xxx;
    }

    star_params(s, apo, rad, mas);

    for (k = 0; k < flaggs->suncount; k++) {
        xxx = flaggs->star[k].ptr;

        if (Counter.z == SOL_Z_GRID && Counter.x == SOL_X_GRID) {
            if (!strcmp(StarName[index], "")) {
                sprintf(ch, "%d-%d,%d", index, Counter.x, Counter.z);
                strcpy(xxx->name, ch);
            } else
                strcpy(xxx->name, StarName[index]);
        } else {
            sprintf(ch, "%d-%d,%d", index, Counter.x, Counter.z);
            strcpy(xxx->name, ch);
        }

        xxx->apo = apo[k];
        xxx->rad = rad[k];
        xxx->mas = mas[k];

        if (flaggs->suncount == 2) {
            xxx->ecc = 0.75 * float_rand();
            xxx->inc = 45.0 * (float_rand() - 0.5);
        } else {
            xxx->ecc = 0.0;
            xxx->inc = 0.0;
        }

        xxx->orb = 0.0;
        xxx->yer = 9.0;
        xxx->day = 1.0;
        xxx->ee = 0.0;
        xxx->ww = 0.0;
        xxx->omega = 0.0;
        xxx->r1 = 0.0;
        xxx->r2 = 0.0;
        xxx->ring[0] = '\0';

        if (k == 1) {
            if (s->scass2 < 0.10) {
                b.r = 1.00;
                b.g = 0.00;
                b.b = 0.00;
            } else if (s->scass2 < 0.28) {
                b.r = 1.00;
                b.g = 0.50;
                b.b = 1.00;
            } else if (s->scass2 < 0.37) {
                b.r = 1.00;
                b.g = 1.00;
                b.b = 0.00;
            } else if (s->scass2 < 0.45) {
                b.r = 0.50;
                b.g = 1.00;
                b.b = 1.00;
            } else if (s->scass2 < 0.58) {
                b.r = 0.00;
                b.g = 1.00;
                b.b = 0.00;
            } else if (s->scass2 < 0.89) {
                b.r = 0.00;
                b.g = 1.00;
                b.b = 0.50;
            } else {
                b.r = 0.00;
                b.g = 0.00;
                b.b = 1.00;
            }
            xxx->col =
                0xff000000 | ((sint32)(b.b * 255.0) << 16) | ((sint32)(b.g * 255.0) << 8) | (sint32)(b.r * 255.0);
        } else {
            xxx->col =
                0xff000000 | ((sint32)(s->b * 255.0) << 16) | ((sint32)(s->g * 255.0) << 8) | (sint32)(s->r * 255.0);
        }

        xxx->tess = 3;
        xxx->texsz = 0;
        xxx->scale = 0.0;
        xxx->lxsize = 0;
        xxx->lysize = 0;
        xxx->lformat = 0;
        xxx->elev[0] = '\0';
        xxx->cxsize = 0;
        xxx->cysize = 0;
        xxx->cformat = 0;
        xxx->colr[0] = '\0';

        if (suction && flaggs->suncount == 2)
            xxx->moon_count = 0;
        else
            xxx->moon_count = (3 * (rand() & 0xf)) >> 2;
        xxx->moons = NULL;

        create_one(&flaggs->star[k], 0.0);

        if (xxx->moon_count > 0) {
            if ((yyy = (t_system *)malloc(xxx->moon_count * sizeof(t_system))) == 0) {
                printf("ERROR: planet system generation malloc failed\n");
                exit(0);
            }

            xxx->moons = yyy;

            for (y = yyy, i = 0; i < xxx->moon_count; y++, i++) {
                y->name[0] = i + 'A';
                y->name[1] = '\0';

                if (i == 0)
                    y->orb = xxx->rad + 100000000.0;
                else
                    y->orb = 2.0 * (y - 1)->orb - 0.4 * AUTOKM;

                y->ecc = 0.1 * float_rand();
                y->inc = xxx->inc + 90.0 * (float_rand() - 0.5) * (float_rand() - 0.5);
                y->rad = 1000.0 + rand();
                y->mas = 1.0e25;
                y->apo = float_rand();
                y->apo = 90.0 * (y->apo * y->apo * y->apo);
                y->yer = newton(y->orb, xxx->mas + y->mas);
                y->day = 0.5 + (rand() & 0x0f);
                y->ee = rand() & 0x01ff;
                y->ww = rand() & 0x01ff;
                y->omega = rand() & 0x01ff;
                y->r1 = 0.0;
                y->r2 = 0.0;

                if ((rand() & 0x7) == 0) {
                    y->r1 = 1.2 * y->rad;
                    y->r2 = 2.0 * y->rad;
                    strcpy(y->ring, "saturn.ring");
                }

                /* earth type temperature ? */
                f = (xxx->rad * xxx->rad * xxx->apo * 150.0) / (7000.0 * 7000.0 * 55.0 * y->orb);

                if (f > 0.8 && f < 1.25) {
                    y->tess = 0x30;
                    y->apo *= 0.25;
                    spRingBell();
                } else
                    y->tess = 0x14;

                y->col = generate_full_color();
                y->texsz = 0;
                y->scale = 0.0;
                y->lxsize = 0;
                y->lysize = 0;
                y->lformat = 0;
                y->elev[0] = '\0';
                y->cxsize = 0;
                y->cysize = 0;
                y->cformat = 0;
                y->colr[0] = '\0';
                y->moon_count = 12.0 * float_rand() * float_rand();
                y->moons = NULL;

                if (y->tess == 0x30)
                    y->moon_count++;

                if (y->moon_count > 0) {
                    if ((zzz = (t_system *)malloc(y->moon_count * sizeof(t_system))) == 0) {
                        printf("ERROR: planet system generation malloc failed\n");
                        exit(0);
                    }

                    y->moons = zzz;

                    for (z = zzz, j = 0; j < y->moon_count; z++, j++) {
                        z->name[0] = j + 'a';
                        z->name[1] = '\0';

                        z->orb = y->rad + (j + 1) * 100000.0;

                        z->ecc = 0.2 * float_rand();
                        z->inc = 180.0 * (float_rand() - 0.5) * (float_rand() - 0.5);
                        z->rad = 100.0 + (rand() >> 3);
                        z->mas = 1.0e22;
                        z->apo = 0.0;
                        z->yer = newton(z->orb, y->mas + z->mas);
                        z->day = z->yer;
                        z->ee = rand() & 0x01ff;
                        z->ww = rand() & 0x01ff;
                        z->omega = rand() & 0x01ff;
                        z->r1 = 0.0;
                        z->r2 = 0.0;

                        z->col = generate_full_color();
                        z->tess = 0x13;
                        z->texsz = 0;
                        z->scale = 0.0;
                        z->lxsize = 0;
                        z->lysize = 0;
                        z->lformat = 0;
                        z->elev[0] = '\0';
                        z->cxsize = 0;
                        z->cysize = 0;
                        z->cformat = 0;
                        z->colr[0] = '\0';

                        z->moon_count = 0;
                        z->moons = NULL;
                    }
                }
            }
        }
    }

    if (flaggs->suncount == 2) {
        double m = flaggs->star[0].ptr->mas + flaggs->star[1].ptr->mas;
        double n = flaggs->star[0].ptr->mas / flaggs->star[1].ptr->mas;

        if (suction) {
            flaggs->star[0].ptr->orb = (2.0 + 6.0 * float_rand()) * flaggs->star[0].ptr->rad;
            flaggs->star[1].ptr->orb = (2.0 + 6.0 * float_rand()) * flaggs->star[1].ptr->rad;

            if (flaggs->star[0].ptr->ecc < 0.35)
                flaggs->star[0].ptr->ecc *= 2.0;
        } else {
            q = 1.0e9 * float_rand();
            if (flaggs->star[0].ptr->moon_count > 0) {
                xxx = flaggs->star[0].ptr->moons;
                xxx += flaggs->star[0].ptr->moon_count - 1;
                q += xxx->orb;
            }
            if (flaggs->star[1].ptr->moon_count > 0) {
                yyy = flaggs->star[1].ptr->moons;
                yyy += flaggs->star[1].ptr->moon_count - 1;
                q += yyy->orb;
            }

            flaggs->star[0].ptr->orb = 1.0e6 * float_rand();
            flaggs->star[1].ptr->orb = flaggs->star[0].ptr->orb * n;

            while (flaggs->star[0].ptr->orb < q || flaggs->star[1].ptr->orb < q) {
                flaggs->star[0].ptr->orb += flaggs->star[0].ptr->orb;
                flaggs->star[1].ptr->orb += flaggs->star[0].ptr->orb * n;
            }
        }

        flaggs->star[1].ptr->ecc = flaggs->star[0].ptr->ecc;
        flaggs->star[1].ptr->orb = -flaggs->star[1].ptr->orb;
        flaggs->star[0].ptr->yer = newton(flaggs->star[0].ptr->orb, m);
        flaggs->star[1].ptr->yer = flaggs->star[0].ptr->yer;
    }
}

/**********************************************************************
 *  generate_full_color()  -
 ***********************************************************************/
static uint32 generate_full_color(void)

{
    uint32 r, g, b, max;

    r = rand() & 0xff;
    g = rand() & 0xff;
    b = rand() & 0xff;

    if (r >= g && r >= b)
        max = r;
    if (g >= b && g >= r)
        max = g;
    if (b >= r && b >= g)
        max = b;

    r = (r * 255) / max;
    g = (g * 255) / max;
    b = (b * 255) / max;

    return (0xff000000 | (b << 16) | (g << 8) | r);
}

/**********************************************************************
 *  do_da_eclipse()  -
 ***********************************************************************/
static sint32 do_da_eclipse(t_boss *flaggs, D3 *p, flot32 r)

{
    V3 v;
    D3 *sun;
    P3 c;
    flot32 R, d, D, t, w, f, g;

    R = flaggs->star[flaggs->suun_current].ptr->rad;
    sun = &flaggs->star[flaggs->suun_current].posit;

    v.x = p->x - sun->x;
    v.y = p->y - sun->y;
    v.z = p->z - sun->z;
    D = fsqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    v.x /= D;
    v.y /= D;
    v.z /= D;

    d = r * D / (R - r);

    t = -v.x * p->x - v.y * p->y - v.z * p->z;

    if (t <= 0.0 || t >= d) {
        if (!(Counter.flags & ECLIP_FLAG))
            Counter.feclipse = 0.0;
        return (0);
    }

    c.x = p->x + t * v.x;
    c.y = p->y + t * v.y;
    c.z = p->z + t * v.z;

    f = r * (d - t) / d;
    g = (r + R) * (t + D) / D - R;

    w = c.x * c.x + c.y * c.y + c.z * c.z;

    if (w > g * g) {
        if (!(Counter.flags & ECLIP_FLAG))
            Counter.feclipse = 0.0;
        return (0);
    } else if (w < f * f) {
        Counter.flags |= ECLIP_FLAG;
        Counter.feclipse = 1.0;
        return (1);
    } else {
        Counter.flags |= ECLIP_FLAG;
        w = fsqrt(w);
        t = (w - g) / (f - g);
        Counter.feclipse = t * t;
        return (1);
    }
}

/**********************************************************************
 *  generate_ringworld_system()  -
 ***********************************************************************/
static t_system *generate_ringworld_system(sint32 index, t_stars *s)

{
    t_system *xxx, *yyy;
    char ch[32];

    if ((xxx = (t_system *)malloc(sizeof(t_system))) == 0) {
        printf("ERROR: solar system generation malloc failed\n");
        exit(0);
    }

    if (Counter.z == SOL_Z_GRID && Counter.x == SOL_X_GRID) {
        if (!strcmp(StarName[index], "")) {
            sprintf(ch, "%d-%d,%d", index, Counter.x, Counter.z);
            strcpy(xxx->name, ch);
        } else
            strcpy(xxx->name, StarName[index]);
    } else {
        sprintf(ch, "%d-%d,%d", index, Counter.x, Counter.z);
        strcpy(xxx->name, ch);
    }

    xxx->orb = 5500.0;
    xxx->ecc = 0.0;
    xxx->inc = 0.0;
    xxx->rad = 700000.0;
    xxx->mas = 2.0e30;
    xxx->apo = 0.0;
    xxx->yer = 9.0;
    xxx->day = 1.0;
    xxx->ee = 0.0;
    xxx->ww = 0.0;
    xxx->omega = 0.0;
    xxx->r1 = 0.0;
    xxx->r2 = 0.0;
    xxx->ring[0] = '\0';

    xxx->col = 0xff00ffff;
    xxx->tess = 3;
    xxx->texsz = 0;
    xxx->scale = 0.0;
    xxx->lxsize = 0;
    xxx->lysize = 0;
    xxx->lformat = 0;
    xxx->elev[0] = '\0';
    xxx->cxsize = 0;
    xxx->cysize = 0;
    xxx->cformat = 0;
    xxx->colr[0] = '\0';

    if ((yyy = (t_system *)malloc(sizeof(t_system))) == 0) {
        printf("ERROR: planet system generation malloc failed\n");
        exit(0);
    }

    xxx->moon_count = 1;
    xxx->moons = yyy;

    strcpy(yyy->name, "Ringworld");
    yyy->orb = 152800000.0;
    yyy->ecc = 0.0;
    yyy->inc = 0.0;
    yyy->rad = 0.0;
    yyy->mas = 2.0e27;
    yyy->apo = 0.0;
    yyy->yer = 9.037;
    yyy->day = 0.0;
    yyy->ee = 0.0;
    yyy->ww = 0.0;
    yyy->omega = 0.0;
    yyy->r1 = 0.0;
    yyy->r2 = 0.0;
    yyy->ring[0] = '\0';

    yyy->col = 0xff00ffff;
    yyy->tess = 0x54;
    yyy->texsz = 0;
    yyy->scale = 0.0;
    yyy->lxsize = 0;
    yyy->lysize = 0;
    yyy->lformat = 0;
    yyy->elev[0] = '\0';
    yyy->cxsize = 0;
    yyy->cysize = 0;
    yyy->cformat = 0;
    yyy->colr[0] = '\0';

    yyy->moon_count = 0;
    yyy->moons = NULL;

    return (xxx);
}

/**********************************************************************
 *  star_params()  -
 **********************************************************************/
void star_params(t_stars *st, flot32 *temp, flot32 *radi, flot64 *mass)

{
    sint32 i;
    flot32 r, g, b, a;
    char cs[4];

    st->a = 1.00;
    *temp = 2500.0 * fexp(flog(fsqrt(2.0)) * 7.0 * st->scass);
    *radi = fexp(flog(10.0) * (10.0 - 14.0 * st->scass - st->abs_mag) / 5.0);
    *mass = 2.0e30 * (*radi) * (*radi) * (*radi);
    *radi *= 700000.0;

    *(temp + 1) = 2500.0 * fexp(flog(fsqrt(2.0)) * 7.0 * st->scass2);
    *(radi + 1) = fexp(flog(10.0) * (10.0 - 14.0 * st->scass2 - st->abs_mag2) / 5.0);
    *(mass + 1) = 2.0e30 * (*(radi + 1)) * (*(radi + 1)) * (*(radi + 1));
    *(radi + 1) *= 700000.0;

    if (st->scass < 0.10) {
        cs[0] = 'M';
        i = 10.0 * (0.10 - st->scass) / 0.10;
        cs[1] = '0' + i;
        st->r = 1.00;
        st->g = 0.00;
        st->b = 0.00;
    } else if (st->scass < 0.28) {
        cs[0] = 'K';
        i = 10.0 * (0.28 - st->scass) / 0.18;
        cs[1] = '0' + i;
        st->r = 1.00;
        st->g = 0.50;
        st->b = 1.00;
    } else if (st->scass < 0.37) {
        cs[0] = 'G';
        i = 10.0 * (0.37 - st->scass) / 0.09;
        cs[1] = '0' + i;
        st->r = 1.00;
        st->g = 1.00;
        st->b = 0.00;
    } else if (st->scass < 0.45) {
        cs[0] = 'F';
        i = 10.0 * (0.45 - st->scass) / 0.08;
        cs[1] = '0' + i;
        st->r = 0.50;
        st->g = 1.00;
        st->b = 1.00;
    } else if (st->scass < 0.58) {
        cs[0] = 'A';
        i = 10.0 * (0.58 - st->scass) / 0.13;
        cs[1] = '0' + i;
        st->r = 0.00;
        st->g = 1.00;
        st->b = 0.00;
    } else if (st->scass < 0.89) {
        cs[0] = 'B';
        i = 10.0 * (0.89 - st->scass) / 0.31;
        cs[1] = '0' + i;
        st->r = 0.00;
        st->g = 1.00;
        st->b = 0.50;
    } else {
        cs[0] = 'O';
        i = 10.0 * (1.00 - st->scass) / 0.11;
        cs[1] = '0' + i;
        st->r = 0.00;
        st->g = 0.00;
        st->b = 1.00;
    }

    if (Counter.flags & NDPNT_FLAG) {
        st->r = 1.0;
        st->g = 1.0;
        st->b = 1.0;
    }
}

/**********************************************************************
 *  newton()  -
 **********************************************************************/
static flot64 newton(flot64 orb, flot64 mas)

{
    return (sqrt(orb * orb * orb * 4.0 * M_PI * M_PI / (GRAV * mas)) / (24.0 * 3600.0));
}

/**********************************************************************
 *  float_rand()  -
 **********************************************************************/
flot32 float_rand(void)

{
    sint32 temp;
    flot32 *fd = (flot32 *)&temp;

    temp = 0x3f800000 | (rand() << 8);

    return (*fd - 1.0);
}

/**********************************************************************
 *  calculate_orbital_params()  -
 **********************************************************************/
void calculate_orbital_params(D3 *bd, flot32 GMm, t_system *myorb)

{
    P4 r, c;
    V3 v;
    flot32 inc, ome, uuu, a, ecc, p, vel, E, V, q, M, n, t, tt;

    vel = Counter.S;
    v.x = Counter.vnorm.x * vel;
    v.y = Counter.vnorm.y * vel;
    v.z = Counter.vnorm.z * vel;

    r.x = -bd->x;
    r.y = -bd->y;
    r.z = -bd->z;
    r.w = fsqrt(r.x * r.x + r.y * r.y + r.z * r.z);

    c.x = r.y * v.z - r.z * v.y;
    c.y = r.z * v.x - r.x * v.z;
    c.z = r.x * v.y - r.y * v.x;
    c.w = fsqrt(c.x * c.x + c.y * c.y + c.z * c.z);

    inc = fatan2(fsqrt(c.x * c.x + c.y * c.y), c.z);
    ome = fatan2(c.x, -c.y);
    uuu = fatan2(-bd->x * fcos(ome) - bd->y * fsin(ome), -bd->z / fsin(inc));

    a = 2.0 / r.w - vel * vel / GMm;

    p = c.w * c.w / GMm;
    ecc = fsqrt(1.0 - p * a);
    q = p / (1.0 + ecc);

    /* elliptic */
    if (a > 0.0) {
        E = fatan2(1.0 - r.w * a, (r.x * v.x + r.y * v.y + r.z * v.z) / fsqrt(a * GMm));
        V = 2.0 * fatan(sqrt((1 + ecc) / (1 - ecc)) * ftan(0.5 * E));

        M = E - ecc * fsin(E);
        n = fsqrt(GMm * a * a * a);
        t = M / n;
    }

    /* hyperbolic */
    else if (a < 0.0) {
        E = asinh((r.x * v.x + r.y * v.y + r.z * v.z) / (ecc * fsqrt(2.0 * GMm / -a)));
        V = 2.0 * fatan(fsqrt((ecc + 1) / (ecc - 1)) * ftanh(0.5 * E));

        t = fsqrt(1.0 / (GMm * a * a * a)) * (ecc * fsinh(E) - E);
    }

    /* parabolic */
    else {
        n = (r.x * v.x + r.y * v.y + r.z * v.z) / fsqrt(2.0 * q * GMm);

        t = q * fsqrt(2.0 * q / GMm) * (n * n * n / 3.0 + n);
    }

    tt = Counter.D - t / (24.0 * 3600.0);

    myorb->orb = 1.0 / a;
    myorb->ecc = ecc;
    myorb->inc = inc * RTOD;
    myorb->ee = tt;
    myorb->ww = (uuu - V + ome) * RTOD;
    myorb->omega = ome * RTOD;
    myorb->yer = fsqrt(4.0 * M_PI * M_PI / (GMm * a * a * a));
    myorb->day = 1.0;
}

/**********************************************************************
 *  check_data_file()  -
 **********************************************************************/
static long check_data_file(char *name)

{
    FILE *fd;

    if (fd = fopen(datatrail(name), "r")) {
        fclose(fd);
        return (1);
    } else {
        char str[256];

        strcpy(str, datatrail(name));
        strcat(str, ".Z");

        if (fd = fopen(str, "r")) {
            fclose(fd);

            strcpy(str, "/usr/bsd/uncompress ");
            strcat(str, datatrail(name));
            strcat(str, ".Z");

            return (!system(str));
        }

        return (0);
    }
}
