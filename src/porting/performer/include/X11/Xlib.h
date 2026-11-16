/*
 * X11/Xlib.h - the X display queries of the samples, answered by freeglut:
 * one screen, of the size of the desktop
 */
#ifndef PERFORMER_X11_XLIB_H
#define PERFORMER_X11_XLIB_H
typedef struct _XDisplay Display;
int pfuScreenWidth(void);
int pfuScreenHeight(void);
#define DefaultScreen(dsp) 0
#define ScreenCount(dsp) 1
#define DisplayWidth(dsp, scr) pfuScreenWidth()
#define DisplayHeight(dsp, scr) pfuScreenHeight()
/* the connection to the window system, after pfInit */
Display *pfGetCurWSConnection(void);
#endif
