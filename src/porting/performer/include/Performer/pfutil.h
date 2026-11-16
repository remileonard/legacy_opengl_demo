/*
 * Performer/pfutil.h - the libpfutil of IRIS Performer 2.0 used by the
 * sample programs: the input of the windows (on freeglut), the GUI panel
 * and its widgets, the draw styles, messages, the scene graph display,
 * images, textures and the multiprocessing helpers (which do nothing).
 */
#ifndef PERFORMER_PFUTIL_H
#define PERFORMER_PFUTIL_H

#include <Performer/pf.h>

/* --- processes: one process, nothing to lock -------------------------------- */

void pfuInitUtil(void);
void pfuExitUtil(void);
int pfuPrioritizeProcs(int on);
int pfuFreeCPUs(void);
int pfuLockDownProc(int cpu);
int pfuLockDownApp(void);
int pfuLockDownCull(pfPipe *p);
int pfuLockDownDraw(pfPipe *p);
int pfuRunDiskProcsOn(int cpu);

/* --- windows ------------------------------------------------------------------- */

typedef struct pfuGLXWindow {
    pfPipeWindow *pw;
    int glutwin;
    int xsize, ysize;
} pfuGLXWindow;

pfuGLXWindow *pfuGLXWinopen(pfPipe *p, pfPipeWindow *pw, const char *name);
void pfuGetGLXWin(pfPipe *p, pfuGLXWindow *win);
void pfuMapWinColors(pfWindow *w, pfVec3 *clrs, int start, int num);

/* --- input ---------------------------------------------------------------------- */

#define PFUINPUT_GL 0
#define PFUINPUT_X 1
#define PFUINPUT_NOFORK_X 2

/* devices */
#define PFUDEV_NULL 0
#define PFUDEV_REDRAW 1
#define PFUDEV_WINQUIT 2
#define PFUDEV_KEYBD 3
#define PFUDEV_MOUSE_LEFT 4
#define PFUDEV_MOUSE_MIDDLE 5
#define PFUDEV_MOUSE_RIGHT 6
#define PFUDEV_F1KEY 10
#define PFUDEV_F2KEY 11
#define PFUDEV_F3KEY 12
#define PFUDEV_F4KEY 13
#define PFUDEV_F5KEY 14
#define PFUDEV_F6KEY 15
#define PFUDEV_F7KEY 16
#define PFUDEV_F8KEY 17
#define PFUDEV_F9KEY 18
#define PFUDEV_F10KEY 19
#define PFUDEV_F11KEY 20
#define PFUDEV_F12KEY 21
#define PFUDEV_LEFTARROWKEY 22
#define PFUDEV_RIGHTARROWKEY 23
#define PFUDEV_UPARROWKEY 24
#define PFUDEV_DOWNARROWKEY 25
#define PFUDEV_PAGEUPKEY 26
#define PFUDEV_PAGEDOWNKEY 27
#define PFUDEV_HOMEKEY 28
#define PFUDEV_ENDKEY 29
#define PFUDEV_INSERTKEY 30
#define PFUDEV_PRINTSCREENKEY 31
#define PFUDEV_WINSIZE 32
#define PFUDEV_MAX 64

#define PFUDEV_MAX_DEVS 256
#define PFUDEV_MAX_KEYS 256

/* mouse buttons in pfuMouse.flags */
#define PFUDEV_MOUSE_LEFT_DOWN 0x1
#define PFUDEV_MOUSE_MIDDLE_DOWN 0x2
#define PFUDEV_MOUSE_RIGHT_DOWN 0x4
#define PFUDEV_MOUSE_DOWN_MASK 0x7

/* modifiers in pfuMouse.modifiers */
#define PFUDEV_MOD_SHIFT 0x1
#define PFUDEV_MOD_CTRL 0x2
#define PFUDEV_MOD_ALT 0x4
#define PFUDEV_MOD_LEFT_SHIFT PFUDEV_MOD_SHIFT
#define PFUDEV_MOD_LEFT_CTRL PFUDEV_MOD_CTRL

typedef struct pfuMouse {
    int flags;              /* PFUDEV_MOUSE_*_DOWN */
    int modifiers;          /* PFUDEV_MOD_* */
    int xpos, ypos;         /* in the window, origin at the bottom left */
    float xchan, ychan;     /* in [-1, 1] of the master channel */
    int xclick, yclick;     /* where the last button was pressed */
    int click;              /* the button of that press */
    double clickTime;
    int release;
    int xrelease, yrelease;
    double releaseTime;
    int winSizeX, winSizeY;
    int winOriginX, winOriginY;
    int inWin;
    pfChannel *chan;
} pfuMouse;

typedef struct pfuEventStream {
    int numDevs;
    int devQ[PFUDEV_MAX_DEVS];
    int devVal[PFUDEV_MAX_DEVS];
    int devCount[PFUDEV_MAX];
    int numKeys;
    int keyQ[PFUDEV_MAX_KEYS];
    int keyCount[PFUDEV_MAX_KEYS];
    int buttonFlags;
} pfuEventStream;

void pfuInitInput(pfPipeWindow *pw, int mode);
void pfuExitInput(void);
/* in the draw process: the events of the windows (freeglut) */
void pfuCollectInput(void);
void pfuGetMouse(pfuMouse *mouse);
void pfuGetEvents(pfuEventStream *events);
void pfuResetEventStream(pfuEventStream *events);

/* --- cursors ---------------------------------------------------------------------- */

#define PFU_CURSOR_X 0
#define PFU_CURSOR_3D 1
#define PFU_CURSOR_MAIN 0
#define PFU_CURSOR_GUI 1
#define PFU_CURSOR_OFF 0
#define PFU_CURSOR_DEFAULT 1

void pfuCursorType(int type);
int pfuGetCursorType(void);
void pfuGUICursor(int target, int cursor);
void pfuUpdateGUICursor(void);
void pfuDrawPWin3DCursor(pfPipeWindow *pw, int x, int y);

/* --- GUI ------------------------------------------------------------------------- */

#define PFUGUI_MAXWIDGETS 256

/* widget types, PFUGUI_HORIZONTAL can be or'ed to the radio buttons */
#define PFUGUI_BUTTON 0x1
#define PFUGUI_SWITCH 0x2
#define PFUGUI_MENU_BUTTON 0x3
#define PFUGUI_SLIDER 0x4
#define PFUGUI_SLIDER_LOG 0x5
#define PFUGUI_RADIO_BUTTON 0x6
#define PFUGUI_RADIO_BUTTON_TOGGLE 0x7
#define PFUGUI_MESSAGE 0x8
#define PFUGUI_TYPE_MASK 0xff
#define PFUGUI_HORIZONTAL 0x100
#define PFUGUI_VERTICAL 0x000

/* the layout of the widgets, in the units of the panel */
#define PFUGUI_BUTTON_XSIZE 60
#define PFUGUI_BUTTON_XINC 64
#define PFUGUI_BUTTON_LONG_XSIZE 90
#define PFUGUI_BUTTON_LONG_XINC 94
#define PFUGUI_BUTTON_VLONG_XSIZE 120
#define PFUGUI_BUTTON_VLONG_XINC 124
#define PFUGUI_BUTTON_YSIZE 24
#define PFUGUI_BUTTON_YINC 28
#define PFUGUI_SLIDER_XSIZE 244
#define PFUGUI_SLIDER_YSIZE 30
#define PFUGUI_SLIDER_YINC 34
#define PFUGUI_MESSAGE_XSIZE 244
#define PFUGUI_MESSAGE_YSIZE 24
#define PFUGUI_PANEL_MARGIN 4

typedef char *pfuGUIString;
typedef struct pfuWidget pfuWidget;
typedef struct pfuPanel pfuPanel;
typedef void (*pfuWidgetFuncType)(pfuWidget *w);

void pfuInitGUI(pfPipeWindow *pw);
void pfuExitGUI(void);
void pfuEnableGUI(int on);
void pfuGUIViewport(float l, float r, float b, float t);
void pfuGetGUIViewport(float *l, float *r, float *b, float *t);
int pfuInGUI(int x, int y);
void pfuUpdateGUI(pfuMouse *mouse);
void pfuRedrawGUI(void);
void pfuResetGUI(void);
pfChannel *pfuGetGUIChan(void);
pfHighlight *pfuGetGUIHlight(void);

pfuPanel *pfuNewPanel(void);
void pfuEnablePanel(pfuPanel *p);
void pfuDisablePanel(pfuPanel *p);
void pfuGetPanelOriginSize(pfuPanel *p, float *xo, float *yo, float *xs, float *ys);

pfuWidget *pfuNewWidget(pfuPanel *p, int type, int id);
int pfuGetWidgetType(pfuWidget *w);
int pfuGetWidgetId(pfuWidget *w);
void pfuWidgetDim(pfuWidget *w, int xo, int yo, int xs, int ys);
void pfuGetWidgetDim(pfuWidget *w, int *xo, int *yo, int *xs, int *ys);
void pfuWidgetLabel(pfuWidget *w, const char *label);
const char *pfuGetWidgetLabel(pfuWidget *w);
void pfuWidgetRange(pfuWidget *w, int mode, float min, float max, float val);
void pfuWidgetValue(pfuWidget *w, float val);
float pfuGetWidgetValue(pfuWidget *w);
void pfuWidgetDefaultValue(pfuWidget *w, float val);
void pfuWidgetSelections(pfuWidget *w, pfuGUIString *strs, int *vals,
                         void (**funcs)(pfuWidget *), int n);
void pfuWidgetSelection(pfuWidget *w, int index);
int pfuGetWidgetSelection(pfuWidget *w);
void pfuWidgetActionFunc(pfuWidget *w, pfuWidgetFuncType func);
pfuWidgetFuncType pfuGetWidgetActionFunc(pfuWidget *w);
void pfuWidgetOnOff(pfuWidget *w, int on);
int pfuIsWidgetOn(pfuWidget *w);
void pfuEnableWidget(pfuWidget *w);
void pfuDisableWidget(pfuWidget *w);
int pfuGetWidgetEnabled(pfuWidget *w);

/* --- messages ------------------------------------------------------------------- */

#define PFU_MSG_PIPE 0
#define PFU_MSG_CHAN 1
#define PFU_LEFT_JUSTIFIED 0
#define PFU_CENTER_JUSTIFIED 1
#define PFU_RIGHT_JUSTIFIED 2
#define PFU_FONT_TINY 0
#define PFU_FONT_SMALL 1
#define PFU_FONT_MED 2
#define PFU_FONT_BIG 3
#define PFU_RGB 0
#define PFU_CI 1

/* a text in a channel or in the whole window, x and y in [0, 1] */
void pfuDrawMessage(pfChannel *chan, const char *msg, int rel, int just,
                    float x, float y, int size, int cimode);
void pfuDrawMessageCI(pfChannel *chan, const char *msg, int rel, int just,
                      float x, float y, int size, int textClr, int shadowClr);

/* --- draw styles ------------------------------------------------------------------ */

#define PFUSTYLE_POINTS 0
#define PFUSTYLE_LINES 1
#define PFUSTYLE_DASHED 2
#define PFUSTYLE_HALOED 3
#define PFUSTYLE_HIDDEN 4
#define PFUSTYLE_FILLED 5
#define PFUSTYLE_SCRIBED 6
#define PFUSTYLE_COUNT 7

void pfuPreDrawStyle(int style, pfVec4 scribeColor);
void pfuPostDrawStyle(int style);

/* --- scene graph tools ------------------------------------------------------------ */

#define PFUCOLLIDE_STATIC 0
#define PFUCOLLIDE_DYNAMIC 1
void pfuCollideSetup(pfNode *node, int mode, unsigned int mask);

#define PFUTEX_SHOW 0x1
#define PFUTEX_APPLY 0x2
pfList *pfuMakeSceneTexList(pfNode *node);
void pfuDownloadTexList(pfList *list, int style);

#define PFU_ATTRS_NONE 0
void pfuTravSetDListMode(pfNode *node, int on, int attrs);
void pfuTravCreatePackedAttrs(pfNode *node, int format, int mask);

/* draws the hierarchy of a scene graph in a channel, pan (x, y) and scale */
void pfuDrawTree(pfChannel *chan, pfNode *node, pfVec3 panScale);

/* the channels side by side in the window (the Multi Channel Option) */
void pfuConfigMCO(pfChannel **chans, int n);

/* writes an area of the front buffer as an SGI image */
int pfuSaveImage(const char *file, int xorg, int yorg, int xsize, int ysize, int alpha);

#endif
