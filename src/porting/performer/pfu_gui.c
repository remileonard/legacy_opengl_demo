/*
 * pfu_gui.c - the GUI panel of libpfutil (IRIS Performer 2.0): buttons,
 * switches, menu buttons, radio buttons, sliders and messages, drawn in
 * OpenGL in their viewport of the window and driven by the pfuMouse.
 *
 * The widgets are placed in the units of the panel; at drawing time the
 * box of the widgets is fitted in the GUI viewport, so that the panel
 * follows the size of the window.
 */
/* freeglut first: windows.h before the GL headers of Performer/pf.h */
#include <GL/freeglut.h>

#include "pf_internal.h"
#include <math.h>

#define MAX_SEL 32

struct pfuWidget {
    int type, flags, id;
    pfuPanel *panel;
    int x, y, w, h;
    char label[PF_MAXSTRING];
    pfuWidgetFuncType action;
    float value, defval, min, max;
    int nsel, sel;
    char *strs[MAX_SEL];
    int vals[MAX_SEL];
    int on, defon;
    int enabled;
    int pressed;
};

struct pfuPanel {
    int enabled;
    float xo, yo, xs, ys;
};

static pfPipeWindow *gui_pw;
static int gui_on;
static float gl_, gr_ = 0.25f, gb_, gt_ = 1;
static pfuWidget *widgets[PFUGUI_MAXWIDGETS];
static int nwidgets;
static pfuPanel *panels[16];
static int npanels;

/* the fit of the panel units in the viewport */
static float fit_s = 1, fit_minx, fit_maxy;
static int vp_x, vp_y, vp_w, vp_h;

/* the interaction */
static pfuWidget *active, *menu;
static int menu_x, menu_y, menu_w, menu_item_h, menu_dragged;
static int prev_buttons;

static void gui_draw(pfPipeWindow *pw);

void pfuInitGUI(pfPipeWindow *pw)
{
    gui_pw = pw;
    pfiSetWinHook(gui_draw);
}

void pfuExitGUI(void) { gui_on = 0; }
void pfuEnableGUI(int on) { gui_on = on; if (!on) { menu = NULL; active = NULL; } }

void pfuGUIViewport(float l, float r, float b, float t)
{
    gl_ = l; gr_ = r; gb_ = b; gt_ = t;
}

void pfuGetGUIViewport(float *l, float *r, float *b, float *t)
{
    *l = gl_; *r = gr_; *b = gb_; *t = gt_;
}

static void gui_rect(int *x, int *y, int *w, int *h)
{
    int ws = gui_pw ? gui_pw->xs : 640, hs = gui_pw ? gui_pw->ys : 480;
    *x = (int)(gl_ * ws + 0.5f);
    *y = (int)(gb_ * hs + 0.5f);
    *w = (int)(gr_ * ws + 0.5f) - *x;
    *h = (int)(gt_ * hs + 0.5f) - *y;
}

int pfuInGUI(int x, int y)
{
    int gx, gy, gw, gh;
    if (!gui_on)
        return 0;
    if (menu)
        return 1;
    gui_rect(&gx, &gy, &gw, &gh);
    return x >= gx && x < gx + gw && y >= gy && y < gy + gh;
}

void pfuRedrawGUI(void) {}
pfChannel *pfuGetGUIChan(void) { return NULL; }
pfHighlight *pfuGetGUIHlight(void) { return NULL; }

pfuPanel *pfuNewPanel(void)
{
    pfuPanel *p = (pfuPanel *)calloc(1, sizeof(pfuPanel));
    int x, y, w, h;
    gui_rect(&x, &y, &w, &h);
    p->xo = PFUGUI_PANEL_MARGIN;
    p->yo = 0;
    p->xs = (float)w;
    p->ys = (float)(h > 200 ? h : 1000);
    if (npanels < 16)
        panels[npanels++] = p;
    return p;
}

void pfuEnablePanel(pfuPanel *p) { p->enabled = 1; }
void pfuDisablePanel(pfuPanel *p) { p->enabled = 0; }

void pfuGetPanelOriginSize(pfuPanel *p, float *xo, float *yo, float *xs, float *ys)
{
    *xo = p->xo; *yo = p->yo; *xs = p->xs; *ys = p->ys;
}

pfuWidget *pfuNewWidget(pfuPanel *p, int type, int id)
{
    pfuWidget *w;
    if (nwidgets == PFUGUI_MAXWIDGETS)
        return NULL;
    w = (pfuWidget *)calloc(1, sizeof(pfuWidget));
    w->type = type & PFUGUI_TYPE_MASK;
    w->flags = type & ~PFUGUI_TYPE_MASK;
    w->id = id;
    w->panel = p;
    w->enabled = 1;
    w->max = 1;
    w->w = PFUGUI_BUTTON_VLONG_XSIZE;
    w->h = PFUGUI_BUTTON_YSIZE;
    widgets[nwidgets++] = w;
    return w;
}

int pfuGetWidgetType(pfuWidget *w) { return w->type | w->flags; }
int pfuGetWidgetId(pfuWidget *w) { return w->id; }

void pfuWidgetDim(pfuWidget *w, int xo, int yo, int xs, int ys)
{
    w->x = xo; w->y = yo; w->w = xs; w->h = ys;
}

void pfuGetWidgetDim(pfuWidget *w, int *xo, int *yo, int *xs, int *ys)
{
    *xo = w->x; *yo = w->y; *xs = w->w; *ys = w->h;
}

void pfuWidgetLabel(pfuWidget *w, const char *label)
{
    if (w)
        strncpy(w->label, label ? label : "", sizeof(w->label) - 1);
}

const char *pfuGetWidgetLabel(pfuWidget *w) { return w->label; }

static int find_sel(pfuWidget *w, float val)
{
    int i;
    for (i = 0; i < w->nsel; i++)
        if ((float)w->vals[i] == val)
            return i;
    return -1;
}

void pfuWidgetRange(pfuWidget *w, int mode, float min, float max, float val)
{
    (void)mode;
    w->min = min;
    w->max = max;
    w->value = w->defval = val;
}

void pfuWidgetValue(pfuWidget *w, float val)
{
    int s;
    if (!w)
        return;
    w->value = val;
    if (w->type == PFUGUI_SWITCH)
        w->on = val != 0;
    if ((s = find_sel(w, val)) >= 0)
        w->sel = s;
}

float pfuGetWidgetValue(pfuWidget *w) { return w ? w->value : 0; }

void pfuWidgetDefaultValue(pfuWidget *w, float val)
{
    w->defval = val;
    pfuWidgetValue(w, val);
}

void pfuWidgetSelections(pfuWidget *w, pfuGUIString *strs, int *vals,
                         void (**funcs)(pfuWidget *), int n)
{
    int i;
    (void)funcs;
    /* the Motion menu of perfly: the vehicles of the paths are added */
    if (n == 3 && strs[0] && strs[2] && strcmp(strs[0], "Trackball") == 0 && strcmp(strs[2], "Fly") == 0)
        n = pfuiVehicleMenu(w, &strs, &vals, n);
    w->nsel = n > MAX_SEL ? MAX_SEL : n;
    for (i = 0; i < w->nsel; i++) {
        w->strs[i] = strs[i];
        w->vals[i] = vals ? vals[i] : i;
    }
    w->sel = 0;
    if (w->nsel)
        w->value = w->defval = (float)w->vals[0];
}

void pfuWidgetSelection(pfuWidget *w, int index)
{
    if (index >= 0 && index < w->nsel) {
        w->sel = index;
        w->value = (float)w->vals[index];
    }
}

int pfuGetWidgetSelection(pfuWidget *w) { return w->sel; }

static void no_action(pfuWidget *w) { (void)w; }

void pfuWidgetActionFunc(pfuWidget *w, pfuWidgetFuncType func) { w->action = func; }
pfuWidgetFuncType pfuGetWidgetActionFunc(pfuWidget *w) { return w && w->action ? w->action : no_action; }
void pfuWidgetOnOff(pfuWidget *w, int on) { if (w) w->on = on != 0; }
int pfuIsWidgetOn(pfuWidget *w) { return w ? w->on : 0; }
void pfuEnableWidget(pfuWidget *w) { if (w) w->enabled = 1; }
void pfuDisableWidget(pfuWidget *w) { if (w) w->enabled = 0; }
int pfuGetWidgetEnabled(pfuWidget *w) { return w ? w->enabled : 0; }

/* the widgets come back to their default values, their actions are called */
void pfuResetGUI(void)
{
    int i;
    for (i = 0; i < nwidgets; i++) {
        pfuWidget *w = widgets[i];
        if (w->type == PFUGUI_BUTTON || w->type == PFUGUI_MESSAGE)
            continue;
        w->on = w->defon;
        pfuWidgetValue(w, w->defval);
        if (w->type == PFUGUI_SWITCH)
            w->on = w->value != 0;
        if (w->action)
            w->action(w);
    }
}

/* --- drawing ------------------------------------------------------------------ */

static float px_x(float x) { return vp_x + (x - fit_minx) * fit_s; }
static float px_y(float y) { return vp_y + vp_h - (fit_maxy - y) * fit_s; }

static void compute_fit(void)
{
    float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f, bw, bh;
    int i;
    gui_rect(&vp_x, &vp_y, &vp_w, &vp_h);
    for (i = 0; i < nwidgets; i++) {
        pfuWidget *w = widgets[i];
        if (!w->panel->enabled)
            continue;
        if (w->x < minx) minx = (float)w->x;
        if (w->y < miny) miny = (float)w->y;
        if (w->x + w->w > maxx) maxx = (float)(w->x + w->w);
        if (w->y + w->h > maxy) maxy = (float)(w->y + w->h);
    }
    if (maxx < minx) {
        fit_s = 1;
        fit_minx = 0;
        fit_maxy = (float)vp_h;
        return;
    }
    minx -= PFUGUI_PANEL_MARGIN; miny -= PFUGUI_PANEL_MARGIN;
    maxx += PFUGUI_PANEL_MARGIN; maxy += PFUGUI_PANEL_MARGIN;
    bw = maxx - minx;
    bh = maxy - miny;
    fit_s = PF_MIN2(vp_w / bw, vp_h / bh);
    if (fit_s > 2)
        fit_s = 2;
    fit_minx = minx;
    fit_maxy = maxy;
}

static void bevel(float x0, float y0, float x1, float y1, int in)
{
    float hi = in ? 0.35f : 0.95f, lo = in ? 0.95f : 0.35f;
    glBegin(GL_LINES);
    glColor3f(hi, hi, hi);
    glVertex2f(x0, y0); glVertex2f(x0, y1);
    glVertex2f(x0, y1); glVertex2f(x1, y1);
    glColor3f(lo, lo, lo);
    glVertex2f(x1, y1); glVertex2f(x1, y0);
    glVertex2f(x1, y0); glVertex2f(x0, y0);
    glEnd();
}

static void box(float x0, float y0, float x1, float y1, float r, float g, float b)
{
    glColor3f(r, g, b);
    glRectf(x0, y0, x1, y1);
}

/* text clipped to a width, x the left side or the center */
static void text(float x, float y, const char *s, float maxw, int center, int big)
{
    char buf[PF_MAXSTRING];
    int w;
    strncpy(buf, s, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    pfuiTextWidth(buf, big, &w);
    while (w > maxw && strlen(buf) > 1) {
        buf[strlen(buf) - 1] = '\0';
        pfuiTextWidth(buf, big, &w);
    }
    pfuiDrawText(center ? x - w / 2.0f : x, y, buf, big);
}

static void light(float cx, float cy, float r, int on, int enabled)
{
    if (on)
        glColor3f(enabled ? 1.0f : 0.6f, enabled ? 0.9f : 0.6f, 0.1f);
    else
        glColor3f(0.35f, 0.35f, 0.38f);
    glRectf(cx - r, cy - r, cx + r, cy + r);
    bevel(cx - r, cy - r, cx + r, cy + r, 1);
}

static void slider_pos(pfuWidget *w, float *t)
{
    float v = w->value, lo = w->min, hi = w->max;
    if (w->type == PFUGUI_SLIDER_LOG) {
        if (lo <= 0) lo = hi > 0 ? hi * 1e-4f : 1e-4f;
        if (v < lo) v = lo;
        *t = hi > lo ? (float)(log(v / lo) / log(hi / lo)) : 0;
    } else {
        *t = hi > lo ? (v - lo) / (hi - lo) : 0;
    }
    *t = *t < 0 ? 0 : *t > 1 ? 1 : *t;
}

static float slider_value(pfuWidget *w, float t)
{
    float lo = w->min, hi = w->max;
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    if (w->type == PFUGUI_SLIDER_LOG) {
        if (lo <= 0) lo = hi > 0 ? hi * 1e-4f : 1e-4f;
        if (t <= 0)
            return w->min;
        return (float)(lo * pow(hi / lo, t));
    }
    return lo + t * (hi - lo);
}

static void fmt_value(char *buf, float v)
{
    float a = (float)fabs(v);
    if (a >= 10000) sprintf(buf, "%.0f", v);
    else if (a >= 100) sprintf(buf, "%.1f", v);
    else if (a >= 1) sprintf(buf, "%.2f", v);
    else sprintf(buf, "%.3f", v);
}

static void draw_widget(pfuWidget *w)
{
    float x0 = px_x((float)w->x), x1 = px_x((float)(w->x + w->w));
    float y0 = px_y((float)w->y), y1 = px_y((float)(w->y + w->h));
    float tc = w->enabled ? 0.0f : 0.45f, ty;
    int big = fit_s >= 1.2f;
    char buf[PF_MAXSTRING + 64];

    ty = (y0 + y1) / 2 - (big ? 4 : 3);
    switch (w->type) {
    case PFUGUI_MESSAGE:
        box(x0, y0, x1, y1, 0.12f, 0.12f, 0.16f);
        bevel(x0, y0, x1, y1, 1);
        glColor3f(1, 1, 0.5f);
        text(x0 + 3, ty, w->label, x1 - x0 - 6, 0, big);
        break;
    case PFUGUI_BUTTON:
    case PFUGUI_SWITCH:
    case PFUGUI_MENU_BUTTON:
        if (w->pressed || (w == menu))
            box(x0, y0, x1, y1, 0.62f, 0.62f, 0.68f);
        else
            box(x0, y0, x1, y1, 0.78f, 0.78f, 0.82f);
        bevel(x0, y0, x1, y1, w->pressed || w == menu);
        glColor3f(tc, tc, tc + (w->enabled ? 0.1f : 0));
        if (w->type == PFUGUI_BUTTON) {
            text((x0 + x1) / 2, ty, w->label, x1 - x0 - 4, 1, big);
        } else if (w->type == PFUGUI_SWITCH) {
            float r = (y1 - y0) * 0.22f;
            text(x0 + 4, ty, w->label, x1 - x0 - 8 - 3 * r, 0, big);
            light(x1 - 2 * r - 3, (y0 + y1) / 2, r, w->value != 0, w->enabled);
        } else {
            float a = (y1 - y0) * 0.2f, cx = x1 - a - 4, cy = (y0 + y1) / 2;
            sprintf(buf, "%s: %s", w->label, w->nsel && w->sel < w->nsel ? w->strs[w->sel] : "");
            text(x0 + 4, ty, buf, x1 - x0 - 10 - 2 * a, 0, big);
            glBegin(GL_TRIANGLES);
            glVertex2f(cx - a, cy + a / 2);
            glVertex2f(cx + a, cy + a / 2);
            glVertex2f(cx, cy - a / 2);
            glEnd();
        }
        break;
    case PFUGUI_SLIDER:
    case PFUGUI_SLIDER_LOG: {
        float t, my = (y0 + y1) / 2, bx0 = x0 + 4, bx1 = x1 - 4, kx;
        char val[32];
        box(x0, y0, x1, y1, 0.72f, 0.72f, 0.76f);
        bevel(x0, y0, x1, y1, 0);
        fmt_value(val, w->value);
        sprintf(buf, "%s: %s", w->label, val);
        glColor3f(tc, tc, tc);
        text(x0 + 4, my + (y1 - my) / 2 - (big ? 4 : 3), buf, x1 - x0 - 8, 0, big);
        slider_pos(w, &t);
        box(bx0, y0 + 4, bx1, my - 2, 0.45f, 0.45f, 0.5f);
        bevel(bx0, y0 + 4, bx1, my - 2, 1);
        if (w->enabled)
            box(bx0 + 1, y0 + 5, bx0 + 1 + t * (bx1 - bx0 - 2), my - 3, 0.35f, 0.45f, 0.85f);
        kx = bx0 + t * (bx1 - bx0);
        box(kx - 3, y0 + 2, kx + 3, my, 0.85f, 0.85f, 0.88f);
        bevel(kx - 3, y0 + 2, kx + 3, my, 0);
        break;
    }
    case PFUGUI_RADIO_BUTTON:
    case PFUGUI_RADIO_BUTTON_TOGGLE: {
        int n = w->nsel, i, toggle = w->type == PFUGUI_RADIO_BUTTON_TOGGLE;
        float lw = toggle ? (x1 - x0) * 0.26f : 0, cw = n ? (x1 - x0 - lw) / n : 0;
        float r = (y1 - y0) * 0.18f;
        box(x0, y0, x1, y1, 0.78f, 0.78f, 0.82f);
        bevel(x0, y0, x1, y1, 0);
        if (toggle) {
            glColor3f(tc, tc, tc);
            text(x0 + 4, ty, w->label, lw - 8 - 2 * r, 0, big);
            light(x0 + lw - r - 4, (y0 + y1) / 2, r, w->on, w->enabled);
            glColor3f(0.5f, 0.5f, 0.5f);
            glBegin(GL_LINES);
            glVertex2f(x0 + lw, y0 + 2);
            glVertex2f(x0 + lw, y1 - 2);
            glEnd();
        }
        for (i = 0; i < n; i++) {
            float cx0 = x0 + lw + i * cw;
            int sel = i == w->sel && (!toggle || w->on);
            if (sel)
                box(cx0 + 2, y0 + 2, cx0 + cw - 2, y1 - 2, 0.62f, 0.62f, 0.7f);
            light(cx0 + r + 4, (y0 + y1) / 2, r * 0.8f, i == w->sel, w->enabled && (!toggle || w->on));
            glColor3f(tc, tc, tc);
            text(cx0 + 2 * r + 7, ty, w->strs[i], cw - 2 * r - 9, 0, big);
        }
        break;
    }
    }
}

static void draw_menu(void)
{
    int i, big = fit_s >= 1.2f;
    float x0 = (float)menu_x, x1 = (float)(menu_x + menu_w);
    for (i = 0; i < menu->nsel; i++) {
        float y1 = (float)(menu_y - i * menu_item_h), y0 = y1 - menu_item_h;
        int hot = 0;
        pfuMouse m;
        pfuiPeekMouse(&m);
        hot = m.xpos >= x0 && m.xpos < x1 && m.ypos >= y0 && m.ypos < y1;
        box(x0, y0, x1, y1, hot ? 0.35f : 0.85f, hot ? 0.45f : 0.85f, hot ? 0.85f : 0.88f);
        bevel(x0, y0, x1, y1, 0);
        if (i == menu->sel)
            light(x0 + 7, (y0 + y1) / 2, menu_item_h * 0.15f, 1, 1);
        glColor3f(hot ? 1.0f : 0.0f, hot ? 1.0f : 0.0f, hot ? 1.0f : 0.1f);
        text(x0 + 14, (y0 + y1) / 2 - (big ? 4 : 3), menu->strs[i], x1 - x0 - 18, 0, big);
    }
}

static void gui_draw(pfPipeWindow *pw)
{
    int i;
    if (!gui_on || pw != gui_pw)
        return;
    compute_fit();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(0, 0, pw->xs, pw->ys);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, pw->xs, 0, pw->ys, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    pfBasicState();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glShadeModel(GL_FLAT);
    box((float)vp_x, (float)vp_y, (float)(vp_x + vp_w), (float)(vp_y + vp_h), 0.55f, 0.55f, 0.6f);
    glEnable(GL_SCISSOR_TEST);
    glScissor(vp_x, vp_y, vp_w, vp_h);
    for (i = 0; i < nwidgets; i++)
        if (widgets[i]->panel->enabled)
            draw_widget(widgets[i]);
    glDisable(GL_SCISSOR_TEST);
    if (menu)
        draw_menu();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

/* --- interaction --------------------------------------------------------------- */

static pfuWidget *widget_at(int x, int y)
{
    int i;
    for (i = nwidgets - 1; i >= 0; i--) {
        pfuWidget *w = widgets[i];
        if (!w->panel->enabled || w->type == PFUGUI_MESSAGE)
            continue;
        if (x >= px_x((float)w->x) && x < px_x((float)(w->x + w->w)) &&
            y >= px_y((float)w->y) && y < px_y((float)(w->y + w->h)))
            return w;
    }
    return NULL;
}

static void call(pfuWidget *w)
{
    if (w->action)
        w->action(w);
}

static int menu_item_at(int x, int y)
{
    int i;
    if (x < menu_x || x >= menu_x + menu_w)
        return -1;
    for (i = 0; i < menu->nsel; i++) {
        int y1 = menu_y - i * menu_item_h;
        if (y < y1 && y >= y1 - menu_item_h)
            return i;
    }
    return -1;
}

static void open_menu(pfuWidget *w)
{
    int ws = gui_pw ? gui_pw->ys : 480;
    menu = w;
    menu_x = (int)px_x((float)w->x);
    menu_w = (int)(w->w * fit_s);
    menu_item_h = (int)PF_MAX2(16, w->h * fit_s * 0.9f);
    /* below the button, or above when there is no room */
    menu_y = (int)px_y((float)w->y);
    if (menu_y - menu->nsel * menu_item_h < 0)
        menu_y = PF_MIN2(ws, (int)px_y((float)(w->y + w->h)) + menu->nsel * menu_item_h);
    menu_dragged = 0;
}

static void slide(pfuWidget *w, int x)
{
    float x0 = px_x((float)w->x) + 4, x1 = px_x((float)(w->x + w->w)) - 4;
    float v = slider_value(w, x1 > x0 ? (x - x0) / (x1 - x0) : 0);
    if (v != w->value) {
        w->value = v;
        call(w);
    }
}

static void radio(pfuWidget *w, int x)
{
    int toggle = w->type == PFUGUI_RADIO_BUTTON_TOGGLE;
    float x0 = px_x((float)w->x), x1 = px_x((float)(w->x + w->w));
    float lw = toggle ? (x1 - x0) * 0.26f : 0, cw;
    if (toggle && x < x0 + lw) {
        w->on = !w->on;
        call(w);
        return;
    }
    cw = w->nsel ? (x1 - x0 - lw) / w->nsel : 1;
    pfuWidgetSelection(w, (int)((x - x0 - lw) / cw));
    w->on = 1;
    call(w);
}

void pfuUpdateGUI(pfuMouse *mouse)
{
    int buttons = mouse->flags & PFUDEV_MOUSE_DOWN_MASK;
    int pressed = buttons && !prev_buttons, released = !buttons && prev_buttons;
    int x = mouse->xpos, y = mouse->ypos;
    prev_buttons = buttons;
    if (!gui_on)
        return;
    compute_fit();

    if (menu) {
        int item = menu_item_at(x, y);
        if (buttons && item < 0 && !pfuInGUI(x, y))
            menu_dragged = 1;
        if (buttons && item >= 0 && !pressed)
            menu_dragged = 1;
        if ((released && (item >= 0 || menu_dragged)) || (pressed && widget_at(x, y) != menu)) {
            pfuWidget *w = menu;
            menu = NULL;
            if (item >= 0 && w->enabled) {
                pfuWidgetSelection(w, item);
                call(w);
            }
            if (pressed)
                return;
        }
        if (menu)
            return;
    }

    if (pressed) {
        int gx, gy, gw, gh;
        pfuWidget *w;
        gui_rect(&gx, &gy, &gw, &gh);
        /* the presses made in the scene are not for the GUI */
        if (x < gx || x >= gx + gw || y < gy || y >= gy + gh)
            return;
        w = widget_at(x, y);
        if (!w || !w->enabled)
            return;
        active = w;
        switch (w->type) {
        case PFUGUI_BUTTON:
        case PFUGUI_SWITCH:
            w->pressed = 1;
            break;
        case PFUGUI_MENU_BUTTON:
            open_menu(w);
            active = NULL;
            break;
        case PFUGUI_SLIDER:
        case PFUGUI_SLIDER_LOG:
            slide(w, x);
            break;
        case PFUGUI_RADIO_BUTTON:
        case PFUGUI_RADIO_BUTTON_TOGGLE:
            radio(w, x);
            active = NULL;
            break;
        }
        return;
    }
    if (active && buttons) {
        if (active->type == PFUGUI_SLIDER || active->type == PFUGUI_SLIDER_LOG)
            slide(active, x);
        else
            active->pressed = widget_at(x, y) == active;
        return;
    }
    if (active && released) {
        pfuWidget *w = active;
        active = NULL;
        if ((w->type == PFUGUI_BUTTON || w->type == PFUGUI_SWITCH) && w->pressed) {
            w->pressed = 0;
            if (w->type == PFUGUI_SWITCH) {
                w->value = w->value != 0 ? 0.0f : 1.0f;
                w->on = w->value != 0;
            }
            call(w);
        }
        w->pressed = 0;
    }
}
