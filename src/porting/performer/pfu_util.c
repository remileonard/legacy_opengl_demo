/*
 * pfu_util.c - libpfutil of IRIS Performer 2.0: the input of the windows
 * (freeglut callbacks queued as Performer events), the messages, the draw
 * styles, the display of the scene graph, the images and the textures.
 * The multiprocessing helpers do nothing: everything runs in one process.
 */
/* freeglut first: windows.h before the GL headers of Performer/pf.h */
#include <GL/freeglut.h>

#include "pf_internal.h"
#include <math.h>

/* --- processes ------------------------------------------------------------- */

void pfuInitUtil(void) {}
void pfuExitUtil(void) {}
int pfuPrioritizeProcs(int on) { (void)on; return 0; }
int pfuFreeCPUs(void) { return 0; }
int pfuLockDownProc(int cpu) { (void)cpu; return 0; }
int pfuLockDownApp(void) { return 0; }
int pfuLockDownCull(pfPipe *p) { (void)p; return 0; }
int pfuLockDownDraw(pfPipe *p) { (void)p; return 0; }
int pfuRunDiskProcsOn(int cpu) { (void)cpu; return 0; }

/* --- windows ---------------------------------------------------------------- */

static pfuGLXWindow glxwin;

pfuGLXWindow *pfuGLXWinopen(pfPipe *p, pfPipeWindow *pw, const char *name)
{
    (void)p;
    if (name && name[0])
        strncpy(pw->name, name, sizeof(pw->name) - 1);
    pfOpenPWin(pw);
    glxwin.pw = pw;
    glxwin.glutwin = pw->glutwin;
    glxwin.xsize = pw->xs;
    glxwin.ysize = pw->ys;
    return pw->glutwin ? &glxwin : NULL;
}

void pfuGetGLXWin(pfPipe *p, pfuGLXWindow *win)
{
    pfPipeWindow *pw = p->pwin;
    memset(win, 0, sizeof(*win));
    if (pw) {
        win->pw = pw;
        win->glutwin = pw->glutwin;
        win->xsize = pw->xs;
        win->ysize = pw->ys;
    }
}

void pfuMapWinColors(pfWindow *w, pfVec3 *clrs, int start, int num)
{
    (void)w; (void)clrs; (void)start; (void)num;
}

/* --- input ------------------------------------------------------------------- */

static pfuMouse live_mouse;
static pfuEventStream live_events;
/* the buttons pressed since the last pfuGetMouse, and those released: a
 * click between two frames is seen down for one frame */
static int unseen_press, pending_release;

static void add_dev(int dev, int val)
{
    pfuEventStream *e = &live_events;
    if (dev <= 0 || dev >= PFUDEV_MAX)
        return;
    if (e->devCount[dev] == 0 && e->numDevs < PFUDEV_MAX_DEVS) {
        e->devQ[e->numDevs] = dev;
        e->devVal[e->numDevs] = val;
        e->numDevs++;
    }
    e->devCount[dev]++;
}

static void add_key(int key)
{
    pfuEventStream *e = &live_events;
    key &= 0xff;
    add_dev(PFUDEV_KEYBD, key);
    if (e->keyCount[key] == 0 && e->numKeys < PFUDEV_MAX_KEYS)
        e->keyQ[e->numKeys++] = key;
    e->keyCount[key]++;
}

static void update_modifiers(void)
{
    int m = glutGetModifiers(), f = 0;
    if (m & GLUT_ACTIVE_SHIFT) f |= PFUDEV_MOD_SHIFT;
    if (m & GLUT_ACTIVE_CTRL) f |= PFUDEV_MOD_CTRL;
    if (m & GLUT_ACTIVE_ALT) f |= PFUDEV_MOD_ALT;
    live_mouse.modifiers = f;
}

static pfPipeWindow *cur_pwin(void)
{
    int i, w = glutGetWindow();
    for (i = 0; i < PFI_MAX_PIPES; i++) {
        pfPipe *p = pfGetPipe(i);
        if (p && p->pwin && p->pwin->glutwin == w)
            return p->pwin;
    }
    return NULL;
}

static void mouse_pos(int x, int y)
{
    pfPipeWindow *pw = cur_pwin();
    int h = pw ? pw->ys : glutGet(GLUT_WINDOW_HEIGHT);
    live_mouse.xpos = x;
    live_mouse.ypos = h - 1 - y;
    live_mouse.winSizeX = pw ? pw->xs : glutGet(GLUT_WINDOW_WIDTH);
    live_mouse.winSizeY = h;
    live_mouse.inWin = x >= 0 && y >= 0 && x < live_mouse.winSizeX && y < h;
}

static void on_keyboard(unsigned char key, int x, int y)
{
    (void)x; (void)y;
    update_modifiers();
    add_key(key);
}

static void on_special(int key, int x, int y)
{
    int dev = 0;
    (void)x; (void)y;
    update_modifiers();
    if (key >= GLUT_KEY_F1 && key <= GLUT_KEY_F12)
        dev = PFUDEV_F1KEY + key - GLUT_KEY_F1;
    else switch (key) {
    case GLUT_KEY_LEFT: dev = PFUDEV_LEFTARROWKEY; break;
    case GLUT_KEY_RIGHT: dev = PFUDEV_RIGHTARROWKEY; break;
    case GLUT_KEY_UP: dev = PFUDEV_UPARROWKEY; break;
    case GLUT_KEY_DOWN: dev = PFUDEV_DOWNARROWKEY; break;
    case GLUT_KEY_PAGE_UP: dev = PFUDEV_PAGEUPKEY; break;
    case GLUT_KEY_PAGE_DOWN: dev = PFUDEV_PAGEDOWNKEY; break;
    case GLUT_KEY_HOME: dev = PFUDEV_HOMEKEY; break;
    case GLUT_KEY_END: dev = PFUDEV_ENDKEY; break;
    case GLUT_KEY_INSERT: dev = PFUDEV_INSERTKEY; break;
    }
    if (dev)
        add_dev(dev, 1);
}

static void on_mouse(int button, int state, int x, int y)
{
    int bit = button == GLUT_LEFT_BUTTON ? PFUDEV_MOUSE_LEFT_DOWN :
              button == GLUT_MIDDLE_BUTTON ? PFUDEV_MOUSE_MIDDLE_DOWN :
              button == GLUT_RIGHT_BUTTON ? PFUDEV_MOUSE_RIGHT_DOWN : 0;
    update_modifiers();
    mouse_pos(x, y);
    if (!bit)
        return;
    if (state == GLUT_DOWN) {
        if (!(live_mouse.flags & PFUDEV_MOUSE_DOWN_MASK)) {
            live_mouse.xclick = live_mouse.xpos;
            live_mouse.yclick = live_mouse.ypos;
            live_mouse.clickTime = pfGetTime();
        }
        live_mouse.click = bit;
        live_mouse.flags |= bit;
        unseen_press |= bit;
        pending_release &= ~bit;
        add_dev(button == GLUT_LEFT_BUTTON ? PFUDEV_MOUSE_LEFT :
                button == GLUT_MIDDLE_BUTTON ? PFUDEV_MOUSE_MIDDLE : PFUDEV_MOUSE_RIGHT, 1);
    } else {
        if (unseen_press & bit)
            pending_release |= bit;
        else
            live_mouse.flags &= ~bit;
        live_mouse.release = bit;
        live_mouse.xrelease = live_mouse.xpos;
        live_mouse.yrelease = live_mouse.ypos;
        live_mouse.releaseTime = pfGetTime();
    }
}

static void on_motion(int x, int y) { mouse_pos(x, y); }

static void on_entry(int state)
{
    if (state == GLUT_LEFT)
        live_mouse.inWin = 0;
}

static void on_reshape(int w, int h)
{
    pfPipeWindow *pw = cur_pwin();
    if (pw) {
        pw->xs = w;
        pw->ys = h;
    }
    live_mouse.winSizeX = w;
    live_mouse.winSizeY = h;
    add_dev(PFUDEV_REDRAW, 1);
}

/* the callbacks of a window, set as soon as it is opened */
void pfuiWinCreated(pfPipeWindow *pw)
{
    glutSetWindow(pw->glutwin);
    glutKeyboardFunc(on_keyboard);
    glutSpecialFunc(on_special);
    glutMouseFunc(on_mouse);
    glutMotionFunc(on_motion);
    glutPassiveMotionFunc(on_motion);
    glutEntryFunc(on_entry);
    glutReshapeFunc(on_reshape);
    live_mouse.winSizeX = pw->xs;
    live_mouse.winSizeY = pw->ys;
}

void pfuInitInput(pfPipeWindow *pw, int mode)
{
    (void)mode;
    if (pw && pw->glutwin)
        pfuiWinCreated(pw);
}

void pfuExitInput(void) {}
void pfuCollectInput(void) {}

void pfuiPeekMouse(pfuMouse *mouse) { *mouse = live_mouse; }

void pfuGetMouse(pfuMouse *mouse)
{
    *mouse = live_mouse;
    unseen_press = 0;
    live_mouse.flags &= ~pending_release;
    pending_release = 0;
}

void pfuGetEvents(pfuEventStream *events)
{
    *events = live_events;
    pfuResetEventStream(&live_events);
}

void pfuResetEventStream(pfuEventStream *e)
{
    memset(e, 0, sizeof(*e));
}

/* --- cursors ------------------------------------------------------------------- */

static int cursor_type = PFU_CURSOR_X;

void pfuCursorType(int type) { cursor_type = type; }
int pfuGetCursorType(void) { return cursor_type; }
void pfuGUICursor(int target, int cursor) { (void)target; (void)cursor; }
void pfuUpdateGUICursor(void) {}
void pfuDrawPWin3DCursor(pfPipeWindow *pw, int x, int y) { (void)pw; (void)x; (void)y; }

/* --- text ------------------------------------------------------------------------- */

static void *font_of(int size)
{
    switch (size) {
    case PFU_FONT_TINY: return GLUT_BITMAP_HELVETICA_10;
    case PFU_FONT_SMALL: return GLUT_BITMAP_HELVETICA_12;
    case PFU_FONT_MED: return GLUT_BITMAP_HELVETICA_18;
    }
    return GLUT_BITMAP_TIMES_ROMAN_24;
}

static int font_height(int size)
{
    switch (size) {
    case PFU_FONT_TINY: return 10;
    case PFU_FONT_SMALL: return 12;
    case PFU_FONT_MED: return 18;
    }
    return 24;
}

static int text_width(const char *s, void *font)
{
    int w = 0;
    for (; *s && *s != '\n'; s++)
        w += glutBitmapWidth(font, (unsigned char)*s);
    return w;
}

void pfuiTextWidth(const char *s, int big, int *w)
{
    *w = text_width(s, big ? GLUT_BITMAP_HELVETICA_12 : GLUT_BITMAP_HELVETICA_10);
}

void pfuiDrawText(float x, float y, const char *s, int big)
{
    void *font = big ? GLUT_BITMAP_HELVETICA_12 : GLUT_BITMAP_HELVETICA_10;
    glRasterPos2f(x, y);
    for (; *s; s++)
        glutBitmapCharacter(font, (unsigned char)*s);
}

static void draw_message(const char *msg, int x0, int y0, int w, int h, int just,
                         float x, float y, int size, const float *color, const float *shadow)
{
    void *font = font_of(size);
    int fh = font_height(size), lines = 1, i;
    const char *p;
    for (p = msg; *p; p++)
        if (*p == '\n')
            lines++;
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glViewport(x0, y0, w, h);
    pfBasicState();
    glDisable(GL_DEPTH_TEST);
    p = msg;
    for (i = 0; i < lines; i++) {
        int tw = text_width(p, font), k;
        float tx = x * w, ty = y * h + (lines / 2.0f - i - 1) * fh * 1.25f + fh * 0.25f;
        if (just == PFU_CENTER_JUSTIFIED) tx -= tw / 2.0f;
        else if (just == PFU_RIGHT_JUSTIFIED) tx -= tw;
        if (tx > w - tw) tx = (float)(w - tw);
        if (tx < 2) tx = 2;
        if (ty > h - fh) ty = (float)(h - fh);
        for (k = 0; k < 2; k++) {
            const char *q;
            if (k == 0 && !shadow)
                continue;
            glColor3fv(k == 0 ? shadow : color);
            glRasterPos2f(tx + (k == 0 ? 2 : 0), ty - (k == 0 ? 2 : 0));
            for (q = p; *q && *q != '\n'; q++)
                glutBitmapCharacter(font, (unsigned char)*q);
        }
        while (*p && *p != '\n')
            p++;
        if (*p)
            p++;
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

static void message_area(pfChannel *chan, int rel, int *x, int *y, int *w, int *h)
{
    if (chan && rel == PFU_MSG_CHAN) {
        pfiChanRect(chan, x, y, w, h);
    } else {
        int vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
        if (chan && chan->pipe->pwin) {
            vp[0] = vp[1] = 0;
            vp[2] = chan->pipe->pwin->xs;
            vp[3] = chan->pipe->pwin->ys;
        } else {
            int win = glutGetWindow();
            if (win) {
                vp[0] = vp[1] = 0;
                vp[2] = glutGet(GLUT_WINDOW_WIDTH);
                vp[3] = glutGet(GLUT_WINDOW_HEIGHT);
            }
        }
        *x = vp[0]; *y = vp[1]; *w = vp[2]; *h = vp[3];
    }
}

void pfuDrawMessage(pfChannel *chan, const char *msg, int rel, int just,
                    float x, float y, int size, int cimode)
{
    static const float purple[3] = { 0.8f, 0.2f, 0.9f }, shadow[3] = { 0.15f, 0.05f, 0.15f };
    int x0, y0, w, h;
    (void)cimode;
    message_area(chan, rel, &x0, &y0, &w, &h);
    draw_message(msg, x0, y0, w, h, just, x, y, size, purple, shadow);
}

void pfuDrawMessageCI(pfChannel *chan, const char *msg, int rel, int just,
                      float x, float y, int size, int textClr, int shadowClr)
{
    static const float purple[3] = { 0.4f, 0.0f, 0.5f }, shadow[3] = { 0.125f, 0.06f, 0.125f };
    int x0, y0, w, h;
    (void)textClr; (void)shadowClr;
    message_area(chan, rel, &x0, &y0, &w, &h);
    draw_message(msg, x0, y0, w, h, just, x, y, size, purple, shadow);
}

/* --- draw styles: the multipass styles draw the scene a second time -------------- */

static float scribe[4] = { 1, 1, 1, 1 };

void pfuPreDrawStyle(int style, pfVec4 scribeColor)
{
    if (scribeColor)
        memcpy(scribe, scribeColor, sizeof(scribe));
    switch (style) {
    case PFUSTYLE_POINTS:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glPointSize(2);
        break;
    case PFUSTYLE_LINES:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case PFUSTYLE_DASHED:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(2, 0x0f0f);
        break;
    case PFUSTYLE_HALOED:
        /* wide lines in the depth buffer only: the lines behind them are
         * cut by the halos */
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(6);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        break;
    case PFUSTYLE_HIDDEN:
        /* the faces in the depth buffer only */
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1, 1);
        break;
    case PFUSTYLE_SCRIBED:
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1, 1);
        break;
    }
}

static void reset_style(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0, 0);
    glLineWidth(1);
    glPointSize(1);
    glDepthFunc(GL_LESS);
}

void pfuPostDrawStyle(int style)
{
    switch (style) {
    case PFUSTYLE_HALOED:
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glLineWidth(1);
        glDepthFunc(GL_LEQUAL);
        pfDraw();
        break;
    case PFUSTYLE_HIDDEN:
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDepthFunc(GL_LEQUAL);
        pfDraw();
        break;
    case PFUSTYLE_SCRIBED:
        glDisable(GL_POLYGON_OFFSET_FILL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDepthFunc(GL_LEQUAL);
        pfForceColor(scribe);
        pfDraw();
        pfForceColor(NULL);
        break;
    }
    reset_style();
}

/* --- scene graph tools ----------------------------------------------------------------- */

void pfuCollideSetup(pfNode *node, int mode, unsigned int mask) { (void)node; (void)mode; (void)mask; }
void pfuTravSetDListMode(pfNode *node, int on, int attrs) { (void)node; (void)on; (void)attrs; }
void pfuTravCreatePackedAttrs(pfNode *node, int format, int mask) { (void)node; (void)format; (void)mask; }

static void collect_tex(pfNode *n, pfList *l, int depth)
{
    int i, j;
    if (!n || depth > 64)
        return;
    for (i = 0; i < n->ngsets; i++) {
        pfGeoState *gs = n->gsets[i]->gstate;
        if (gs && gs->texture) {
            for (j = 0; j < pfGetNum(l); j++)
                if (pfGetListElt(l, j) == gs->texture)
                    break;
            if (j == pfGetNum(l))
                pfAdd(l, gs->texture);
        }
    }
    for (i = 0; i < n->nchildren; i++)
        collect_tex(n->children[i], l, depth + 1);
}

pfList *pfuMakeSceneTexList(pfNode *node)
{
    pfList *l = pfNewList(sizeof(void *), 16, NULL);
    collect_tex(node, l, 0);
    return l;
}

/* the textures are loaded in the context now rather than at their first use */
void pfuDownloadTexList(pfList *list, int style)
{
    int i;
    (void)style;
    for (i = 0; i < pfGetNum(list); i++)
        pfApplyTex((pfTexture *)pfGetListElt(list, i));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void pfuConfigMCO(pfChannel **chans, int n)
{
    int i;
    for (i = 0; i < n; i++)
        pfChanViewport(chans[i], (float)i / n, (float)(i + 1) / n, 0, 1);
}

/* the hierarchy: the nodes by depth, the leaves spread across the width */
typedef struct {
    float x;
    int leaves;
} TreeInfo;

static int tree_leaves(pfNode *n, int depth)
{
    int i, s = 0;
    if (!n || depth > 32 || n->nchildren == 0)
        return 1;
    for (i = 0; i < n->nchildren; i++)
        s += tree_leaves(n->children[i], depth + 1);
    return s;
}

static int tree_depth(pfNode *n, int depth)
{
    int i, d = depth;
    if (!n || depth > 32)
        return depth;
    for (i = 0; i < n->nchildren; i++) {
        int c = tree_depth(n->children[i], depth + 1);
        if (c > d)
            d = c;
    }
    return d;
}

static void node_color(int type)
{
    switch (type) {
    case PFNODE_SCENE: glColor3f(1, 1, 1); break;
    case PFNODE_DCS: case PFNODE_SCS: glColor3f(1, 0.5f, 0.2f); break;
    case PFNODE_GEODE: case PFNODE_BBOARD: glColor3f(0.3f, 1, 0.3f); break;
    case PFNODE_LOD: glColor3f(1, 1, 0.2f); break;
    case PFNODE_SWITCH: case PFNODE_SEQUENCE: glColor3f(1, 0.3f, 1); break;
    case PFNODE_LSOURCE: glColor3f(1, 1, 0.8f); break;
    default: glColor3f(0.4f, 0.7f, 1); break;
    }
}

static void draw_tree_node(pfNode *n, float x0, float x1, int depth, float dy, float sx, float sy,
                           float px, float py, int labels)
{
    float x = (x0 + x1) / 2, y = 1 - (depth + 0.5f) * dy, cx = x0;
    float X = (x - 0.5f) * sx + 0.5f + px, Y = (y - 1) * sy + 1 - py;
    int i, total;
    if (!n || depth > 32)
        return;
    total = tree_leaves(n, depth);
    for (i = 0; i < n->nchildren; i++) {
        pfNode *c = n->children[i];
        float w = (x1 - x0) * tree_leaves(c, depth + 1) / total;
        float cxm = cx + w / 2, cy = 1 - (depth + 1.5f) * dy;
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_LINES);
        glVertex2f(X, Y);
        glVertex2f((cxm - 0.5f) * sx + 0.5f + px, (cy - 1) * sy + 1 - py);
        glEnd();
        draw_tree_node(c, cx, cx + w, depth + 1, dy, sx, sy, px, py, labels);
        cx += w;
    }
    node_color(n->type);
    glBegin(GL_QUADS);
    glVertex2f(X - 0.004f, Y - 0.006f);
    glVertex2f(X + 0.004f, Y - 0.006f);
    glVertex2f(X + 0.004f, Y + 0.006f);
    glVertex2f(X - 0.004f, Y + 0.006f);
    glEnd();
    if (labels) {
        static const char *names[] = {
            "", "Group", "Scene", "DCS", "SCS", "Switch", "Seq", "LOD", "Layer",
            "Geode", "Morph", "BBoard", "LSource"
        };
        const char *t = n->type > 0 && n->type <= PFNODE_LSOURCE ? names[n->type] : "?";
        char buf[48];
        if (n->name)
            sprintf(buf, "%s %.24s", t, n->name);
        else
            sprintf(buf, "%s", t);
        glRasterPos2f(X + 0.006f, Y - 0.004f);
        for (t = buf; *t; t++)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, (unsigned char)*t);
    }
}

void pfuDrawTree(pfChannel *chan, pfNode *node, pfVec3 panScale)
{
    int x, y, w, h, depth, leaves;
    float s = panScale[2] > 0 ? panScale[2] : 1;
    pfiChanRect(chan, &x, &y, &w, &h);
    depth = tree_depth(node, 0) + 1;
    leaves = tree_leaves(node, 0);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    pfBasicState();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0, 0, 0.6f);
    glRectf(0, 0, 1, 1);
    /* the scale spreads the leaves, the pans move across and down */
    draw_tree_node(node, 0, 1, 0, 1.0f / depth, s * PF_MAX2(1.0f, leaves / 60.0f), s,
                   -panScale[0] * s * PF_MAX2(1.0f, leaves / 60.0f) / 2, -panScale[1] * s,
                   leaves * s < 80 || s * w / (float)leaves > 60);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

/* --- images ---------------------------------------------------------------------------- */

static void put16(FILE *fp, int v) { fputc((v >> 8) & 0xff, fp); fputc(v & 0xff, fp); }
static void put32(FILE *fp, long v)
{
    fputc((int)((v >> 24) & 0xff), fp); fputc((int)((v >> 16) & 0xff), fp);
    fputc((int)((v >> 8) & 0xff), fp); fputc((int)(v & 0xff), fp);
}

/* an uncompressed SGI image of the front buffer */
int pfuSaveImage(const char *file, int xorg, int yorg, int xsize, int ysize, int alpha)
{
    unsigned char *pix, hdr[404];
    int zsize = alpha ? 4 : 3, c, y, x;
    FILE *fp;
    if (xsize <= 0 || ysize <= 0 || (fp = fopen(file, "wb")) == NULL)
        return 0;
    pix = (unsigned char *)malloc((size_t)xsize * ysize * 4);
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(xorg, yorg, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, pix);
    glReadBuffer(GL_BACK);
    put16(fp, 474);
    fputc(0, fp);           /* verbatim */
    fputc(1, fp);           /* one byte per channel */
    put16(fp, 3);
    put16(fp, xsize);
    put16(fp, ysize);
    put16(fp, zsize);
    put32(fp, 0);
    put32(fp, 255);
    put32(fp, 0);
    memset(hdr, 0, sizeof(hdr));
    strncpy((char *)hdr, "perfly", 79);
    fwrite(hdr, 1, 80, fp);
    put32(fp, 0);
    fwrite(hdr, 1, 404, fp);
    for (c = 0; c < zsize; c++)
        for (y = 0; y < ysize; y++)
            for (x = 0; x < xsize; x++)
                fputc(pix[4 * ((size_t)y * xsize + x) + c], fp);
    fclose(fp);
    free(pix);
    return 1;
}

/* the border of the clip texture centering of libpfutil (clipcenter.c),
 * changed by the arrow keys of perfly */
int GLOBALBorder = 64;
