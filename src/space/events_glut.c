/*
 * Copyright (C) 1992, 1993, 1994, Silicon Graphics, Inc.
 * All Rights Reserved.
 * 
 * Modified for GLUT compatibility
 */
#include "space.h"


#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <GL/glut.h>
extern t_stopwatch Counter ;

/* Global variables for GLUT callbacks */
static t_boss *g_flaggs = NULL;
static int g_window_width = 512;
static int g_window_height = 512;

/**********************************************************************
*  spOpenWindow()                                                
**********************************************************************/
uint32 spOpenWindow(void)
{
   int argc = 1;
   char *argv[] = {"space", NULL};
   
   /* Initialize GLUT */
   glutInit(&argc, argv);
   
   /* Set display mode */
   glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
   
   /* Set window size and position */
   if (Counter.flags & FLSCR_FLAG) {
     Counter.rotsizex = glutGet(GLUT_SCREEN_WIDTH);
     Counter.rotsizey = glutGet(GLUT_SCREEN_HEIGHT);
     glutInitWindowPosition(0, 0);
     glutInitWindowSize(Counter.rotsizex, Counter.rotsizey);
   } else {
     Counter.rotsizex = glutGet(GLUT_SCREEN_WIDTH);
     Counter.rotsizey = glutGet(GLUT_SCREEN_HEIGHT);
     glutInitWindowPosition(100, 100);
     glutInitWindowSize(512, 512);
     g_window_width = 512;
     g_window_height = 512;
   }
   
   /* Create window */
   Counter.winst.win = glutCreateWindow("Space");
   
   if (Counter.winst.win == 0) {
     fprintf(stderr, "ERROR: glutCreateWindow() failed.\n");
     return 0;
   }
   
   /* Fullscreen mode */
   if (Counter.flags & FLSCR_FLAG) {
     glutFullScreen();
   }
   
   return 1;
}

/**********************************************************************
*  spGetWindowGeometry()                                                
**********************************************************************/
void spGetWindowGeometry(void)
{
   Counter.winsizex = glutGet(GLUT_WINDOW_WIDTH);
   Counter.winsizey = glutGet(GLUT_WINDOW_HEIGHT);
   Counter.winorigx = glutGet(GLUT_WINDOW_X);
   Counter.winorigy = glutGet(GLUT_WINDOW_Y);
}

/**********************************************************************
*  spInitFont()                                                
**********************************************************************/
uint32 spInitFont(char *fn)
{
   /* GLUT handles fonts differently - use built-in fonts */
   /* Font name is ignored, we'll use GLUT bitmap fonts */
   Counter.fontbase = 0;
   return 1;
}

/**********************************************************************
*  spDrawString()                                                
**********************************************************************/
void spDrawString(float x, float y, float z, char *ch)
{
   int i;
   
   glRasterPos3f(x, y, z);
   
   /* Use GLUT bitmap font */
   for (i = 0; ch[i] != '\0'; i++) {
     glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, ch[i]);
   }
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
   
   /* Set cursor to center of window */
   spGetWindowGeometry();
   glutWarpPointer(Counter.winsizex / 2, Counter.winsizey / 2);
   
   /* Hide cursor for fullscreen mode */
   if (Counter.flags & FLSCR_FLAG) {
     glutSetCursor(GLUT_CURSOR_NONE);
   }
}

/**********************************************************************
*  GLUT Keyboard Callback
**********************************************************************/
static void keyboard_callback(unsigned char key, int x, int y)
{
   if (!g_flaggs) return;
   
   /* Handle modifier keys */
   int modifiers = glutGetModifiers();
   if (modifiers & GLUT_ACTIVE_SHIFT) {
     Counter.flags |= SHIFT_FLAG;
   } else {
     Counter.flags &= ~SHIFT_FLAG;
   }
   
   if (modifiers & GLUT_ACTIVE_CTRL) {
     Counter.flags |= CNTRL_FLAG;
   } else {
     Counter.flags &= ~CNTRL_FLAG;
   }
   
   /* Process key */
   key_press(g_flaggs, key);
}

/**********************************************************************
*  GLUT Special Key Callback
**********************************************************************/
static void special_callback(int key, int x, int y)
{
   if (!g_flaggs) return;
   
   switch(key) {
     case GLUT_KEY_UP:
       key_press(g_flaggs, SP_IO_up);
       break;
     case GLUT_KEY_DOWN:
       key_press(g_flaggs, SP_IO_dwn);
       break;
     case GLUT_KEY_F1:
       key_press(g_flaggs, SP_IO_snd);
       break;
   }
}

/**********************************************************************
*  GLUT Mouse Button Callback
**********************************************************************/
static void mouse_callback(int button, int state, int x, int y)
{
   if (state == GLUT_DOWN) {
     switch(button) {
       case GLUT_LEFT_BUTTON:
         Counter.mouse_n |= (SP_LMOUSE << 4);
         Counter.mouse_b |= SP_LMOUSE;
         break;
       case GLUT_MIDDLE_BUTTON:
         Counter.mouse_n |= (SP_MMOUSE << 4);
         Counter.mouse_b |= SP_MMOUSE;
         break;
       case GLUT_RIGHT_BUTTON:
         Counter.mouse_n |= (SP_RMOUSE << 4);
         Counter.mouse_b |= SP_RMOUSE;
         break;
     }
   } else {
     switch(button) {
       case GLUT_LEFT_BUTTON:
         Counter.mouse_n &= ~(SP_LMOUSE << 4);
         break;
       case GLUT_MIDDLE_BUTTON:
         Counter.mouse_n &= ~(SP_MMOUSE << 4);
         break;
       case GLUT_RIGHT_BUTTON:
         Counter.mouse_n &= ~(SP_RMOUSE << 4);
         break;
     }
   }
   
   Counter.mouse_x = Counter.winorigx + x;
   Counter.mouse_y = Counter.winorigy + Counter.winsizey - y - 1;
}

/**********************************************************************
*  GLUT Mouse Motion Callback
**********************************************************************/
static void motion_callback(int x, int y)
{
   Counter.mouse_x = Counter.winorigx + x;
   Counter.mouse_y = Counter.winorigy + Counter.winsizey - y - 1;
}

/**********************************************************************
*  GLUT Passive Motion Callback (mouse movement without buttons)
**********************************************************************/
static void passive_motion_callback(int x, int y)
{
   Counter.mouse_x = Counter.winorigx + x;
   Counter.mouse_y = Counter.winorigy + Counter.winsizey - y - 1;
}

/**********************************************************************
*  spInitKeyboard()                                                
**********************************************************************/
void spInitKeyboard(void)
{
   /* Register GLUT callbacks */
   glutKeyboardFunc(keyboard_callback);
   glutSpecialFunc(special_callback);
   glutMouseFunc(mouse_callback);
   glutMotionFunc(motion_callback);
   glutPassiveMotionFunc(passive_motion_callback);
}

/**********************************************************************
*  spReadEvents()                                                
**********************************************************************/
void spReadEvents(t_boss *flaggs)
{
   g_flaggs = flaggs;
   
   /* Update mouse button state */
   Counter.mouse_b = 0;
   if (Counter.mouse_n & (SP_LMOUSE << 4))
     Counter.mouse_b |= SP_LMOUSE;
   if (Counter.mouse_n & (SP_MMOUSE << 4))
     Counter.mouse_b |= SP_MMOUSE;
   if (Counter.mouse_n & (SP_RMOUSE << 4))
     Counter.mouse_b |= SP_RMOUSE;
   
   /* Note: In standard GLUT, we can't process events manually */
   /* Events are processed automatically in glutMainLoop() */
   /* This function is kept for compatibility but does minimal work */
}

/**********************************************************************
*  spInitSpaceball()                                                
**********************************************************************/
void spInitSpaceball(void)
{
   /* Spaceball not supported in GLUT */
}

/**********************************************************************
*  spReadSpaceball()                                                
**********************************************************************/
void spReadSpaceball(t_boss *flaggs)
{
   /* Spaceball not supported in GLUT */
}

/**********************************************************************
*  spInitBell()                                                
**********************************************************************/
void spInitBell(void)
{
   /* Bell not available in GLUT - no action needed */
}

/**********************************************************************
*  spRingBell()                                                
**********************************************************************/
void spRingBell(void)
{
   /* Bell not available in GLUT - could use system beep */
#ifdef _WIN32
   /* Use Windows beep */
   MessageBeep(MB_OK);
#endif
}

/**********************************************************************
*  spWaitForLeftButton()                                                
**********************************************************************/
void spWaitForLeftButton(void)
{
   /* In GLUT, we need to use a busy-wait loop */
   /* This is not ideal but necessary without FreeGLUT extensions */
   while (!(Counter.mouse_b & SP_LMOUSE)) {
     /* Small delay to avoid busy waiting */
#ifdef _WIN32
     Sleep(10);
#else
     usleep(10000);
#endif
   }
}

/**********************************************************************
*  spSwapBuffers()                                                
**********************************************************************/
void spSwapBuffers(void)
{
   glutSwapBuffers();
}

/********************************************************************** 
*  spIdentifyMachine()  -  
**********************************************************************/
void spIdentifyMachine(void)
{
   /* For GLUT, just set a generic hardware ID */
   Counter.winst.hwid = SP_HW_RE;
}

/********************************************************************** 
*  spReadFloat()  -  
**********************************************************************/
flot32 spReadFloat(void)
{
   /* This function reads a float from keyboard input */
   /* In GLUT, we need to handle this through the keyboard callback */
   /* For now, return 0.0 - this would need a more complex implementation */
   fprintf(stderr, "spReadFloat() not fully implemented in GLUT version\n");
   return 0.0;
}

/********************************************************************** 
*  spReadStar()  -  
**********************************************************************/
sint32 spReadStar(sint32 n[3])
{
   /* This function reads star coordinates from keyboard */
   /* In GLUT, we need to handle this through the keyboard callback */
   /* For now, return default values */
   n[0] = 0;
   n[1] = SOL_X_GRID;
   n[2] = SOL_Z_GRID;
   
   fprintf(stderr, "spReadStar() not fully implemented in GLUT version\n");
   return 1;
}


