/*
 * pfopts.c - the options and menus that powerflip adds to flip
 *
 * From parse_args, init_scene, init_fog, set_fogcolor, the fog menus,
 * init_clipplanes, cp1 / cp2, draw_background and draw_title_message of the
 * IRIX binary:
 *
 *  -w / -W   wireframe models, wireframe while moving ("Move Wireframe")
 *  -L        lines model: no lighting
 *  -b file   a YAODL background, drawn in [-1, 1] without lighting nor z
 *  -c r,g,b  the background color
 *  -F / -f   fog of the given density / color ("Fog" menu: clear air, fog,
 *            haze, black fog; density, near and far distances, modes)
 *  -C        clipping plane ("Clipping Planes" menu): the opaque objects
 *            keep one side of a plane that can be moved with the mouse,
 *            the transparent ones the other side
 *  -B n      motion blur, with an accumulation buffer ("Blur and Sharpen")
 *  -m m,b    material and back material of the models
 *  -p / -o   position and orientation (quaternion) of the models
 *  -T text   a title message shown while the models are loaded
 *  -R        root window: full screen here
 *  -r        continuous redraw
 *  -S -M -D -z  stereo, no multisampling, debug and an undocumented option:
 *            accepted, without effect here
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pfopts.h"

void remake_menus(void);

int pf_wireframe;
int pf_move_wireframe;
int pf_nolighting;
int pf_redraw;
int pf_material = -1;
int pf_backmaterial = -1;
int pf_have_position, pf_have_orientation;
float pf_position[3];
float pf_orientation[4] = { 0, 0, 0, 1 };
int pf_fullscreen;
flipobj pf_clipobj;

static int clip_on;                    /* -C, "Clip" */
static int blur_amount;                 /* -B: percent of the previous frames kept */
static float bg_color[3];               /* -c */
static char *bg_file;                   /* -b */
static YModel *bg_model;

static const char *titles[16] = { "Please wait, loading data files..." };
static int ntitles = 1;

/* --- fog --------------------------------------------------------------------------- */

enum { CLEAR_AIR = 1, FOG, HAZE, BLACK_FOG };

static int fog_type = CLEAR_AIR;
static int fog_mode = 1;                /* index in fog_modes */
static float fog_density = 0.2f, fog_near = 0.02f, fog_far = 5.0f;
static float fog_color[3] = { 0.5f, 0.5f, 0.5f }; /* -f, 0x808080 */
static float fog_rgb[3];                /* color of the current fog */

static const char *fog_mode_names[] = { "vertex_exp", "pixel_exp", "vertex_exp2", "pixel_exp2", "vertex_lin",
                                        "pixel_lin" };
static const int fog_modes[] = { FG_VTX_EXP, FG_PIX_EXP, FG_VTX_EXP2, FG_PIX_EXP2, FG_VTX_LIN, FG_PIX_LIN };
static const float fog_densities[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.5f, 2.0f };
#define NUM_DENSITIES (int)(sizeof(fog_densities) / sizeof(fog_densities[0]))

/* set_fogcolor: the background takes the color of the fog */
static void apply_fog(void)
{
    float params[5];
    if (fog_type == CLEAR_AIR) {
        fogvertex(FG_OFF, NULL);
        return;
    }
    if (fog_type == FOG)
        memcpy(fog_rgb, fog_color, sizeof(fog_rgb));
    else if (fog_type == HAZE)
        fog_rgb[0] = 0.25f, fog_rgb[1] = 0.333333f, fog_rgb[2] = 0.5f;
    else
        fog_rgb[0] = fog_rgb[1] = fog_rgb[2] = 0;
    if (fog_mode < 5) {
        params[0] = fog_density;
        memcpy(params + 1, fog_rgb, sizeof(fog_rgb));
    } else {
        params[0] = fog_near;
        params[1] = fog_far;
        memcpy(params + 2, fog_rgb, sizeof(fog_rgb));
    }
    fogvertex(fog_modes[fog_mode - 1], params);
    fogvertex(FG_ON, params);
}

unsigned long pf_clear_color(void)
{
    const float *c = fog_type == CLEAR_AIR ? bg_color : fog_rgb;
    return 0xff000000UL | ((unsigned long)(c[2] * 255) << 16) | ((unsigned long)(c[1] * 255) << 8)
           | (unsigned long)(c[0] * 255);
}

/* --- options ------------------------------------------------------------------------ */

int pf_option(int c, const char *arg)
{
    switch (c) {
    case 'w':
        pf_wireframe = 1;
        YAODL_DrawStyle = 3;
        break;
    case 'W':
        pf_move_wireframe = 1;
        break;
    case 'L':
        pf_nolighting = 1;
        YAODL_NoLighting = 1;
        break;
    case 'r':
        pf_redraw = 1;
        break;
    case 'b':
        bg_file = strdup(arg);
        break;
    case 'c':
        if (sscanf(arg, "%f,%f,%f", &bg_color[0], &bg_color[1], &bg_color[2]) != 3)
            return -1;
        break;
    case 'F':
        fog_type = FOG;
        fog_density = (float)atof(arg);
        break;
    case 'f':
        fog_type = FOG;
        sscanf(arg, "%f,%f,%f", &fog_color[0], &fog_color[1], &fog_color[2]);
        break;
    case 'C':
        clip_on = 1;
        break;
    case 'B':
        blur_amount = atoi(arg);
        break;
    case 'm':
        if (sscanf(arg, "%d,%d", &pf_material, &pf_backmaterial) < 1)
            return -1;
        break;
    case 'p':
        if (sscanf(arg, "%f,%f,%f", &pf_position[0], &pf_position[1], &pf_position[2]) != 3)
            return -1;
        pf_have_position = 1;
        break;
    case 'o':
        if (sscanf(arg, "%f,%f,%f,%f", &pf_orientation[0], &pf_orientation[1], &pf_orientation[2],
                   &pf_orientation[3]) != 4)
            return -1;
        pf_have_orientation = 1;
        break;
    case 'T':
        if (ntitles < 16)
            titles[ntitles++] = arg;
        break;
    case 'R':
        pf_fullscreen = 1;
        break;
    case 'S': /* stereo */
    case 'M': /* no multisampling */
    case 'D': /* debug */
    case 'z':
        break;
    default:
        return 0;
    }
    return 1;
}

/* --- motion blur ---------------------------------------------------------------------- */

static int blur_supported;
static int blur_clear;

void pf_before_window(void)
{
    acsize(16);
}

void pf_blur(void)
{
    float b = blur_amount / 100.0f;
    if (!blur_supported || blur_amount <= 0)
        return;
    if (blur_clear) {
        acbuf(AC_CLEAR_ACCUMULATE, 1.0f);
        blur_clear = 0;
    } else {
        acbuf(AC_MULT, b);
        acbuf(AC_ACCUMULATE, 1.0f - b);
    }
    acbuf(AC_RETURN, 1.0f);
}

int pf_animating(void)
{
    return pf_redraw || (blur_supported && blur_amount > 0);
}

/* --- title messages and background -------------------------------------------------- */

void pf_title_message(void)
{
    static Matrix idm = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    int i;
    mmode(MPROJECTION);
    pushmatrix();
    ortho2(0.0f, 1.0f, 0.0f, 1.0f);
    mmode(MVIEWING);
    pushmatrix();
    loadmatrix(idm);
    czclear(pf_clear_color(), getgdesc(GD_ZMIN));
    cpack(0xffffffff);
    for (i = 0; i < ntitles; i++) {
        cmov2(0.1f, 0.8f - 0.1f * i);
        charstr((char *)titles[i]);
    }
    popmatrix();
    mmode(MPROJECTION);
    popmatrix();
    mmode(MVIEWING);
    swapbuffers();
}

/* draw_background: the YAODL file in [-1, 1], without z nor lighting */
void pf_draw_background(void)
{
    static Matrix idm = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    int style = YAODL_DrawStyle;
    if (!bg_model)
        return;
    mmode(MPROJECTION);
    pushmatrix();
    ortho2(-1.0f, 1.0f, -1.0f, 1.0f);
    mmode(MVIEWING);
    pushmatrix();
    loadmatrix(idm);
    zbuffer(FALSE);
    lmbind(LMODEL, 0);
    lmcolor(LMC_COLOR);
    YAODL_DrawStyle = 0;
    yaodl_draw(bg_model);
    YAODL_DrawStyle = style;
    resetmodel();
    zbuffer(TRUE);
    popmatrix();
    mmode(MPROJECTION);
    popmatrix();
    mmode(MVIEWING);
}

/* --- clipping planes ----------------------------------------------------------------- */

static int clip_supported;
static float clip_planes[2][4] = { { -1, 0, 0, 0 }, { 1, 0, 0, 0 } };

/* cp1 / cp2: the plane in the frame of the clip object */
void pf_clip_begin(int pass)
{
    Matrix m;
    if (!clip_supported || !clip_on)
        return;
    pushmatrix();
    translate(pf_clipobj.trans[0], pf_clipobj.trans[1], pf_clipobj.trans[2]);
    build_rotmatrix(m, pf_clipobj.er);
    multmatrix(m);
    clipplane(pass ? 0 : 3, CP_OFF, NULL);
    clipplane(pass ? 3 : 0, CP_DEFINE, clip_planes[pass ? 1 : 0]);
    clipplane(pass ? 3 : 0, CP_ON, NULL);
    popmatrix();
}

void pf_clip_end(void)
{
    if (!clip_supported)
        return;
    clipplane(0, CP_OFF, NULL);
    clipplane(3, CP_OFF, NULL);
}

/* --- init ------------------------------------------------------------------------------- */

void pf_init_scene(void)
{
    clip_supported = getgdesc(GD_CLIPPLANES) >= 6;
    blur_supported = getgdesc(GD_BITS_ACBUF) > 0;
    blur_clear = 1;
    /* the clip object turns like the others, it is not displayed */
    pf_clipobj.select = FALSE;
    pf_clipobj.er[3] = 1.0f;
    pf_clipobj.espin[3] = 1.0f;
    apply_fog();
    if (bg_file) {
        bg_model = yaodl_read(bg_file);
        if (!bg_model)
            fprintf(stderr, "Warning: couldn't read background %s: continuing\n", bg_file);
    }
}

int pf_draw_style(int type)
{
    if (pf_wireframe || (pf_move_wireframe && !ui_quiet))
        return 3;
    return type == POLYGONS ? 0 : 3;
}

/* --- menus ------------------------------------------------------------------------------ */

static long fogmenu, densitymenu, nearmenu, farmenu, modemenu, blurmenu, clipmenu;

static void check(long menu, int item, int on)
{
    setpup(menu, item, on ? PUP_CHECK : PUP_BOX);
}

static void remake_fogmenus(void);

/* the titles of the menus are items of value 0 in porting: ignored */
static void select_fogtype(int n)
{
    if (n < CLEAR_AIR || n > BLACK_FOG)
        return;
    fog_type = n;
    apply_fog();
    remake_fogmenus();
    remake_menus();
}

static void select_fogdensity(int n)
{
    if (n < 1 || n > NUM_DENSITIES)
        return;
    fog_density = fog_densities[n - 1];
    apply_fog();
    remake_fogmenus();
    remake_menus();
}

static void select_fognear(int n)
{
    if (n != 1 && n != 2)
        return;
    fog_near += n == 1 ? 0.5f : -0.5f;
    if (fog_near < 0)
        fog_near = 0;
    apply_fog();
    remake_fogmenus();
    remake_menus();
}

static void select_fogfar(int n)
{
    if (n != 1 && n != 2)
        return;
    fog_far += n == 1 ? 0.5f : -0.5f;
    if (fog_far < fog_near + 0.5f)
        fog_far = fog_near + 0.5f;
    apply_fog();
    remake_fogmenus();
    remake_menus();
}

static void select_fogmode(int n)
{
    if (n < 1 || n > 6)
        return;
    fog_mode = n;
    apply_fog();
    remake_fogmenus();
    remake_menus();
}

static void remake_fogmenus(void)
{
    char temp[64];
    int i;

    freepup(densitymenu);
    densitymenu = newpup();
    addtopup(densitymenu, "Density %t %F", select_fogdensity);
    for (i = 0; i < NUM_DENSITIES; i++) {
        sprintf(temp, "%.1f %%x%d", fog_densities[i], i + 1);
        addtopup(densitymenu, temp);
        check(densitymenu, i + 1, fog_densities[i] == fog_density);
    }

    freepup(nearmenu);
    nearmenu = newpup();
    sprintf(temp, "Near Distance (%.1f) %%t %%F", fog_near);
    addtopup(nearmenu, temp, select_fognear);
    addtopup(nearmenu, "+ 0.5 %x1");
    addtopup(nearmenu, "- 0.5 %x2");

    freepup(farmenu);
    farmenu = newpup();
    sprintf(temp, "Far Distance (%.1f) %%t %%F", fog_far);
    addtopup(farmenu, temp, select_fogfar);
    addtopup(farmenu, "+ 0.5 %x1");
    addtopup(farmenu, "- 0.5 %x2");

    freepup(modemenu);
    modemenu = newpup();
    addtopup(modemenu, "Fog Mode %t %F", select_fogmode);
    for (i = 0; i < 6; i++) {
        sprintf(temp, "%s %%x%d", fog_mode_names[i], i + 1);
        addtopup(modemenu, temp);
        check(modemenu, i + 1, fog_mode == i + 1);
    }

    freepup(fogmenu);
    fogmenu = newpup();
    addtopup(fogmenu, "Fog Options %t %F", select_fogtype);
    addtopup(fogmenu, "Clear Air %x1");
    addtopup(fogmenu, "Fog %x2");
    addtopup(fogmenu, "Haze %x3");
    addtopup(fogmenu, "Black Fog (depthcue) %x4");
    for (i = 1; i <= 4; i++)
        check(fogmenu, i, fog_type == i);
    addtopup(fogmenu, "Density %m", densitymenu);
    addtopup(fogmenu, "Near Distance %m", nearmenu);
    addtopup(fogmenu, "Far Distance %m", farmenu);
    addtopup(fogmenu, "Modes %m", modemenu);
}

static const int blur_amounts[] = { 0, 25, 50, 75, 90 };

static void remake_blurmenu(void);

static void select_blur(int n)
{
    if (n < 1 || n > 5)
        return;
    blur_amount = blur_amounts[n - 1];
    blur_clear = 1;
    remake_blurmenu();
    remake_menus();
}

static void remake_blurmenu(void)
{
    char temp[64];
    int i;
    freepup(blurmenu);
    blurmenu = newpup();
    addtopup(blurmenu, "Motion Blur %t %F", select_blur);
    for (i = 0; i < 5; i++) {
        sprintf(temp, "%d %%%% %%x%d", blur_amounts[i], i + 1);
        addtopup(blurmenu, temp);
        check(blurmenu, i + 1, blur_amount == blur_amounts[i]);
    }
}

static void remake_clipmenu(void);

static void toggle_clipplanes(void)
{
    clip_on = !clip_on;
    remake_clipmenu();
    remake_menus();
}

/* the plane moves with the mouse instead of the selected objects */
static void toggle_clipselect(void)
{
    pf_clipobj.select = !pf_clipobj.select;
    remake_clipmenu();
    remake_menus();
}

static void remake_clipmenu(void)
{
    freepup(clipmenu);
    clipmenu = newpup();
    /* no title: in porting it would call the last function of the menu */
    addtopup(clipmenu, clip_on ? "Clip off %f" : "Clip on %f", toggle_clipplanes);
    addtopup(clipmenu, pf_clipobj.select ? "Move the objects with Mouse %f" : "Move with Mouse %f",
             toggle_clipselect);
}

static void toggle_move_wireframe(void)
{
    pf_move_wireframe = !pf_move_wireframe;
    remake_menus();
}

void pf_init_menus(void)
{
    densitymenu = newpup();
    nearmenu = newpup();
    farmenu = newpup();
    modemenu = newpup();
    fogmenu = newpup();
    remake_fogmenus();
    blurmenu = newpup();
    remake_blurmenu();
    clipmenu = newpup();
    remake_clipmenu();
}

/* the entries of powerflip in the main menu */
void pf_add_menus(long menu)
{
    if (!pf_nolighting && !pf_wireframe)
        addtopup(menu, "Fog %m", fogmenu);
    if (blur_supported)
        addtopup(menu, "Blur and Sharpen %m", blurmenu);
    if (clip_supported)
        addtopup(menu, "Clipping Planes %m", clipmenu);
    if (!pf_wireframe)
        addtopup(menu, pf_move_wireframe ? "Move Filled %f" : "Move Wireframe %f", toggle_move_wireframe);
}
