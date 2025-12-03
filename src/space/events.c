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

static uint16 cursor[16] = {
          0x0100, 0x0100, 0x0100, 0x0100,
          0x0100, 0x0100, 0x0100, 0xFFFE,
          0x0100, 0x0100, 0x0100, 0x0100,
          0x0100, 0x0100, 0x0100, 0x0000
} ;

#ifdef SP_OPEN_GL
static Bool WaitForNotify(Display *d, XEvent *e, char *arg)
   { return (e->type == MapNotify) && (e->xmap.window == (Window)arg); }
#endif

extern t_stopwatch Counter ;

/**********************************************************************
*  spOpenWindow()                                                
**********************************************************************/
uint32 spOpenWindow(void)

{
#ifdef SP_IRIS_GL
   if (getgdesc(GD_BITS_NORM_DBL_RED) <= 0)  {
     fprintf(stderr,"Need doublebuffer RGB mode for this demo!\n") ;
     exit(0) ;
     }

   if (Counter.flags & FLSCR_FLAG)
     prefposition(0,getgdesc(GD_XPMAX)-1,0,getgdesc(GD_YPMAX)-1) ;

   noborder() ;
   foreground() ;
   Counter.winst.wid = winopen("Space");

   Counter.rotsizex = getgdesc(GD_XPMAX); 
   Counter.rotsizey = getgdesc(GD_YPMAX);

   RGBmode() ;
   doublebuffer() ;
   if (getgdesc(GD_MULTISAMPLE))  {
     mssize(8,32,0);
     zbsize(0);
     gconfig() ;

     if (getgconfig(GC_BITS_MS_ZBUFFER) == 0 || getgconfig(GC_MS_SAMPLES) < 4) {
       mssize(0,0,0);
       zbsize(32);
       gconfig() ;
       }
     }
   else gconfig() ;

   return(1);
#endif

#ifdef SP_OPEN_GL
   Colormap cmap;
   XSetWindowAttributes swa;
   XEvent event;
   uint32 x1,y1,x2,y2;
   int arr[8];

   /* get display */
   if (!(Counter.winst.dpy = XOpenDisplay(0))) {
     fprintf(stderr,"ERROR: XOpenDisplay() failed.\n");
     return(0);
     }

   /* get screen number */
   Counter.winst.snu = DefaultScreen(Counter.winst.dpy);

   /* screen size */
   Counter.rotsizex = DisplayWidth(Counter.winst.dpy,Counter.winst.snu);
   Counter.rotsizey = DisplayHeight(Counter.winst.dpy,Counter.winst.snu);

   /* get visual */
   arr[0] = GLX_RGBA;
   arr[1] = GLX_DEPTH_SIZE;
   arr[2] = 24;
   arr[3] = GLX_DOUBLEBUFFER;
   arr[4] = None;
   if (!(Counter.winst.vis = glXChooseVisual(Counter.winst.dpy,Counter.winst.snu,arr)))  {
     fprintf(stderr,"ERROR: glXChooseVisual() failed.\n");
     return(0);
     }

   /* get context */
   if (!(Counter.winst.ctx = glXCreateContext(Counter.winst.dpy,Counter.winst.vis,0,GL_TRUE))) {
     fprintf(stderr,"ERROR: glXCreateContext() failed.\n");
     return(0);
     }

   /* root window */
   Counter.winst.rwi = RootWindow(Counter.winst.dpy,Counter.winst.vis->screen);

   /* get RGBA colormap */
   if (!(cmap = XCreateColormap(Counter.winst.dpy,Counter.winst.rwi,Counter.winst.vis->visual,AllocNone))) {
     fprintf(stderr,"ERROR: glXCreateColorMap() failed.\n");
     return(0);
     }

   if (Counter.flags & FLSCR_FLAG) {
     x1 = 0;
     y1 = 0;
     x2 = Counter.rotsizex;
     y2 = Counter.rotsizey;
     }
   else {
     x1 = 100;
     y1 = 100;
     x2 = 512;
     y2 = 512;
     }

   /* get window */
   swa.colormap = cmap;
   swa.border_pixel = 0;
   swa.event_mask = StructureNotifyMask;
   if (!(Counter.winst.win = XCreateWindow(Counter.winst.dpy,Counter.winst.rwi,x1,y1,x2,y2,0,Counter.winst.vis->depth,
           InputOutput,Counter.winst.vis->visual,CWBorderPixel|CWColormap|CWEventMask,&swa))) {
     fprintf(stderr,"ERROR: XCreateWindow() failed.\n");
     return(0);
     }

   if (Counter.flags & FLSCR_FLAG) {
     XSizeHints sh;

     sh.flags = USPosition;
     sh.x = x1;
     sh.y = y1;
     XSetStandardProperties(Counter.winst.dpy,Counter.winst.win,"", "", None, 0, 0, &sh);
     }

   {  /* noborder */
      struct { long flags,functions,decorations,input_mode; } hints;
      int     fmt;
      uint32  nitems,byaf;
      Atom    type,mwmhints = XInternAtom(Counter.winst.dpy,"_MOTIF_WM_HINTS",False);
   
      XGetWindowProperty(Counter.winst.dpy,Counter.winst.win,mwmhints,0,4,False,
                                mwmhints,&type,&fmt,&nitems,&byaf,(char**)&hints);
   
      hints.decorations = 0;
      hints.flags |= 2;
   
      XChangeProperty(Counter.winst.dpy,Counter.winst.win,mwmhints,mwmhints,32,PropModeReplace,(uchar8 *)&hints,4);
      XFlush(Counter.winst.dpy);
   }

   XStoreName(Counter.winst.dpy,Counter.winst.win, "space");
   XMapWindow(Counter.winst.dpy,Counter.winst.win);
   XFlush(Counter.winst.dpy);

   XIfEvent(Counter.winst.dpy,&event,WaitForNotify,(char*)Counter.winst.win);

   glXMakeCurrent(Counter.winst.dpy,Counter.winst.win,Counter.winst.ctx);
   return(1);
#endif
}

/**********************************************************************
*  spGetWindowGeometry()                                                
**********************************************************************/
void spGetWindowGeometry(void)

{
#ifdef SP_IRIS_GL
   getsize(&Counter.winsizex,&Counter.winsizey);
   getorigin(&Counter.winorigx,&Counter.winorigy);
#endif

#ifdef SP_OPEN_GL
   XWindowAttributes wa;

   XGetWindowAttributes(Counter.winst.dpy,Counter.winst.win,&wa);
   Counter.winorigx = wa.x;
   Counter.winorigy = wa.y;
   Counter.winsizex = wa.width;
   Counter.winsizey = wa.height;
#endif
}

/**********************************************************************
*  spInitFont()                                                
**********************************************************************/
uint32 spInitFont(char *fn)

{
#ifdef SP_IRIS_GL
   return(1);
#endif

#ifdef SP_OPEN_GL
   XFontStruct *fontInfo;
   uint32 firt, last;

   /* Lookup font */
   if (!(fontInfo = XLoadQueryFont(Counter.winst.dpy,fn)))  {
     fprintf(stderr,"ERROR: XLoadQueryFont() failed.\n");
     return(0);
     }

   firt = fontInfo->min_char_or_byte2;
   last = fontInfo->max_char_or_byte2;

   if (!(Counter.fontbase = glGenLists(last+1))) {
     fprintf(stderr,"ERROR: glGenLists() failed.\n");
     return(0);
     }

   glXUseXFont(fontInfo->fid, firt, last-firt+1, Counter.fontbase+firt);
   return(1);
#endif
}

/**********************************************************************
*  spDrawString()                                                
**********************************************************************/
void spDrawString(float x,float y,float z,char *ch)

{
#ifdef SP_IRIS_GL
   cmov(x,y,z);
   charstr(ch);
#endif

#ifdef SP_OPEN_GL
    glRasterPos3f(x,y,z);
    glListBase(Counter.fontbase);
    glCallLists(strlen(ch),GL_UNSIGNED_BYTE,ch);
#endif
}

/**********************************************************************
*  spInitCursor()                                                
**********************************************************************/
void spInitCursor(void)

{
   Counter.mouse_x = 0;
   Counter.mouse_y = 0;
   Counter.mouse_b = 0;
   Counter.mouse_n = 0;

#ifdef SP_IRIS_GL
   curstype(C16X1) ;
   defcursor(1,cursor) ;
   curorigin(1,7,7) ;
   setcursor(1,0,0) ;

   setvaluator(MOUSEX,Counter.winorigx+Counter.winsizex/2,0,getgdesc(GD_XPMAX)-1) ;
   setvaluator(MOUSEY,Counter.winorigy+Counter.winsizey/2,0,getgdesc(GD_YPMAX)-1) ;
#endif

#ifdef SP_OPEN_GL
   XWarpPointer(Counter.winst.dpy,None,Counter.winst.win,0,0,0,0,
         Counter.winorigx+(Counter.winsizex>>1),Counter.winorigy+(Counter.winsizey>>1));
#endif
}

/**********************************************************************
*  spInitKeyboard()                                                
**********************************************************************/
void spInitKeyboard(void)

{
#ifdef SP_IRIS_GL
   qdevice(ESCKEY); /* exit */
   qdevice(BUT10);  /* a: autopilot */
   qdevice(BUT11);  /* s: stats */
   qdevice(BUT20);  /* x: text toggle */
   qdevice(BUT26);  /* h: help */
   qdevice(BUT41);  /* l: shade every frame */
   qdevice(BUT28);  /* v: reverse velocity */
   qdevice(BUT23);  /* r: time slow */
   qdevice(BUT24);  /* t: time fast */
   qdevice(BUT31);  /* y: time reset */
   qdevice(BUT17);  /* d: debug flag */
   qdevice(BUT36);  /* n: star name flag */
   qdevice(BUT19);  /* z: zodiac flag */
   qdevice(BUT39);  /* i: moon orbit flag */
   qdevice(BUT40);  /* o: planet orbit flag */
   qdevice(BUT9 );  /* q: control panel */
   qdevice(BUT35);  /* b: single buffer mode */
   qdevice(BUT32);  /* u: user entry mode */

   qdevice(BUT157); /* printscreen: image dump flag */
   qdevice(BUT46);  /* -: time direction */
   qdevice(BUT73);  /* down arrow: decrease tesselation level */
   qdevice(BUT80);  /* up   arrow: increase tesselation level */
   qdevice(BUT50);  /* return */
   qdevice(BUT145); /* f1: sound */
   qdevice(BUT51);  /* .: for reading floats */
   qdevice(BUT44);  /* ,: for star naming */
   qdevice(BUT4);   /* right shift */
   qdevice(BUT5);   /* left  shift */
   qdevice(BUT2);   /* left  ctrl */
   qdevice(BUT144); /* right ctrl */

   qdevice(BUT45);  /* 0 */
   qdevice(BUT7);   /* 1 */
   qdevice(BUT13);  /* 2 */
   qdevice(BUT14);  /* 3 */
   qdevice(BUT21);  /* 4 */
   qdevice(BUT22);  /* 5 */
   qdevice(BUT29);  /* 6 */
   qdevice(BUT30);  /* 7 */
   qdevice(BUT37);  /* 8 */
   qdevice(BUT38);  /* 9 */

   qdevice(WINFREEZE) ;
   qdevice(WINTHAW) ;
#endif

#ifdef SP_OPEN_GL
   long mask;

   mask = FocusChangeMask | ExposureMask | ResizeRedirectMask | PointerMotionMask |
            KeyPressMask | KeyReleaseMask| ButtonPressMask | ButtonReleaseMask ;
   XSelectInput(Counter.winst.dpy,Counter.winst.win,mask);
   XFlush(Counter.winst.dpy);
#endif
}

/**********************************************************************
*  spReadEvents()                                                
**********************************************************************/
void spReadEvents(t_boss *flaggs)

{
#ifdef SP_IRIS_GL
   sint16   val ;
   sint32   v;

   while (qtest())  {
     v = qread(&val) ;

     if (v == WINFREEZE)  {
       while (qread(&val) != WINTHAW) ;

       if (Counter.flags & PANEL_FLAG)
         draw_menu() ;

       qreset() ;
       return ;
       }

     if (v == SP_IO_lsh || v == SP_IO_rsh)
       if (val)
         Counter.flags |= SHIFT_FLAG ;
       else Counter.flags &= ~SHIFT_FLAG ;

     if (v == SP_IO_lct || v == SP_IO_rct)
       if (val)
         Counter.flags |= CNTRL_FLAG ;
       else Counter.flags &= ~CNTRL_FLAG ;

     if (!val)
       key_press(flaggs,v);
     }

   Counter.mouse_x = getvaluator(MOUSEX) ;
   Counter.mouse_y = getvaluator(MOUSEY) ;

   Counter.mouse_b = 0;
   if (getbutton(LEFTMOUSE))
     Counter.mouse_b |= SP_LMOUSE;
   if (getbutton(MIDDLEMOUSE))
     Counter.mouse_b |= SP_MMOUSE;
   if (getbutton(RIGHTMOUSE))
     Counter.mouse_b |= SP_RMOUSE;
#endif

#ifdef SP_OPEN_GL
   int flag;
   XEvent event;
   KeySym keysym;
   char   buf[16];

   Counter.mouse_b = 0;

   while (XEventsQueued(Counter.winst.dpy,QueuedAfterReading)) {
     XNextEvent(Counter.winst.dpy, &event);

     switch (event.type) {
       case MotionNotify:
         Counter.mouse_x = Counter.winorigx + event.xbutton.x;
         Counter.mouse_y = Counter.winorigy + Counter.rotsizey - event.xbutton.y - 1;
         break;

       case ButtonPress:
         if (event.xbutton.button == Button1) {
           Counter.mouse_n |= (SP_LMOUSE<<4);
           Counter.mouse_b |= SP_LMOUSE;
           }
         if (event.xbutton.button == Button2) {
           Counter.mouse_n |= (SP_MMOUSE<<4);
           Counter.mouse_b |= SP_MMOUSE;
           }
         if (event.xbutton.button == Button3) {
           Counter.mouse_n |= (SP_RMOUSE<<4);
           Counter.mouse_b |= SP_RMOUSE;
           }
         break;

       case ButtonRelease:
         if (event.xbutton.button == Button1)
           Counter.mouse_n &= ~(SP_LMOUSE<<4);
         if (event.xbutton.button == Button2)
           Counter.mouse_n &= ~(SP_MMOUSE<<4);
         if (event.xbutton.button == Button3)
           Counter.mouse_n &= ~(SP_RMOUSE<<4);
         break;

       case KeyPress:
         flag = XLookupString((XKeyEvent *)&event, buf, 16, &keysym, 0);

         if (keysym == 65505 || keysym == 65506) {
           if (Counter.flags & SHIFT_FLAG)
             Counter.flags &= ~SHIFT_FLAG ;
           else Counter.flags |= SHIFT_FLAG ;
           }
         else if (keysym == 65507 || keysym == 65508) {
           if (Counter.flags & CNTRL_FLAG)
             Counter.flags &= ~CNTRL_FLAG ;
           else Counter.flags |= CNTRL_FLAG ;
           }
         else if (keysym == 65362)
           key_press(flaggs,SP_IO_up);
         else if (keysym == 65364) 
           key_press(flaggs,SP_IO_dwn);
         else if (keysym == 65377)
           key_press(flaggs,SP_IO_pri);

         if (flag)
           key_press(flaggs,buf[0]);
         break;

       case DestroyNotify:
         exit(1);
         break;

       case Expose:
       case ConfigureNotify:
       case KeyRelease:
       default:
         break;
       }
     }

   if (Counter.mouse_n & (SP_LMOUSE<<4))
     Counter.mouse_b |= SP_LMOUSE;
   if (Counter.mouse_n & (SP_MMOUSE<<4))
     Counter.mouse_b |= SP_MMOUSE;
   if (Counter.mouse_n & (SP_RMOUSE<<4))
     Counter.mouse_b |= SP_RMOUSE;
#endif
}

/**********************************************************************
*  spInitSpaceball()                                                
**********************************************************************/
void spInitSpaceball(void)

{
#ifdef SP_IRIS_GL
   qdevice(SBPICK) ;
   qdevice(SBBUT1) ;
   qdevice(SBBUT2) ;
   qdevice(SBBUT3) ;
   qdevice(SBBUT4) ;
   qdevice(SBBUT5) ;
   qdevice(SBBUT6) ;
   qdevice(SBBUT7) ;
   qdevice(SBBUT8) ;

   qdevice(SBTX) ;
   qdevice(SBTY) ;
   qdevice(SBTZ) ;
   qdevice(SBRX) ;
   qdevice(SBRY) ;
   qdevice(SBRZ) ;
   qdevice(SBPERIOD) ;
#endif

#ifdef SP_OPEN_GL

#endif
}

/**********************************************************************
*  spReadSpaceball()                                                
**********************************************************************/
void spReadSpaceball(t_boss *flaggs)

{

}

/**********************************************************************
*  spInitBell()                                                
**********************************************************************/
void spInitBell(void)

{
#ifdef SP_IRIS_GL
   setbell(1);
#endif

#ifdef SP_OPEN_GL
   XKeyboardControl kbc;

   kbc.bell_percent  = 100;
   kbc.bell_pitch    =  -1;
   kbc.bell_duration =  -1;
   XChangeKeyboardControl(Counter.winst.dpy,KBBellPercent|KBBellPitch|KBBellDuration,&kbc);
#endif
}

/**********************************************************************
*  spRingBell()                                                
**********************************************************************/
void spRingBell(void)

{
#ifdef SP_IRIS_GL
   ringbell();
#endif

#ifdef SP_OPEN_GL
   XBell(Counter.winst.dpy,100);
#endif
}

/**********************************************************************
*  spWaitForLeftButton()                                                
**********************************************************************/
void spWaitForLeftButton(void)

{
#ifdef SP_IRIS_GL
   while (1)
     if (getbutton(LEFTMOUSE))  {
       Counter.mouse_x = getvaluator(MOUSEX);
       Counter.mouse_y = getvaluator(MOUSEY);
       return;
       }
#endif

#ifdef SP_OPEN_GL
   XEvent event;
   uint32 flag = 0;

   while (1)
     if (XEventsQueued(Counter.winst.dpy,QueuedAfterReading)) {
       XNextEvent(Counter.winst.dpy, &event);

       switch (event.type) {
         case MotionNotify:
           Counter.mouse_x = Counter.winorigx + event.xbutton.x;
           Counter.mouse_y = Counter.winorigy + Counter.rotsizey - event.xbutton.y - 1;
           flag |= 2;
           break;

         case ButtonPress:
           if (event.xbutton.button == Button1)
             flag |= 1;
           break;
  
         default:
           break;
         }
  
       if (flag == 3)
         return;
       }
#endif
}

/**********************************************************************
*  spSwapBuffers()                                                
**********************************************************************/
void spSwapBuffers(void)

{
#ifdef SP_IRIS_GL
   swapbuffers() ;
#endif

#ifdef SP_OPEN_GL
   glXSwapBuffers(Counter.winst.dpy, Counter.winst.win);
#endif
}

/********************************************************************** 
*  spIdentifyMachine()  -  
**********************************************************************/
void spIdentifyMachine(void)

{
#ifdef SP_IRIS_GL
   char ghw[64];

   gversion(ghw);

   if (strncmp("GL4DRE",ghw,6) == 0)
     Counter.winst.hwid = SP_HW_RE;
   else if (strncmp("GL4DVGX",ghw,7) == 0)
     Counter.winst.hwid = SP_HW_VGX;
   else if (strncmp("GL4DGT", ghw,6) == 0)
     Counter.winst.hwid = SP_HW_GT;
   else if (strncmp("GL4DLG", ghw,6) == 0)
     Counter.winst.hwid = SP_HW_LG;
   else if (strncmp("GL4DXG", ghw,6) == 0)
     Counter.winst.hwid = SP_HW_XG;
   else if (strncmp("GL4DPI", ghw,6) == 0)
     Counter.winst.hwid = SP_HW_PI;
   else if (strncmp("GL4DG", ghw,5) == 0)
     Counter.winst.hwid = SP_HW_G;
   else
     Counter.winst.hwid = SP_HW_UNKNOWN;
#endif

#ifdef SP_OPEN_GL
   Counter.winst.hwid = SP_HW_RE;
#endif
}

/********************************************************************** 
*  spReadFloat()  -  
**********************************************************************/
flot32 spReadFloat(void)

{
#ifdef SP_IRIS_GL
   sint32 v,i=0,flag=0;
   flot32 r,f=1.0,num;
   sint16 val;

   qreset() ;
   num = 0.0;

   while (1)  {
     v = qread(&val) ;

     if (!val)  {
       r = -1 ;

       switch(v)  {
         case SP_IO_0  : r = 0 ; break ;
         case SP_IO_1  : r = 1 ; break ;
         case SP_IO_2  : r = 2 ; break ;
         case SP_IO_3  : r = 3 ; break ;
         case SP_IO_4  : r = 4 ; break ;
         case SP_IO_5  : r = 5 ; break ;
         case SP_IO_6  : r = 6 ; break ;
         case SP_IO_7  : r = 7 ; break ;
         case SP_IO_8  : r = 8 ; break ;
         case SP_IO_9  : r = 9 ; break ;
         case SP_IO_pnt: flag = 1; break ;
         case SP_IO_ret: return(num*f); break;
         default       : break ;
         }

       if (r >= 0)  {
         if (flag)
           f *= 0.1;
         num = 10*num + r ;
         }
       }
     }
#endif

#ifdef SP_OPEN_GL
   XEvent event;
   KeySym keysym;
   char   buf[16];
   sint32 v,i=0,flag=0;
   flot32 r,f=1.0,num=0.0;

   while (1) {
     XEventsQueued(Counter.winst.dpy,QueuedAfterReading);
     XNextEvent(Counter.winst.dpy, &event);

     if (event.type == KeyPress) {
       if (XLookupString((XKeyEvent *)&event, buf, 16, &keysym, 0))  {
         r = -1 ;
         switch(buf[0])  {
           case SP_IO_0  : r = 0 ; break ;
           case SP_IO_1  : r = 1 ; break ;
           case SP_IO_2  : r = 2 ; break ;
           case SP_IO_3  : r = 3 ; break ;
           case SP_IO_4  : r = 4 ; break ;
           case SP_IO_5  : r = 5 ; break ;
           case SP_IO_6  : r = 6 ; break ;
           case SP_IO_7  : r = 7 ; break ;
           case SP_IO_8  : r = 8 ; break ;
           case SP_IO_9  : r = 9 ; break ;
           case SP_IO_pnt: flag = 1; break ;
           case SP_IO_ret: return(num*f); break;
           default       : break ;
           }

         if (r >= 0)  {
           if (flag)
             f *= 0.1;
           num = 10*num + r ;
           }
         }
       }
     }
#endif
}

/********************************************************************** 
*  spReadStar()  -  
**********************************************************************/
sint32 spReadStar(sint32 n[3])

{
#ifdef SP_IRIS_GL
     sint32 v,i=0,r ;
   sint16 val ;

   qreset() ;
   n[0] = n[1] = n[2] = 0 ;

   while (1)  {
     v = qread(&val) ;

     if (!val)  {
       r = -1 ;

       switch(v)  {
         case SP_IO_0  : r = 0 ; break ;
         case SP_IO_1  : r = 1 ; break ;
         case SP_IO_2  : r = 2 ; break ;
         case SP_IO_3  : r = 3 ; break ;
         case SP_IO_4  : r = 4 ; break ;
         case SP_IO_5  : r = 5 ; break ;
         case SP_IO_6  : r = 6 ; break ;
         case SP_IO_7  : r = 7 ; break ;
         case SP_IO_8  : r = 8 ; break ;
         case SP_IO_9  : r = 9 ; break ;
         case SP_IO_com: i++   ; break ;
         case SP_IO_tid: i++   ; break ;

         case SP_IO_ret: if (i < 2)  {
                           n[1] = SOL_X_GRID ; 
                           n[2] = SOL_Z_GRID ;
                           }
                         else if (i > 2)
                           n[0] = n[1] = n[2] = -1 ;
                   
                         if (n[0] < 0 || n[1] < 0 || n[1] >= STARSQ || n[2] < 0 || n[2] >= STARSQ)
                           return(0) ;

                         return(1) ;
                         break ;

         default       : break ;
         }

       if (r >= 0)
         n[i] = 10*n[i] + r ;
       }
     }
#endif

#ifdef SP_OPEN_GL
   XEvent event;
   KeySym keysym;
   char   buf[16];
   sint32 i=0,r;

   n[0] = n[1] = n[2] = 0 ;

   while (1) {
     XEventsQueued(Counter.winst.dpy,QueuedAfterReading);
     XNextEvent(Counter.winst.dpy, &event);

     if (event.type == KeyPress) {
       if (XLookupString((XKeyEvent *)&event, buf, 16, &keysym, 0))  {
         r = -1 ;
         switch(buf[0])  {
           case SP_IO_0  : r = 0 ; break ;
           case SP_IO_1  : r = 1 ; break ;
           case SP_IO_2  : r = 2 ; break ;
           case SP_IO_3  : r = 3 ; break ;
           case SP_IO_4  : r = 4 ; break ;
           case SP_IO_5  : r = 5 ; break ;
           case SP_IO_6  : r = 6 ; break ;
           case SP_IO_7  : r = 7 ; break ;
           case SP_IO_8  : r = 8 ; break ;
           case SP_IO_9  : r = 9 ; break ;
           case SP_IO_com: i++   ; break ;
           case SP_IO_tid: i++   ; break ;
    
           case SP_IO_ret: if (i < 2)  {
                             n[1] = SOL_X_GRID ;
                             n[2] = SOL_Z_GRID ;
                             }
                           else if (i > 2)
                             n[0] = n[1] = n[2] = -1 ;

                           if (n[0] < 0 || n[1] < 0 || n[1] >= STARSQ || n[2] < 0 || n[2] >= STARSQ)
                             return(0) ;

                           return(1) ;
                           break ;

           default       : break ;
           }

         if (r >= 0)
           n[i] = 10*n[i] + r ;
         }
       }
     }
#endif
}

