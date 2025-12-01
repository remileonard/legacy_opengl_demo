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
#ifndef _WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#else
/* Windows doesn't need IPC headers for this implementation */
#include <windows.h>

/* Windows compatibility functions for SGI IRIX functions */

/* sginap - sleep for specified number of clock ticks (1/100 second on IRIX) */
static void sginap(long ticks)
{
    Sleep((DWORD)(ticks * 10)); /* Convert to milliseconds */
}

/* shmdt - detach shared memory segment (no-op on Windows for this port) */
static int shmdt(const void *shmaddr)
{
    /* On Windows, we don't use shared memory for this port */
    return 0;
}

#endif
#ifdef SOUND
#include <audio.h>
#endif
#include "space.h"
#include <stdint.h>

static Matrix idmatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

static Matrix hermite = {2.0, -3.0, 0.0, 1.0, -2.0, 3.0, 0.0, 0.0, 1.0, -2.0, 1.0, 0.0, 1.0, -1.0, 0.0, 0.0};

extern V4 raww[NUM_CLIP_PLANES];
extern t_galaga ggaa[STARSQ][STARSQ];
extern sint32 timer_flag;

extern char mem_col_nme[32];
extern char mem_ele_nme[32];
extern char *mem_col_ptr;
extern char *mem_ele_ptr;

t_stopwatch Counter;
V4 plum[NUM_CLIP_PLANES];
static t_stars dodo[DODO_SIZE];
static t_stars erikl[NUMBER_OF_STARS];
char StarName[NUMBER_OF_STARS][16];
flot32 lut[4096];
static t_const constellation;

#if STATS
static long feed_buff[256];
static sint32 statistics(long);
#endif

/**********************************************************************
 *  initialize_time()
 **********************************************************************/
void initialize_time(void)

{
    Counter.click = 0;
    Counter.timer_old = check_timer();
    Counter.timer = check_timer();

    reverse_julian_date(Counter.D, Counter.date);
}

/**********************************************************************
 *  spQuantifyMachine()
 **********************************************************************/
void spQuantifyMachine(void) {
    printf("DEBUG: spQuantifyMachine() START - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
           Counter.flags, Counter.stars_per_square);
    printf("DEBUG: Address of Counter = %p\n", (void*)&Counter);
    printf("DEBUG: Address of Counter.flags = %p\n", (void*)&Counter.flags);
    printf("DEBUG: Address of Counter.stars_per_square = %p\n", (void*)&Counter.stars_per_square);
    
    Counter.alpha = 0;
    Counter.flags = 0;
    uint32_t flags = 0;
    flags = TEXTR_FLAG;
    flags = 0;
    flags |= TEXTR_FLAG;
    Counter.flags = TEXTR_FLAG;
    
    printf("DEBUG: After direct assignment - Counter.flags = 0x%08X (should be 0x%08X)\n", 
           Counter.flags, TEXTR_FLAG);
    
    switch (Counter.winst.hwid) {
    case SP_HW_RE:
        Counter.alpha |= HW_AAPNT;
        Counter.alpha |= HW_AALIN;
        Counter.alpha |= HW_FATPT;
        Counter.flags = Counter.flags | TEXTR_FLAG;
        Counter.hw_graphics = 0;
        Counter.stars_per_square = 256;
        Counter.cutoff = 0.05;
        printf("DEBUG: SP_HW_RE - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
               Counter.flags, Counter.stars_per_square);
        break;

    case SP_HW_VGX:
        Counter.alpha |= HW_AAPNT;
        Counter.alpha |= HW_AALIN;
        Counter.alpha |= HW_FATPT;
        Counter.flags |= TEXTR_FLAG;
        Counter.hw_graphics = 0;
        Counter.stars_per_square = 192;
        Counter.cutoff = 0.05;
        printf("DEBUG: SP_HW_VGX - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
               Counter.flags, Counter.stars_per_square);
        break;

    case SP_HW_GT:
        Counter.hw_graphics = -1;
        Counter.stars_per_square = 160;
        Counter.cutoff = 0.05;
        printf("DEBUG: SP_HW_GT - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
               Counter.flags, Counter.stars_per_square);
        break;

    case SP_HW_LG:
        Counter.alpha |= HW_SOUND;
        Counter.alpha |= HW_FATPT;
        Counter.flags |= SLOWZ_FLAG;
        Counter.hw_graphics = -2;
        Counter.stars_per_square = 128;
        Counter.cutoff = 0.10;
        printf("DEBUG: SP_HW_LG - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
               Counter.flags, Counter.stars_per_square);
        break;

    case SP_HW_XG:
        Counter.alpha |= HW_AAPNT;
        Counter.alpha |= HW_AALIN;
        Counter.alpha |= HW_SOUND;
        Counter.alpha |= HW_FATPT;
        Counter.hw_graphics = -1;
        Counter.stars_per_square = 160;
        Counter.cutoff = 0.05;
        printf("DEBUG: SP_HW_XG - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
               Counter.flags, Counter.stars_per_square);
        break;

    default:
    case SP_HW_PI:
    case SP_HW_UNKNOWN:
        Counter.hw_graphics = -2;
        Counter.stars_per_square = 128;
        Counter.cutoff = 0.05;
        printf("DEBUG: SP_HW_UNKNOWN - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
               Counter.flags, Counter.stars_per_square);
        break;
    }
    
    printf("DEBUG: spQuantifyMachine() END - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
           Counter.flags, Counter.stars_per_square);
    fflush(stdout);
}

/**********************************************************************
 *  initialize_shmem()
 **********************************************************************/
void initialize_shmem(t_boss *flaggs)

{
    char ghw[64];
    t_body *tb;
    sint32 i, j, m[3];
    flot32 *d;

    printf("DEBUG: initialize_shmem() START - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
           Counter.flags, Counter.stars_per_square);

    mem_col_nme[0] = '\0';
    mem_ele_nme[0] = '\0';

    if ((mem_ele_ptr = (char *)malloc(MAX_ELEV_BYTES)) == NULL) {
        printf("INTERNAL ERROR: malloc failed for elevation data array\n");
        exit(0);
    }

    if ((mem_col_ptr = (char *)malloc(MAX_COLR_BYTES)) == NULL) {
        printf("INTERNAL ERROR: malloc failed for color data array\n");
        exit(0);
    }

    for (d = (flot32 *)&j, i = 0; i < 4096; i++) {
        j = (i << 19);
        lut[i] = exp(-0.75 * log(*d));
    }

    Counter.noroll = 0;
    Counter.attach = 1;
    Counter.timacc = 1.0;
    Counter.S = 20.0;

    spCopyMatrix((flot32 *)Counter.mat, (flot32 *)idmatrix);

    init_stars();

    Counter.constobj = init_cnstl();

    Counter.x = SOL_X_GRID;
    Counter.y = 0;
    Counter.z = SOL_Z_GRID;
    Counter.infoco = STELL_COLR;
    Counter.status = STELL_STAT;
    Counter.star_current = 0;

    generate_star_squares();

    printf("Nearest star is %s\n", StarName[Counter.star_current]);

    create_solar_system(Counter.star_current, flaggs, &ggaa[Counter.z][Counter.x]);

    /* set initial starting position at space station */
    Counter.eye.x = 0.0;
    Counter.eye.y = 0.0;
    Counter.eye.z = 0.0;
    scan_star_system(flaggs);

    tb = (t_body *)flaggs->star[0].next[2];
    Counter.eye.x = tb->posit.x;
    Counter.eye.y = tb->posit.y;
    Counter.eye.z = tb->posit.z;
    scan_star_system(flaggs);

    Counter.eye.x += flaggs->stat.posit.x - 500.0;
    Counter.eye.y += flaggs->stat.posit.y;
    Counter.eye.z += flaggs->stat.posit.z;

    Counter.locun = 0;
    Counter.galobj[0] = make_galaxy_object(0);
    Counter.galobj[1] = make_galaxy_object(1);

    Counter.starobj = generate_galaxy_stars();

    if (Counter.alpha & HW_FATPT)
        glPointSize(2.0);

#ifndef _WIN32
    if (Counter.alpha & HW_SOUND) {
        if ((Counter.shmid = shmget(0, 32, 0x3b6)) == -1) {
            printf("INTERNAL ERROR: shmget() failed\n");
            exit(1);
        }

        if ((sint32)(Counter.shmad = (sint32 *)shmat(Counter.shmid, 0, SHM_RND)) == -1) {
            printf("INTERNAL ERROR: shmat() failed\n");
            exit(1);
        }
    }
#else
    /* Shared memory not supported on Windows */
    Counter.shmid = -1;
    Counter.shmad = NULL;
#endif

    if (glIsList(Counter.locun))
        glDeleteLists(Counter.locun, 1);

    Counter.locun = glGenLists(1);
    glNewList(Counter.locun, GL_COMPILE);
    draw_all_them_stars(0);
    glEndList();

    spRingBell();
    
    printf("DEBUG: initialize_shmem() END - Counter.flags = 0x%08X, Counter.stars_per_square = %d\n", 
           Counter.flags, Counter.stars_per_square);
}

/**********************************************************************
 *  read_time()
 **********************************************************************/
void read_time(void)

{
    Counter.click++;
    Counter.timer_old = Counter.timer;
    Counter.timer = check_timer();

    if ((Counter.click & 15) == 15)
        reverse_julian_date(Counter.D, Counter.date);
}

/**********************************************************************
 *  evaluate_mouse()
 **********************************************************************/
void evaluate_mouse(t_boss *flaggs)

{
    sint32 i, lm, mm, rm;
    flot32 c1, c2, c3, s1, s2, s3, xpos, ypos, Droll, Dpitch, Dyaw;
    Matrix m;
    V4 *c;
    static sint32 frame_sleep = 0;
    static sint32 history = 0;
    sint32 pvbuf[8];
    flot64 vel;

    xpos = Counter.mouse_x;
    ypos = Counter.mouse_y;

    lm = Counter.mouse_b & SP_LMOUSE;
    mm = Counter.mouse_b & SP_MMOUSE;
    rm = Counter.mouse_b & SP_RMOUSE;

    if ((lm && !mm && !rm) || (!lm && mm && !rm)) {
        history++;
        if (history > 127)
            history = 127;
    } else if (!lm && !mm && !rm) {
        history--;
        if (history < 0)
            history = 0;
    }

    frame_sleep--;

    if (lm && (frame_sleep < 0) && (Counter.flags & PANEL_FLAG)) {
        check_menu(flaggs);
        frame_sleep = ((Counter.fps < 3.0) ? 1 : Counter.fps / 3.0);
    }

    if (!(Counter.flags & SHIFT_FLAG)) {
        if (rm && !lm && !mm) {
            Counter.noroll = !Counter.noroll;
            sginap(5);
        }

        if (lm && mm && rm)
            el_cheato(flaggs, 3, Counter.mat);
        else if (lm && mm)
            el_cheato(flaggs, 0, Counter.mat);
        else if (mm && rm)
            el_cheato(flaggs, 1, Counter.mat);
        else if (lm && rm)
            el_cheato(flaggs, 2, Counter.mat);

        xpos = (xpos - Counter.winorigx - Counter.winsizex / 2) / 400.0;
        ypos = (ypos - Counter.winorigy - Counter.winsizey / 2) / 400.0;

        Droll = ((Counter.noroll) ? 0 : M_PI * xpos / 180.0);
        Dpitch = M_PI * ypos / 180.0;
        Dyaw = M_PI * xpos / 720.0;

        /* set volume */
#ifdef SOUND
        if ((Counter.alpha & HW_SOUND) && (Counter.flags & SOUND_FLAG)) {
            c1 = 3000.0 * (fabs(Dyaw) + fabs(Dpitch) + fabs(Droll));
            if (c1 > 127)
                c1 = 127.0;
            pvbuf[0] = AL_LEFT_SPEAKER_GAIN;
            pvbuf[1] = c1 + history;
            pvbuf[2] = AL_RIGHT_SPEAKER_GAIN;
            pvbuf[3] = pvbuf[1];
            ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 4);

            Counter.shmad[0] = pvbuf[1];
        }
#endif

        c1 = fcos(Dyaw);
        s1 = fsin(Dyaw);
        c2 = fcos(Dpitch);
        s2 = fsin(Dpitch);
        c3 = fcos(Droll);
        s3 = fsin(Droll);

        m[0][0] = c1 * c3 + s3 * s2 * s1;
        m[0][1] = s3 * c2;
        m[0][2] = -c3 * s1 + s3 * s2 * c1;
        m[0][3] = 0.0;
        m[1][0] = -s3 * c1 + c3 * s2 * s1;
        m[1][1] = c3 * c2;
        m[1][2] = s1 * s3 + c3 * s2 * c1;
        m[1][3] = 0.0;
        m[2][0] = c2 * s1;
        m[2][1] = -s2;
        m[2][2] = c1 * c2;
        m[2][3] = 0.0;
        m[3][0] = 0.0;
        m[3][1] = 0.0;
        m[3][2] = 0.0;
        m[3][3] = 1.0;

        spMultMatrix((flot32 *)Counter.mat, (flot32 *)Counter.mat, (flot32 *)m);

        for (c = Counter.clop, i = 0; i < NUM_CLIP_PLANES; c++, i++) {
            c->x = Counter.mat[0][0] * raww[i].x + Counter.mat[0][1] * raww[i].y + Counter.mat[0][2] * raww[i].z;
            c->y = Counter.mat[1][0] * raww[i].x + Counter.mat[1][1] * raww[i].y + Counter.mat[1][2] * raww[i].z;
            c->z = Counter.mat[2][0] * raww[i].x + Counter.mat[2][1] * raww[i].y + Counter.mat[2][2] * raww[i].z;
            c->w =
                ((Counter.status == STELL_STAT) ? 0.0
                                                : -c->x * Counter.eye.x - c->y * Counter.eye.y - c->z * Counter.eye.z);
        }

        if (!(Counter.flags & AUTOP_FLAG)) {
            vel = Counter.S * ((Counter.status != STELL_STAT) ? PRTOKM : 1.0);

            if (lm && !mm && !rm) {
                if (vel > 0.80 * LIGHTSPEED && vel < 0.999 * LIGHTSPEED)
                    vel += 0.08 * (LIGHTSPEED - vel);
                else
                    vel *= 1.1;
                Counter.attach = 0;
            }
            if (mm && !lm && !rm) {
                if (vel > LIGHTSPEED && vel < 1.1 * LIGHTSPEED)
                    vel = 0.9985 * LIGHTSPEED;
                else if (vel > 0.80 * LIGHTSPEED && vel < 0.999 * LIGHTSPEED)
                    vel -= 0.08 * (LIGHTSPEED - vel);
                else
                    vel /= 1.1;
            }

            Counter.S = vel / ((Counter.status != STELL_STAT) ? PRTOKM : 1.0);
        }

        if (Counter.alpha & HW_SOUND) {
            vel = Counter.S * ((Counter.status != STELL_STAT) ? PRTOKM : 1.0);
            Counter.shmad[1] = ((vel >= LIGHTSPEED) ? 1 : 0);
        }

        Counter.enorm.x = -Counter.mat[0][2];
        Counter.enorm.y = -Counter.mat[1][2];
        Counter.enorm.z = -Counter.mat[2][2];

        if (!(Counter.flags & CNTRL_FLAG)) {
            Counter.vnorm.x = Counter.enorm.x;
            Counter.vnorm.y = Counter.enorm.y;
            Counter.vnorm.z = Counter.enorm.z;
        }
    }

    if (Counter.flags & AUTOP_FLAG) {
        Counter.attach = 0;
        take_me_there(&Counter.spline);
    }

    if (Counter.attach) {
        Counter.eye.x += flaggs->stat.posit.x - 500.0;
        Counter.eye.y += flaggs->stat.posit.y;
        Counter.eye.z += flaggs->stat.posit.z;
    } else if (!(Counter.flags & AUTOP_FLAG)) {
        c1 = Counter.S * ((Counter.flags & VELOC_FLAG) ? -1.0 : 1.0) * (Counter.timer - Counter.timer_old);

        if (!(Counter.flags & FREEZ_FLAG)) {
            Counter.eye.x += Counter.vnorm.x * c1;
            Counter.eye.y += Counter.vnorm.y * c1;
            Counter.eye.z += Counter.vnorm.z * c1;
        }
    }

    Counter.flags &= ~FREEZ_FLAG;
}

/**********************************************************************
 *  key_press()  -
 **********************************************************************/
void key_press(t_boss *flaggs, sint32 v)

{
    char ch[64];

    switch (v) {
    case SP_IO_esc:
        printf("Total Frames = %d\n", Counter.click);
        if (Counter.alpha & HW_SOUND)
            sound_control(0, 0);
        if (Counter.alpha & HW_SOUND)
            shmdt(Counter.shmad);
        exit(1);
        break;

    case SP_IO_a:
        if (Counter.flags & AUTOP_FLAG) {
            Counter.flags &= ~AUTOP_FLAG;
            if (Counter.flags & PANEL_FLAG)
                draw_item(SP_IO_a, 0);
            break;
        }

        if (!autopilot_menu_stuff(flaggs))
            break;

        if (Counter.flags & TMREV_FLAG) {
            Counter.flags &= ~TMREV_FLAG;

            if (Counter.flags & PANEL_FLAG)
                draw_item(SP_IO_a, 0);
        }
        if (Counter.timacc < 1.0) {
            Counter.timacc = 1.0;
            if (Counter.flags & PANEL_FLAG)
                draw_item(SP_IO_tco, 0);
        } else if (Counter.timacc > 10.0) {
            Counter.timacc = 10.0;
            if (Counter.flags & PANEL_FLAG)
                draw_item(SP_IO_tco, 0);
        }

        if (Counter.flags & PANEL_FLAG)
            draw_item(SP_IO_a, 0);
        break;

    case SP_IO_s:
        if (Counter.status != STELL_STAT)
            break;

        if (Counter.flags & STATS_FLAG)
            Counter.flags &= ~STATS_FLAG;
        else
            Counter.flags |= STATS_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(SP_IO_s, 0);

        stats_menu_stuff(flaggs);

        if (Counter.flags & STATS_FLAG)
            Counter.flags &= ~STATS_FLAG;
        else
            Counter.flags |= STATS_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(SP_IO_s, 0);
        break;

    case SP_IO_x:
        if (Counter.flags & NOTXT_FLAG)
            Counter.flags &= ~NOTXT_FLAG;
        else
            Counter.flags |= NOTXT_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_h:
        if (Counter.flags & HELPP_FLAG)
            Counter.flags &= ~HELPP_FLAG;
        else
            Counter.flags |= HELPP_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_l:
        if (Counter.flags & SHADE_FLAG)
            Counter.flags &= ~SHADE_FLAG;
        else
            Counter.flags |= SHADE_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_v:
        if (Counter.flags & VELOC_FLAG)
            Counter.flags &= ~VELOC_FLAG;
        else
            Counter.flags |= VELOC_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_d:
        if (Counter.flags & DEBUG_FLAG)
            Counter.flags &= ~DEBUG_FLAG;
        else
            Counter.flags |= DEBUG_FLAG;
        break;

    case SP_IO_i:
        if (Counter.flags & MRBIT_FLAG)
            Counter.flags &= ~MRBIT_FLAG;
        else
            Counter.flags |= MRBIT_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_o:
        if (Counter.flags & PRBIT_FLAG)
            Counter.flags &= ~PRBIT_FLAG;
        else
            Counter.flags |= PRBIT_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_tid:
        if (Counter.flags & TMREV_FLAG)
            Counter.flags &= ~TMREV_FLAG;
        else
            Counter.flags |= TMREV_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_u:
        if (Counter.flags & USERM_FLAG)
            Counter.flags &= ~USERM_FLAG;
        else
            Counter.flags |= USERM_FLAG;
        break;

    case SP_IO_pri:
        if (Counter.flags & PRINT_FLAG)
            Counter.flags &= ~PRINT_FLAG;
        else
            Counter.flags |= PRINT_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_n:
        if (Counter.flags & STNAM_FLAG)
            Counter.flags &= ~STNAM_FLAG;
        else
            Counter.flags |= STNAM_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_snd:
        if (Counter.flags & SOUND_FLAG) {
            Counter.flags &= ~SOUND_FLAG;
            if (Counter.alpha & HW_SOUND)
                sound_control(0, 0);
        } else {
            Counter.flags |= SOUND_FLAG;
            if ((Counter.alpha & HW_SOUND) && (Counter.flags & SOUND_FLAG))
                sound_control(1, SND_BACK);
        }
        break;

    case SP_IO_b:
        if (Counter.flags & HIRES_FLAG) {
            Counter.flags &= ~HIRES_FLAG;
#if STATS
            statistics(0);
#endif
            if (Counter.winst.hwid & (SP_HW_XG | SP_HW_LG))
                Counter.flags &= ~TEXTR_FLAG;
        } else {
            Counter.flags |= HIRES_FLAG;
            if (Counter.winst.hwid & (SP_HW_XG | SP_HW_LG))
                Counter.flags |= TEXTR_FLAG;
#if STATS
            statistics(1);
#endif
        }

        if (Counter.flags & PANEL_FLAG)
            draw_menu();
        break;

    case SP_IO_z:
        if (Counter.flags & ZODAC_FLAG)
            Counter.flags &= ~ZODAC_FLAG;
        else
            Counter.flags |= ZODAC_FLAG;

        if (Counter.flags & PANEL_FLAG)
            draw_item(v, 0);
        break;

    case SP_IO_t:
        Counter.timacc *= 10.0;
        if (Counter.timacc > 1.0e11)
            Counter.timacc = 1.0e11;
        else if (Counter.timacc == 0.0)
            Counter.timacc = 0.01;

        if (Counter.flags & PANEL_FLAG)
            draw_item(SP_IO_tco, 0);
        break;

    case SP_IO_r:
        Counter.timacc *= 0.1;
        if (Counter.timacc < 0.0099)
            Counter.timacc = 0.0;

        if (Counter.flags & PANEL_FLAG)
            draw_item(SP_IO_tco, 0);
        break;

    case SP_IO_y:
        timer_flag = 0;
        Counter.timacc = 1.0;
        Counter.timer_old = check_timer();
        Counter.timer = check_timer();

        if (Counter.flags & PANEL_FLAG)
            draw_item(SP_IO_tco, 0);
        break;

    case SP_IO_dwn:
        Counter.hw_graphics--;
        Counter.flags |= REGEN_FLAG;
        break;

    case SP_IO_up:
        Counter.hw_graphics++;
        Counter.flags |= REGEN_FLAG;
        break;

    case SP_IO_q:
        if (Counter.flags & PANEL_FLAG)
            Counter.flags &= ~PANEL_FLAG;
        else
            Counter.flags |= PANEL_FLAG;

        set_window_view(300.0);
        draw_menu();
        break;

    case SP_IO_tco:
        draw_item(SP_IO_tco, 0);
        break;

    default:
        break;
    }
}

/**********************************************************************
 *  el_cheato()  -
 **********************************************************************/
static void el_cheato(t_boss *flaggs, sint32 flag, Matrix m)

{
    D3 p;
    t_body *tb, *tc;
    flot32 x, y, z;
    sint32 index;

    switch (flag) {
    case 0:
        if (Counter.status == STELL_STAT) {
            p = flaggs->star[flaggs->suun_current].posit;
            spLookMatrix(0.0, 0.0, 0.0, (flot32)p.x, (flot32)p.y, (flot32)p.z, m);
        } else if (Counter.status == GALAC_STAT) {
            t_galaga *ga = &ggaa[Counter.z][Counter.x];

            find_closest_star(&index, ga);
            if (index < 0)
                break;

            x = ga->stars[index].x - 0.5 * GALAXY_EDGE + (Counter.x + 0.5) * EDGESQ;
            y = ga->stars[index].y;
            z = ga->stars[index].z - 0.5 * GALAXY_EDGE + (Counter.z + 0.5) * EDGESQ;

            spLookMatrix(Counter.eye.x, Counter.eye.y, Counter.eye.z, x, y, z, m);
        } else if (Counter.status == COSMC_STAT)
            spLookMatrix(Counter.eye.x, Counter.eye.y, Counter.eye.z, 0.0, 0.0, 0.0, m);

        break;

    case 1:
        if (Counter.status != STELL_STAT || flaggs->plan_current == -1)
            return;
        tb = (t_body *)flaggs->star[flaggs->suun_current].next[flaggs->plan_current];
        p = tb->posit;

        spLookMatrix(0.0, 0.0, 0.0, (flot32)p.x, (flot32)p.y, (flot32)p.z, m);
        break;

    case 2:
        if (Counter.status != STELL_STAT || flaggs->plan_current == -1 || flaggs->moon_current == -1)
            return;
        tb = (t_body *)flaggs->star[flaggs->suun_current].next[flaggs->plan_current];
        tc = (t_body *)tb->next[flaggs->moon_current];
        p = tc->posit;

        spLookMatrix(0.0, 0.0, 0.0, (flot32)p.x, (flot32)p.y, (flot32)p.z, m);
        break;

    case 3:
        if (Counter.status == STELL_STAT && flaggs->suncount == 2) {
            if (flaggs->suun_current == 1)
                p = flaggs->star[0].posit;
            else
                p = flaggs->star[1].posit;

            spLookMatrix(0.0, 0.0, 0.0, (flot32)p.x, (flot32)p.y, (flot32)p.z, m);
        }
        break;

    default:
        break;
    }
}

/**********************************************************************
 *  take_me_there()  -
 **********************************************************************/
static void take_me_there(t_spline *tsp)

{
    flot64 t, a, b, c, d;

    t = (Counter.D - tsp->t1) / (tsp->t2 - tsp->t1);

    if (t < 0.0) {
        printf("USER ERROR: Something fishy about the flow of time.\n");
        Counter.flags &= ~AUTOP_FLAG;
        if (Counter.flags & PANEL_FLAG)
            draw_item(SP_IO_a, 0);
    } else if (t >= 1.0 || (Counter.star_current == tsp->newstar[0] && Counter.x == tsp->newstar[1] &&
                            Counter.z == tsp->newstar[2])) {
        Counter.flags &= ~AUTOP_FLAG;
        if (Counter.flags & PANEL_FLAG)
            draw_item(SP_IO_a, 0);

        if (Counter.spline.newstar[0] != -2) {
            Counter.S = 100000.0 * LIGHTSPEED;
            if (Counter.status != STELL_STAT)
                Counter.S *= KMTOPR;
        } else
            Counter.S = 100.0;

        spRingBell();
    } else {
        if (t <= 0.5)
            t = 2.0 * t * t;
        else
            t = -2.0 * (t - 1.0) * (t - 1.0) + 1.0;

        a = ((hermite[0][0] * t + hermite[0][1]) * t + hermite[0][2]) * t + hermite[0][3];
        b = ((hermite[1][0] * t + hermite[1][1]) * t + hermite[1][2]) * t + hermite[1][3];
        c = ((hermite[2][0] * t + hermite[2][1]) * t + hermite[2][2]) * t + hermite[2][3];
        d = ((hermite[3][0] * t + hermite[3][1]) * t + hermite[3][2]) * t + hermite[3][3];

        Counter.eye.x = tsp->p1.x * a + tsp->p2.x * b + tsp->v1.x * c + tsp->v2.x * d;
        Counter.eye.y = tsp->p1.y * a + tsp->p2.y * b + tsp->v1.y * c + tsp->v2.y * d;
        Counter.eye.z = tsp->p1.z * a + tsp->p2.z * b + tsp->v1.z * c + tsp->v2.z * d;
    }
}

/**********************************************************************
 *  scan_galactic_system()  -
 **********************************************************************/
void scan_galactic_system(t_boss *flaggs)

{
    sint32 q, index, count;
    flot32 d, x, y, z;
    D3 *eye = &Counter.eye;

    /* currently in stellar system */
    if (Counter.status == STELL_STAT) {
        if (eye->x * eye->x + eye->y * eye->y + eye->z * eye->z >= SOLSYS_EDGE * PRTOKM * PRTOKM) {
            t_stars *st = &ggaa[Counter.z][Counter.x].stars[Counter.star_current];

            eye->x = eye->x * KMTOPR + st->x + -0.5 * GALAXY_EDGE + (Counter.x + 0.5) * EDGESQ;
            eye->y = eye->y * KMTOPR + st->y;
            eye->z = eye->z * KMTOPR + st->z + -0.5 * GALAXY_EDGE + (Counter.z + 0.5) * EDGESQ;
            Counter.S *= KMTOPR;

            if (Counter.flags & AUTOP_FLAG) {
                t_galaga *ga = &ggaa[Counter.spline.newstar[2]][Counter.spline.newstar[1]];

                Counter.spline.p1.x = eye->x;
                Counter.spline.p1.y = eye->y;
                Counter.spline.p1.z = eye->z;

                Counter.spline.v1.x *= KMTOPR;
                Counter.spline.v1.y *= KMTOPR;
                Counter.spline.v1.z *= KMTOPR;

                Counter.spline.p2.x = ga->stars[Counter.spline.newstar[0]].x - 0.5 * GALAXY_EDGE +
                                      (Counter.spline.newstar[1] + 0.5) * EDGESQ;
                Counter.spline.p2.y = ga->stars[Counter.spline.newstar[0]].y;
                Counter.spline.p2.z = ga->stars[Counter.spline.newstar[0]].z - 0.5 * GALAXY_EDGE +
                                      (Counter.spline.newstar[2] + 0.5) * EDGESQ;

                Counter.spline.v2.x *= KMTOPR;
                Counter.spline.v2.y *= KMTOPR;
                Counter.spline.v2.z *= KMTOPR;
            }

            destroy_solar_system(flaggs);

            Counter.star_current = -1;
            Counter.status = GALAC_STAT;
            Counter.infoco = GALAC_COLR;
        } else {
            Counter.status = STELL_STAT;
            Counter.infoco = STELL_COLR;
            scan_star_system(flaggs);
        }
    }

    /* currently in galactic system */
    else if (Counter.status == GALAC_STAT) {
        Counter.x = (flot32)0.5 * STARSQ + eye->x / EDGESQ;
        Counter.y = (flot32)eye->y / EDGESQ;
        Counter.z = (flot32)0.5 * STARSQ + eye->z / EDGESQ;

        if (eye->x * eye->x + eye->z * eye->z > 0.25 * GALAXY_EDGE * GALAXY_EDGE || eye->y * eye->y > HEIGHT_ABOVE) {
            printf("Leaving galaxy\n");
            Counter.star_current = -1;
            Counter.status = COSMC_STAT;
            Counter.infoco = COSMC_COLR;
        } else {
            q = find_closest_star(&index, &ggaa[Counter.z][Counter.x]);

            if (q == STELL_STAT) {
                t_stars *st;

                if (glIsList(Counter.locun))
                    glDeleteLists(Counter.locun, 1);

                Counter.infoco = STELL_COLR;
                Counter.status = STELL_STAT;

                if (Counter.x == SOL_X_GRID && Counter.z == SOL_Z_GRID)
                    printf("New Star: %d  Name: %s\n", index, StarName[index]);
                else
                    printf("New Star: %d-%d,%d\n", index, Counter.x, Counter.z);

                create_solar_system(index, flaggs, &ggaa[Counter.z][Counter.x]);
                Counter.star_current = index;

                Counter.locun = glGenLists(1);
                glNewList(Counter.locun, GL_COMPILE);
                draw_all_them_stars(0);
                glEndList();

                st = &ggaa[Counter.z][Counter.x].stars[Counter.star_current];
                eye->x = PRTOKM * (eye->x - (-0.5 * GALAXY_EDGE + (Counter.x + 0.5) * EDGESQ + st->x));
                eye->y = PRTOKM * (eye->y - (st->y));
                eye->z = PRTOKM * (eye->z - (-0.5 * GALAXY_EDGE + (Counter.z + 0.5) * EDGESQ + st->z));
                Counter.S = 100000.0 * LIGHTSPEED;

                scan_star_system(flaggs);
            } else {
                Counter.status = GALAC_STAT;
                Counter.infoco = GALAC_COLR;
            }
        }
    }

    /* currently in cosmic system */
    else if (Counter.status == COSMC_STAT) {
        Counter.x = (flot32)0.5 * STARSQ + eye->x / EDGESQ;
        Counter.y = (flot32)eye->y / EDGESQ;
        Counter.z = (flot32)0.5 * STARSQ + eye->z / EDGESQ;

        if (eye->x * eye->x + eye->z * eye->z < 0.25 * GALAXY_EDGE * GALAXY_EDGE && eye->y * eye->y < HEIGHT_ABOVE) {
            printf("Entering galaxy\n");
            Counter.star_current = -1;
            Counter.infoco = GALAC_COLR;
            Counter.status = GALAC_STAT;
        } else {
            Counter.infoco = COSMC_COLR;
            Counter.status = COSMC_STAT;
        }
    } else {
        printf("INTERNAL ERROR: Unsupported case in scan_galactic_system()\n");
        exit(0);
    }
}

/**********************************************************************
 *  init_stars()  -
 **********************************************************************/
static void init_stars(void)

{
    sint32 i, j;
    FILE *fdi;
    t_stars *st;
    flot32 q[2], d, rmag, rmag2;
    flot64 u[2];
    char code[4], *su, c;

    printf("Loading stellar database\n");
    if ((fdi = fopen(datatrail("star.db"), "r")) == NULL) {
        printf("USER ERROR: Star database file not found: %s\n", datatrail("star.db"));
        exit(0);
    }

    st = erikl;
    su = StarName[0];

    st->x = 0.0;
    st->y = 0.0;
    st->z = 0.0;
    st->abs_mag = 4.83;
    st->fny_mag = exp(LN2 * (6.0 - st->abs_mag));
    st->scass = 0.34;
    st->abs_mag2 = 0.0;
    st->scass2 = 0.0;
    star_params(st, q, q, u);
    strcpy(su, "Sol");

    for (st++, su += 16, i = 1; i < NUMBER_OF_STARS; st++, su += 16, i++) {
        fscanf(fdi, "%f %f %f %f %c%c%c%c", &st->x, &st->y, &st->z, &rmag, &code[0], &code[1], &code[2], &code[3]);

        if (code[2] != '.')
            fscanf(fdi, "%f", &rmag2);
        else
            rmag2 = 0.0;

        for (j = 0, c = getc(fdi); c != '\n' && c != '\0'; c = getc(fdi))
            if (j != 0 || c != ' ')
                su[j++] = c;
        su[j] = '\0';

        d = st->x * st->x + st->y * st->y + st->z * st->z;
        st->abs_mag = rmag + 5.0 - 2.5 * log10(d);
        st->fny_mag = exp(LN2 * (6.0 - st->abs_mag));

        switch (code[0]) {
        case 'W':
            st->scass = 0.99;
            break;
        case 'O':
            st->scass = 0.95;
            break;
        case 'B':
            st->scass = 0.70;
            break;
        case 'A':
            st->scass = 0.55;
            break;
        case 'F':
            st->scass = 0.41;
            break;
        case 'G':
            st->scass = 0.32;
            break;
        case 'K':
            st->scass = 0.19;
            break;
        case 'M':
            st->scass = 0.05;
            break;
        case 'S':
            st->scass = 0.04;
            break;
        case 'R':
            st->scass = 0.03;
            break;
        case 'N':
            st->scass = 0.02;
            break;
        case 'C':
            st->scass = 0.01;
            break;
        case 'P':
            st->scass = 0.01;
            break;
        default:
            printf("INTERNAL ERROR: Star Fuckup %d (%c)\n", i, code[0]);
            exit(0);
            break;
        }

        if (code[2] != '.') {
            st->abs_mag2 = rmag2 + 5.0 - 2.5 * log10(d);

            switch (code[2]) {
            case 'W':
                st->scass2 = 0.99;
                break;
            case 'O':
                st->scass2 = 0.95;
                break;
            case 'B':
                st->scass2 = 0.70;
                break;
            case 'A':
                st->scass2 = 0.55;
                break;
            case 'F':
                st->scass2 = 0.41;
                break;
            case 'G':
                st->scass2 = 0.32;
                break;
            case 'K':
                st->scass2 = 0.19;
                break;
            case 'M':
                st->scass2 = 0.05;
                break;
            case 'S':
                st->scass2 = 0.04;
                break;
            case 'R':
                st->scass2 = 0.03;
                break;
            case 'N':
                st->scass2 = 0.02;
                break;
            case 'C':
                st->scass2 = 0.01;
                break;
            case 'P':
                st->scass2 = 0.01;
                break;
            default:
                printf("INTERNAL ERROR: Binary Star Fuckup %d (%c)\n", i, code[2]);
                exit(0);
                break;
            }
        } else {
            st->abs_mag2 = 0.0;
            st->scass2 = 0.0;
        }

        star_params(st, q, q, u);
    }
}

/**********************************************************************
 *  init_cnstl()  -
 **********************************************************************/
static sint32 init_cnstl(void)

{
    sint32 i, j, count, obj;
    FILE *fd;
    flot32 x, y, z, a, b, c1, s1;
    P4 *c;
    t_bordr *br;
    Matrix mb;

    if (!(fd = fopen(datatrail("const.db"), "r"))) {
        printf("USER ERROR: Constellation database file not found: %s\n", datatrail("const.db"));
        exit(0);
    }

    c1 = fcos(DTOR * 23.5);
    s1 = fsin(DTOR * 23.5);
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

    fscanf(fd, "%d", &constellation.arr_count);

    for (c = constellation.arr, i = 0; i < constellation.arr_count; c++, i++) {
        fscanf(fd, "%f %f", &x, &y);

        a = 2.0 * M_PI * (0.0 - x);
        b = M_PI * (y - 0.5);

        x = PRTOKM * fcos(b) * fsin(a);
        y = PRTOKM * fsin(b);
        z = PRTOKM * fcos(b) * fcos(a);

        c->x = mb[0][0] * x + mb[0][1] * y + mb[0][2] * z;
        c->y = mb[1][0] * x + mb[1][1] * y + mb[1][2] * z;
        c->z = mb[2][0] * x + mb[2][1] * y + mb[2][2] * z;
    }

    fscanf(fd, "%d", &constellation.brd_count);

    for (br = constellation.brd, i = 0; i < constellation.brd_count; br++, i++) {
        fscanf(fd, "%d", &br->count);
        for (j = 0; j < br->count; j++)
            fscanf(fd, "%d", &br->index[j]);
        fscanf(fd, "%f %f %s", &x, &y, br->name);

        a = 2.0 * M_PI * (0.0 - x);
        b = M_PI * (y - 0.5);

        x = PRTOKM * fcos(b) * fsin(a);
        y = PRTOKM * fsin(b);
        z = PRTOKM * fcos(b) * fcos(a);

        br->x = mb[0][0] * x + mb[0][1] * y + mb[0][2] * z;
        br->y = mb[1][0] * x + mb[1][1] * y + mb[1][2] * z;
        br->z = mb[2][0] * x + mb[2][1] * y + mb[2][2] * z;
    }

    fclose(fd);

    obj = glGenLists(1);
    glNewList(obj, GL_COMPILE);
    glColor4f(1.0, 0.0, 0.0, 1.0);

    for (br = constellation.brd, i = 0; i < constellation.brd_count; br++, i++) {
        glBegin(GL_LINE_LOOP);
        for (j = 0; j < br->count; j++)
            glVertex3fv(&constellation.arr[br->index[j]].x);
        glEnd();

        spDrawString(br->x, br->y, br->z, br->name);
    }

    glEndList();
    return (obj);
}

/**********************************************************************
 *  generate_star_squares()  -
 **********************************************************************/
static void generate_star_squares()

{
    sint32 i, j, k, m, temp[3], index;
    flot32 *fd = (flot32 *)temp;
    t_stars *st;
    t_galaga *ga;
    flot32 q[2];
    flot64 u[2];

    srand(0x2468);

    for (st = dodo, i = 0; i < DODO_SIZE; st++, i++) {
        temp[0] = 0x3f800000 | ((j = rand()) << 8);
        temp[1] = 0x3f800000 | ((k = rand()) << 8);
        temp[2] = 0x3f800000 | ((m = rand()) << 8);

        st->x = (fd[0] - 1.5) * EDGESQ;
        st->y = (fd[1] - 1.5) * 8.0 * EDGESQ;
        st->z = (fd[2] - 1.5) * EDGESQ;

        st->abs_mag = 8 - (k & 0xf);
        st->fny_mag = exp(LN2 * (6.0 - st->abs_mag));
        temp[0] = 0x3f800000 | (rand() << 8);
        st->scass = fd[0] - 1.0;
        st->scass *= st->scass;

        if ((rand() & 3) == 3) {
            st->abs_mag2 = 8 - (m & 0xf);
            temp[0] = 0x3f800000 | (rand() << 8);
            st->scass2 = fd[0] - 1.0;
            st->scass2 *= st->scass2;
        } else {
            st->abs_mag2 = 0.0;
            st->scass2 = 0.0;
        }

        star_params(st, q, q, u);
    }

    for (ga = ggaa[0], i = 0; i < STARSQ; i++)
        for (j = 0; j < STARSQ; ga++, j++) {
            ga->count = (Counter.stars_per_square * ga->count) / 1024.0;
            if (ga->count < 2)
                ga->count = 0;

            temp[0] = 0x3f800000 | (rand() << 8);
            fd[0] = (fd[0] - 1.0);
            fd[0] *= (DODO_SIZE - ga->count);
            ga->stars = &dodo[(sint32)fd[0]];
        }

    ggaa[Counter.z][Counter.x].count = NUMBER_OF_STARS;
    ggaa[Counter.z][Counter.x].stars = erikl;
}

/**********************************************************************
 *  generate_galaxy_stars()  -
 **********************************************************************/
static sint32 generate_galaxy_stars(void)

{
    P3 temp;
    sint32 i, j, k, col, obj, count;
    sint32 tp;
    flot32 xc, yc, zc, s, t, factor, *f = (flot32 *)&tp;
    t_galaga *ga;
    t_stars *st;

    obj = glGenLists(1);
    glNewList(obj, GL_COMPILE);
    glBegin(GL_POINTS);
    yc = 0.0;
    for (count = k = 0; k < STARSQ; k++)
        for (j = 0; j < STARSQ; j++) {
            ga = &ggaa[k][j];

            s = (flot32)(k + 0.5) / STARSQ;
            t = (flot32)(j + 0.5) / STARSQ;
            factor = 4.0 * fexp(-40.0 * ((s - 0.5) * (s - 0.5) + (t - 0.5) * (t - 0.5))) - yc;

            xc = 0.5 * GALAXY_EDGE - (j + 0.5) * EDGESQ;
            yc = 0.0;
            zc = 0.5 * GALAXY_EDGE - (k + 0.5) * EDGESQ;

            tp = 0x3f800000 | (rand() << 8);
            tp = 2.0 * (*f - 1.0) * Counter.stars_per_square;

            if (ga->count - 12 > tp) {
                st = &ga->stars[ga->count >> 1];
                count++;

                glColor4f(st->r, st->g, st->b, 0.25 * st->a);

                temp.x = xc - st->x;
                temp.y = yc - st->y * factor;
                temp.z = zc - st->z;
                glVertex3fv(&temp.x);
            }
        }

    glEnd();
    glEndList();

    return (obj);
}

/**********************************************************************
 *  star_square_clip_check()  -
 **********************************************************************/
void star_square_clip_check(sint32 mm[4], sint32 flag[18][18], P3 *eye)

{
    flot32 tt, sto, dx, dz, save[3];
    sint32 i, k, mask, q, *f;
    V4 *c;

    for (f = flag[0], i = 0; i < 36; f += 9, i++) {
        *(f + 0) = 0;
        *(f + 1) = 0;
        *(f + 2) = 0;
        *(f + 3) = 0;
        *(f + 4) = 0;
        *(f + 5) = 0;
        *(f + 6) = 0;
        *(f + 7) = 0;
        *(f + 8) = 0;
    }

    save[2] = EDGESQ;
    save[0] = -0.5 * GALAXY_EDGE + mm[2] * save[2];
    save[1] = -0.5 * GALAXY_EDGE + mm[0] * save[2];

    for (c = Counter.clop, mask = 1, k = NUM_CLIP_PLANES; k > 0; mask <<= 1, c++, k--) {
        sto = (flot32)4.0 * save[2] * ((c->y > 0.0) ? -c->y : c->y) + c->x * eye->x + c->y * eye->y + c->z * eye->z;

        dx = c->x * save[2];
        dz = c->z * save[2] - (flot32)17.0 * dx;
        tt = c->x * save[0] + c->z * save[1];

        for (f = flag[0], i = 0; i < 18; f += 18, i++) {
            q = *(f + 0);
            q |= mask;
            if (tt < sto)
                *(f + 0) = q;
            tt += dx;
            q = *(f + 1);
            q |= mask;
            if (tt < sto)
                *(f + 1) = q;
            tt += dx;
            q = *(f + 2);
            q |= mask;
            if (tt < sto)
                *(f + 2) = q;
            tt += dx;
            q = *(f + 3);
            q |= mask;
            if (tt < sto)
                *(f + 3) = q;
            tt += dx;
            q = *(f + 4);
            q |= mask;
            if (tt < sto)
                *(f + 4) = q;
            tt += dx;
            q = *(f + 5);
            q |= mask;
            if (tt < sto)
                *(f + 5) = q;
            tt += dx;
            q = *(f + 6);
            q |= mask;
            if (tt < sto)
                *(f + 6) = q;
            tt += dx;
            q = *(f + 7);
            q |= mask;
            if (tt < sto)
                *(f + 7) = q;
            tt += dx;
            q = *(f + 8);
            q |= mask;
            if (tt < sto)
                *(f + 8) = q;
            tt += dx;
            q = *(f + 9);
            q |= mask;
            if (tt < sto)
                *(f + 9) = q;
            tt += dx;
            q = *(f + 10);
            q |= mask;
            if (tt < sto)
                *(f + 10) = q;
            tt += dx;
            q = *(f + 11);
            q |= mask;
            if (tt < sto)
                *(f + 11) = q;
            tt += dx;
            q = *(f + 12);
            q |= mask;
            if (tt < sto)
                *(f + 12) = q;
            tt += dx;
            q = *(f + 13);
            q |= mask;
            if (tt < sto)
                *(f + 13) = q;
            tt += dx;
            q = *(f + 14);
            q |= mask;
            if (tt < sto)
                *(f + 14) = q;
            tt += dx;
            q = *(f + 15);
            q |= mask;
            if (tt < sto)
                *(f + 15) = q;
            tt += dx;
            q = *(f + 16);
            q |= mask;
            if (tt < sto)
                *(f + 16) = q;
            tt += dx;
            q = *(f + 17);
            q |= mask;
            if (tt < sto)
                *(f + 17) = q;
            tt += dz;
        }
    }
}

/**********************************************************************
 *  sound_control()  -
 **********************************************************************/
void sound_control(sint32 status, sint32 flag)

{
    char a[16], b[16], c[16], str[128];

    if (status == 0)
        system("killall spay &");
    else {
        strcpy(str, "nice spay ");

        switch (flag) {
        case SND_BACK:
            strcat(str, "0 1 ");
            break;
        case SND_BUTT:
            strcat(str, "1 0 ");
            break;
        case SND_AUON:
            strcat(str, "2 0 ");
            break;
        case SND_AUOF:
            strcat(str, "3 0 ");
            break;
        default:
            exit(0);
            break;
        }

        sprintf(c, "%d", Counter.shmid);
        strcat(str, c);
        strcat(str, "&");
        system(str);
    }
}

/**********************************************************************
 *  stats_menu_stuff()  -
 **********************************************************************/
static void stats_menu_stuff(t_boss *flaggs)

{
    sint32 i, j, v, max;
    t_menu plenu[24];
    char ch[128];
    t_body *tb, *tc, *suc;
    t_stars *st;

    if (flaggs->suncount == 2) {
        j = ((flaggs->suun_current == 0) ? 0xffff80ff : 0xff80ff80);
    } else
        j = 0xffffffff;

    suc = &flaggs->star[flaggs->suun_current];

    i = 0;
    make_new_item(&plenu[i], i, j, suc->ptr->name);

    for (i++; i < suc->ptr->moon_count + 1; i++) {
        tb = (t_body *)suc->next[i - 1];
        j = (tb->ptr->tess >> 4) & 0xf;
        v = ((j > 2 && j < 7) ? 0xff00ffff : 0xffffffff);
        make_new_item(&plenu[i], i, v, tb->ptr->name);
    }

    max = i;
    draw2_menu(i, plenu);
    v = check2_menu(suc->ptr->moon_count + 1, plenu);

    if (v == 0) {
        st = &ggaa[Counter.z][Counter.x].stars[Counter.star_current];

        i = 5;
        sprintf(ch, "Name:     %s", suc->ptr->name);
        make_new_item(&plenu[0], 0, 0xffffffff, ch);
        if (flaggs->suun_current == 0)
            sprintf(ch, "Abs Mag:  %.2f", st->abs_mag);
        else
            sprintf(ch, "Abs Mag:  %.2f", st->abs_mag2);
        make_new_item(&plenu[1], 1, 0xffffffff, ch);
        sprintf(ch, "Radius:   %.1f km", suc->ptr->rad);
        make_new_item(&plenu[2], 2, 0xffffffff, ch);
        sprintf(ch, "Mass:     %.2e kg", suc->ptr->mas);
        make_new_item(&plenu[3], 3, 0xffffffff, ch);
        sprintf(ch, "Temp:     %.0f deg.", suc->ptr->apo);
        make_new_item(&plenu[4], 4, 0xffffffff, ch);
    } else {
        tb = (t_body *)suc->next[v - 1];

        i = 11;
        sprintf(ch, "Name:     %s", tb->ptr->name);
        make_new_item(&plenu[0], 0, 0xffffffff, ch);
        sprintf(ch, "Radius:   %.0f km", tb->ptr->rad);
        make_new_item(&plenu[1], 1, 0xffffffff, ch);
        sprintf(ch, "Mass:     %.2e kg", tb->ptr->mas);
        make_new_item(&plenu[2], 2, 0xffffffff, ch);
        sprintf(ch, "Day:      %.2f days", tb->ptr->day);
        make_new_item(&plenu[3], 3, 0xffffffff, ch);
        sprintf(ch, "AxialTilt:%.2f deg.", tb->ptr->apo);
        make_new_item(&plenu[4], 4, 0xffffffff, ch);
        sprintf(ch, "Year:     %.2f days", tb->ptr->yer);
        make_new_item(&plenu[5], 5, 0xffffffff, ch);
        sprintf(ch, "Orbit:    %.2e km", tb->ptr->orb);
        make_new_item(&plenu[6], 6, 0xffffffff, ch);
        sprintf(ch, "Eccent.:  %.3f", tb->ptr->ecc);
        make_new_item(&plenu[7], 7, 0xffffffff, ch);
        sprintf(ch, "Inclin.:  %.1f deg.", tb->ptr->inc);
        make_new_item(&plenu[8], 8, 0xffffffff, ch);
        sprintf(ch, "Gravity:  %1.2f", GRAV * tb->ptr->mas / (EARTHG * tb->ptr->rad * tb->ptr->rad));
        make_new_item(&plenu[9], 9, 0xffffffff, ch);
        sprintf(ch, "Moons:    %d", tb->ptr->moon_count);
        make_new_item(&plenu[10], 10, 0xffffffff, ch);

        if (tb->ptr->r1 != 0.0) {
            sprintf(ch, "Inner Ring: %.0f km", tb->ptr->r1);
            make_new_item(&plenu[i], i, 0xffffffff, ch);
            sprintf(ch, "Outer Ring: %.0f km", tb->ptr->r2);
            make_new_item(&plenu[i + 1], i + 1, 0xffffffff, ch);
            i += 2;
        }
    }

    if (max < i)
        max = i;

    for (; i < max; i++)
        make_new_item(&plenu[i], i, 0x00002000, "");

    draw2_menu(i, plenu);
    sginap(30);
    i = check2_menu(i, plenu);

    if ((Counter.flags & USERM_FLAG) && (i < 10)) {
        switch (i) {
        case 1:
            tb->ptr->rad = spReadFloat();
            break;
        case 2:
            tb->ptr->mas = spReadFloat();
            break;
        case 3:
            tb->ptr->day = spReadFloat();
            break;
        case 4:
            tb->ptr->apo = spReadFloat();
            break;
        case 5:
            tb->ptr->yer = spReadFloat();
            break;
        case 6:
            tb->ptr->orb = spReadFloat();
            object_planet_orbit(tb, 0.0);
            break;
        case 7:
            tb->ptr->ecc = spReadFloat();
            object_planet_orbit(tb, 0.0);
            break;
        case 8:
            tb->ptr->inc = spReadFloat();
            object_planet_orbit(tb, 0.0);
            break;
        default:
            break;
        }

        return;
    }

    if (i == 10 && v > 0 && tb->ptr->moon_count > 0) {
        for (i = 0; i < tb->ptr->moon_count; i++) {
            tc = (t_body *)tb->next[i];
            sprintf(ch, "Name:     %s", tc->ptr->name);
            make_new_item(&plenu[i], i, 0xffffffff, ch);
        }

        if (max < i)
            max = i;

        for (; i < max; i++)
            make_new_item(&plenu[i], i, 0x00002000, "");

        draw2_menu(i, plenu);
        sginap(30);
        i = check2_menu(i, plenu);

        tc = (t_body *)tb->next[i];

        i = 10;
        sprintf(ch, "Name:     %s", tc->ptr->name);
        make_new_item(&plenu[0], 0, 0xffffffff, ch);
        sprintf(ch, "Radius:   %.0f km", tc->ptr->rad);
        make_new_item(&plenu[1], 1, 0xffffffff, ch);
        sprintf(ch, "Mass:     %.2e kg", tc->ptr->mas);
        make_new_item(&plenu[2], 2, 0xffffffff, ch);
        sprintf(ch, "Day:      %.2f days", tc->ptr->day);
        make_new_item(&plenu[3], 3, 0xffffffff, ch);
        sprintf(ch, "AxialTilt:%.2f deg.", tc->ptr->apo);
        make_new_item(&plenu[4], 4, 0xffffffff, ch);
        sprintf(ch, "Year:     %.2f days", tc->ptr->yer);
        make_new_item(&plenu[5], 5, 0xffffffff, ch);
        sprintf(ch, "Orbit:    %.2e km", tc->ptr->orb);
        make_new_item(&plenu[6], 6, 0xffffffff, ch);
        sprintf(ch, "Eccent.:  %.3f", tc->ptr->ecc);
        make_new_item(&plenu[7], 7, 0xffffffff, ch);
        sprintf(ch, "Inclin.:  %.1f deg.", tc->ptr->inc);
        make_new_item(&plenu[8], 8, 0xffffffff, ch);
        sprintf(ch, "Gravity:  %1.2f", GRAV * tc->ptr->mas / (EARTHG * tc->ptr->rad * tc->ptr->rad));
        make_new_item(&plenu[9], 9, 0xffffffff, ch);

        if (max < i)
            max = i;

        for (; i < max; i++)
            make_new_item(&plenu[i], i, 0x00002000, "");

        draw2_menu(i, plenu);
        sginap(30);

        i = check2_menu(i, plenu);

        if ((Counter.flags & USERM_FLAG) && (i < 10)) {
            switch (i) {
            case 1:
                tc->ptr->rad = spReadFloat();
                break;
            case 2:
                tc->ptr->mas = spReadFloat();
                break;
            case 3:
                tc->ptr->day = spReadFloat();
                break;
            case 4:
                tc->ptr->apo = spReadFloat();
                break;
            case 5:
                tc->ptr->yer = spReadFloat();
                break;
            case 6:
                tc->ptr->orb = spReadFloat();
                object_planet_orbit(tc, 0.0);
                break;
            case 7:
                tc->ptr->ecc = spReadFloat();
                object_planet_orbit(tc, 0.0);
                break;
            case 8:
                tc->ptr->inc = spReadFloat();
                object_planet_orbit(tc, 0.0);
                break;
            default:
                break;
            }

            return;
        }
    }
    
    /* Clear popup menu when exiting stats */
    clear_popup_menu();
}

/**********************************************************************
 *  autopilot_menu_stuff()  -
 **********************************************************************/
static sint32 autopilot_menu_stuff(t_boss *flaggs)

{
    sint32 i, j, v, aupi[3];
    t_body *tb;
    flot32 d, timespan;
    V3 u;
    t_menu plenu[24];
    t_galaga *ga;
    P3 pp;

    i = 0;
    if (Counter.status == STELL_STAT) {
        for (; i < flaggs->star[flaggs->suun_current].ptr->moon_count; i++) {
            tb = (t_body *)flaggs->star[flaggs->suun_current].next[i];
            j = (tb->ptr->tess >> 4) & 0xf;
            v = ((j > 2 && j < 7) ? 0xff00ffff : 0xffffffff);
            make_new_item(&plenu[i], i, v, tb->ptr->name);
        }

        make_new_item(&plenu[i], i, 0xffffffff, "New Star System");

        draw2_menu(i + 1, plenu);
        aupi[0] = check2_menu(flaggs->star[flaggs->suun_current].ptr->moon_count + 1, plenu);
    }

    if ((Counter.status == STELL_STAT && aupi[0] == flaggs->star[flaggs->suun_current].ptr->moon_count) ||
        Counter.status != STELL_STAT) {
        for (j = 0; j < i; j++)
            strcpy(plenu[j].mes0, "");

        make_new_item(&plenu[i], i, 0xffffffff, "Enter Star Number <CR>");
        draw2_menu(i + 1, plenu);

        if (!spReadStar(aupi))
            return (0);

        ga = &ggaa[aupi[2]][aupi[1]];

        if (aupi[0] >= ga->count)
            return (0);

        Counter.spline.p2.x = ga->stars[aupi[0]].x - 0.5 * GALAXY_EDGE + (aupi[1] + 0.5) * EDGESQ;
        Counter.spline.p2.y = ga->stars[aupi[0]].y;
        Counter.spline.p2.z = ga->stars[aupi[0]].z - 0.5 * GALAXY_EDGE + (aupi[2] + 0.5) * EDGESQ;

        if (Counter.status == STELL_STAT) {
            pp.x = ga->stars[Counter.star_current].x - 0.5 * GALAXY_EDGE + (Counter.x + 0.5) * EDGESQ;
            pp.y = ga->stars[Counter.star_current].y;
            pp.z = ga->stars[Counter.star_current].z - 0.5 * GALAXY_EDGE + (Counter.z + 0.5) * EDGESQ;

            pp.x = Counter.spline.p2.x - pp.x;
            pp.y = Counter.spline.p2.y - pp.y;
            pp.z = Counter.spline.p2.z - pp.z;
            d = fsqrt(pp.x * pp.x + pp.y * pp.y + pp.z * pp.z);

            Counter.spline.p2.x = 1.0 * PRTOKM * pp.x / d;
            Counter.spline.p2.y = 1.0 * PRTOKM * pp.y / d;
            Counter.spline.p2.z = 1.0 * PRTOKM * pp.z / d;

            Counter.spline.v1.x = Counter.spline.v2.x = 100000.0 * Counter.vnorm.x;
            Counter.spline.v1.y = Counter.spline.v2.y = 100000.0 * Counter.vnorm.y;
            Counter.spline.v1.z = Counter.spline.v2.z = 100000.0 * Counter.vnorm.z;
        } else {
            Counter.spline.v1.x = Counter.spline.v2.x = 2.0 * Counter.vnorm.x;
            Counter.spline.v1.y = Counter.spline.v2.y = 2.0 * Counter.vnorm.y;
            Counter.spline.v1.z = Counter.spline.v2.z = 2.0 * Counter.vnorm.z;
        }

        timespan = 60.0;
        Counter.spline.newstar[0] = aupi[0];
        Counter.spline.newstar[1] = aupi[1];
        Counter.spline.newstar[2] = aupi[2];
        Counter.flags |= AUTOP_FLAG;
    }

    else {
        if (aupi[0] < 0 || aupi[0] >= flaggs->star[flaggs->suun_current].ptr->moon_count)
            return (0);

        tb = (t_body *)flaggs->star[flaggs->suun_current].next[aupi[0]];

        u.x = tb->posit.x - flaggs->star[flaggs->suun_current].posit.x;
        u.y = tb->posit.y - flaggs->star[flaggs->suun_current].posit.y;
        u.z = tb->posit.z - flaggs->star[flaggs->suun_current].posit.z;
        d = fsqrt(u.x * u.x + u.y * u.y + u.z * u.z);
        u.x /= d;
        u.y /= d;
        u.z /= d;

        Counter.spline.p2.x = Counter.eye.x + tb->posit.x - 6.0 * tb->ptr->rad * u.x;
        Counter.spline.p2.y = Counter.eye.y + tb->posit.y - 6.0 * tb->ptr->rad * u.y;
        Counter.spline.p2.z = Counter.eye.z + tb->posit.z - 6.0 * tb->ptr->rad * u.z;

        Counter.spline.v2.x = 1.0e7 * u.x;
        Counter.spline.v2.y = 1.0e7 * u.y;
        Counter.spline.v2.z = 1.0e7 * u.z;

        Counter.spline.v1.x = 100000.0 * Counter.vnorm.x;
        Counter.spline.v1.y = 100000.0 * Counter.vnorm.y;
        Counter.spline.v1.z = 100000.0 * Counter.vnorm.z;

        timespan = 20.0;
        Counter.spline.newstar[0] = -2;
        Counter.spline.newstar[1] = -2;
        Counter.spline.newstar[2] = -2;
        Counter.flags |= AUTOP_FLAG;
    }

    Counter.spline.p1 = Counter.eye;
    Counter.spline.t1 = Counter.D;
    Counter.spline.t2 = Counter.D + timespan / (24.0 * 3600.0) + delta_timer();

    return (1);
}

#if STATS
/**********************************************************************
 *  statistics()  -
 **********************************************************************/
static sint32 statistics(long flag)

{
    sint32 i, ptr[16];

    if (flag) {
        for (i = 0; i < 256; i++)
            feed_buff[i] = 0;

        ptr[0] = (long)feed_buff;
        ptr[1] = 1024;
        glcompat(1009, (long)ptr);
    } else {
        glcompat(1012, 0);

        ptr[0] = (long)feed_buff;
        ptr[1] = 1024;
        glcompat(1010, (long)ptr);

        printf("0x%08x 0x%08x\n", feed_buff[0], feed_buff[1]);
        for (i = 2; i < 18; i += 4)
            printf("0x%08x 0x%08x 0x%08x 0x%08x\n", feed_buff[i], feed_buff[1 + i], feed_buff[2 + i], feed_buff[3 + i]);
    }
}
#endif
