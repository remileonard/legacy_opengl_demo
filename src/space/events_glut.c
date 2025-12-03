/*
 * Copyright (C) 1992, 1993, 1994, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * Modified for GLUT compatibility
 */
#include "space.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <GL/glut.h>
extern t_stopwatch Counter;

/* Global variables for GLUT callbacks */
static t_boss *g_flaggs = NULL;
static int g_window_width = 512;
static int g_window_height = 512;

/* Input state for spReadFloat and spReadStar */
typedef enum { INPUT_MODE_NONE, INPUT_MODE_FLOAT, INPUT_MODE_STAR } InputMode;

static InputMode g_input_mode = INPUT_MODE_NONE;
static char g_input_buffer[64];
static int g_input_pos = 0;
static int g_input_complete = 0;
static flot32 g_float_result = 0.0f;
static sint32 g_star_result[3] = {0, 0, 0};

/* Modifier key states - must be tracked manually in GLUT */
static int g_shift_pressed = 0;
static int g_ctrl_pressed = 0;

/**********************************************************************
 *  spOpenWindow()
 **********************************************************************/
uint32 spOpenWindow(void) {
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
        // glutFullScreen();
    }

    return 1;
}

/**********************************************************************
 *  spGetWindowGeometry()
 **********************************************************************/
void spGetWindowGeometry(void) {
    Counter.winsizex = glutGet(GLUT_WINDOW_WIDTH);
    Counter.winsizey = glutGet(GLUT_WINDOW_HEIGHT);
    Counter.winorigx = glutGet(GLUT_WINDOW_X);
    Counter.winorigy = glutGet(GLUT_WINDOW_Y);
}

/**********************************************************************
 *  spInitFont()
 **********************************************************************/
uint32 spInitFont(char *fn) {
    /* GLUT handles fonts differently - use built-in fonts */
    /* Font name is ignored, we'll use GLUT bitmap fonts */
    Counter.fontbase = 0;
    return 1;
}

/**********************************************************************
 *  spDrawString()
 **********************************************************************/
void spDrawString(float x, float y, float z, char *ch) {
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
void spInitCursor(void) {
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
static void keyboard_callback(unsigned char key, int x, int y) {
    /* Handle special input modes first */
    if (g_input_mode == INPUT_MODE_FLOAT) {
        if (key >= '0' && key <= '9') {
            if (g_input_pos < 63) {
                g_input_buffer[g_input_pos++] = key;
                g_input_buffer[g_input_pos] = '\0';
            }
            return;
        } else if (key == '.' || key == SP_IO_pnt) {
            if (g_input_pos < 63) {
                g_input_buffer[g_input_pos++] = '.';
                g_input_buffer[g_input_pos] = '\0';
            }
            return;
        } else if (key == '\r' || key == '\n' || key == SP_IO_ret) {
            g_float_result = (flot32)atof(g_input_buffer);
            g_input_complete = 1;
            g_input_mode = INPUT_MODE_NONE;
            return;
        }
    } else if (g_input_mode == INPUT_MODE_STAR) {
        if (key >= '0' && key <= '9') {
            if (g_input_pos < 63) {
                g_input_buffer[g_input_pos++] = key;
                g_input_buffer[g_input_pos] = '\0';
            }
            return;
        } else if (key == ',' || key == SP_IO_com || key == '~' || key == SP_IO_tid) {
            if (g_input_pos < 63) {
                g_input_buffer[g_input_pos++] = ',';
                g_input_buffer[g_input_pos] = '\0';
            }
            return;
        } else if (key == '\r' || key == '\n' || key == SP_IO_ret) {
            /* Parse the input buffer for star coordinates */
            int parsed = sscanf(g_input_buffer, "%d,%d,%d", &g_star_result[0], &g_star_result[1], &g_star_result[2]);
            if (parsed < 2) {
                g_star_result[1] = SOL_X_GRID;
                g_star_result[2] = SOL_Z_GRID;
            } else if (parsed < 3) {
                g_star_result[2] = SOL_Z_GRID;
            }
            g_input_complete = 1;
            g_input_mode = INPUT_MODE_NONE;
            return;
        }
    }

    if (!g_flaggs)
        return;

    /* Update flags based on tracked modifier states and glutGetModifiers */
    int modifiers = glutGetModifiers();
    g_shift_pressed = (modifiers & GLUT_ACTIVE_SHIFT) ? 1 : 0;
    g_ctrl_pressed = (modifiers & GLUT_ACTIVE_CTRL) ? 1 : 0;

    if (g_ctrl_pressed) {
        Counter.flags |= CNTRL_FLAG;
    } else {
        Counter.flags &= ~CNTRL_FLAG;
    }

    /* Process key */
    key_press(g_flaggs, key);
}/**********************************************************************
 *  GLUT Keyboard Up Callback
 **********************************************************************/
static void keyboard_up_callback(unsigned char key, int x, int y) {
    /* Update modifier states when keys are released */
    int modifiers = glutGetModifiers();
    g_shift_pressed = (modifiers & GLUT_ACTIVE_SHIFT) ? 1 : 0;
    g_ctrl_pressed = (modifiers & GLUT_ACTIVE_CTRL) ? 1 : 0;


    if (g_ctrl_pressed) {
        Counter.flags |= CNTRL_FLAG;
    } else {
        Counter.flags &= ~CNTRL_FLAG;
    }
}

/**********************************************************************
 *  GLUT Special Key Callback
 **********************************************************************/
static void special_callback(int key, int x, int y) {
    if (!g_flaggs)
        return;

    /* Track modifier keys */
    int modifiers = glutGetModifiers();
    g_shift_pressed = (modifiers & GLUT_ACTIVE_SHIFT) ? 1 : 0;
    g_ctrl_pressed = (modifiers & GLUT_ACTIVE_CTRL) ? 1 : 0;

    if (g_shift_pressed) {
        if (! (Counter.flags & SHIFT_FLAG)) {
            Counter.flags |= SHIFT_FLAG;
            glutSetCursor(GLUT_CURSOR_INHERIT);
        } else {
            Counter.flags &= ~SHIFT_FLAG;
            glutSetCursor(GLUT_CURSOR_NONE);
        }   
    }

    if (g_ctrl_pressed) {
        if (! (Counter.flags & CNTRL_FLAG)) {
            Counter.flags |= CNTRL_FLAG;
        } else {
            Counter.flags &= ~CNTRL_FLAG;
        }   
    }

    switch (key) {
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
 *  GLUT Special Key Up Callback
 **********************************************************************/
static void special_up_callback(int key, int x, int y) {
    /* Update modifier states when special keys are released */
    int modifiers = glutGetModifiers();
    g_shift_pressed = (modifiers & GLUT_ACTIVE_SHIFT) ? 1 : 0;
    g_ctrl_pressed = (modifiers & GLUT_ACTIVE_CTRL) ? 1 : 0;

}/**********************************************************************
 *  GLUT Mouse Button Callback
 **********************************************************************/
static void mouse_callback(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        switch (button) {
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
        switch (button) {
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
static void motion_callback(int x, int y) {
    Counter.mouse_x = Counter.winorigx + x;
    Counter.mouse_y = Counter.winorigy + Counter.winsizey - y - 1;
}

/**********************************************************************
 *  GLUT Passive Motion Callback (mouse movement without buttons)
 **********************************************************************/
static void passive_motion_callback(int x, int y) {
    Counter.mouse_x = Counter.winorigx + x;
    Counter.mouse_y = Counter.winorigy + Counter.winsizey - y - 1;
}

/**********************************************************************
 *  GLUT Reshape Callback
 **********************************************************************/
static void reshape_callback(int width, int height) {
    g_window_width = width;
    g_window_height = height;
    
    /* Update window geometry in Counter */
    spGetWindowGeometry();
    Counter.winsizex = width;
    Counter.winsizey = height;
    /* Update viewport */
    glViewport(0, 0, width, height);
}

/**********************************************************************
 *  GLUT Visibility Callback
 **********************************************************************/
static void visibility_callback(int state) {
    /* Handle window iconification/deiconification */
    if (state == GLUT_VISIBLE) {
        /* Window is visible - resume rendering */
    } else {
        /* Window is not visible - could pause rendering */
    }
}

/**********************************************************************
 *  spInitKeyboard()
 **********************************************************************/
void spInitKeyboard(void) {
    /*   GLUT callbacks */
    glutKeyboardFunc(keyboard_callback);
    glutKeyboardUpFunc(keyboard_up_callback);
    glutSpecialFunc(special_callback);
    glutSpecialUpFunc(special_up_callback);
    glutMouseFunc(mouse_callback);
    glutMotionFunc(motion_callback);
    glutPassiveMotionFunc(passive_motion_callback);
    glutReshapeFunc(reshape_callback);
    glutVisibilityFunc(visibility_callback);

    /* FreeGLUT specific: Enable key repeat */
#ifdef GLUT_KEY_REPEAT_ON
    glutSetKeyRepeat(GLUT_KEY_REPEAT_ON);
#endif

    /* FreeGLUT specific: Close behavior */
#ifdef GLUT_ACTION_ON_WINDOW_CLOSE
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif
}

/**********************************************************************
 *  spReadEvents()
 **********************************************************************/
void spReadEvents(t_boss *flaggs) {
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
void spInitSpaceball(void) { /* Spaceball not supported in GLUT */ }

/**********************************************************************
 *  spReadSpaceball()
 **********************************************************************/
void spReadSpaceball(t_boss *flaggs) { /* Spaceball not supported in GLUT */ }

/**********************************************************************
 *  spInitBell()
 **********************************************************************/
void spInitBell(void) { /* Bell not available in GLUT - no action needed */ }

/**********************************************************************
 *  spRingBell()
 **********************************************************************/
void spRingBell(void) {
    /* Bell not available in GLUT - could use system beep */
#ifdef _WIN32
    /* Use Windows beep */
    MessageBeep(MB_OK);
#endif
}

/**********************************************************************
 *  spWaitForLeftButton()
 **********************************************************************/
void spWaitForLeftButton(void) {
    /* Clear the button state */
    Counter.mouse_b &= ~SP_LMOUSE;
    Counter.mouse_n &= ~(SP_LMOUSE << 4);

    /* Wait for left button press */
    while (!(Counter.mouse_b & SP_LMOUSE)) {
#ifdef FREEGLUT
        /* FreeGLUT: Process events manually and force redraw */
        glutPostRedisplay();
        glutMainLoopEvent();
#endif

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
void spSwapBuffers(void) { glutSwapBuffers(); }

/**********************************************************************
 *  spIdentifyMachine()  -
 **********************************************************************/
void spIdentifyMachine(void) {
    /* For GLUT, just set a generic hardware ID */
    Counter.winst.hwid = SP_HW_RE;
}

/**********************************************************************
 *  spReadFloat()  -
 **********************************************************************/
flot32 spReadFloat(void) {
    /* Initialize input mode */
    g_input_mode = INPUT_MODE_FLOAT;
    g_input_pos = 0;
    g_input_buffer[0] = '\0';
    g_input_complete = 0;
    g_float_result = 0.0f;

    /* Wait for input to complete */
    while (!g_input_complete) {
#ifdef FREEGLUT
        /* FreeGLUT: Process events manually and force redraw */
        glutPostRedisplay();
        glutMainLoopEvent();
#endif

#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
    }

    return g_float_result;
}

/**********************************************************************
 *  spReadStar()  -
 **********************************************************************/
sint32 spReadStar(sint32 n[3]) {
    /* Initialize input mode */
    g_input_mode = INPUT_MODE_STAR;
    g_input_pos = 0;
    g_input_buffer[0] = '\0';
    g_input_complete = 0;
    g_star_result[0] = 0;
    g_star_result[1] = SOL_X_GRID;
    g_star_result[2] = SOL_Z_GRID;

    /* Wait for input to complete */
    while (!g_input_complete) {
#ifdef FREEGLUT
        /* FreeGLUT: Process events manually and force redraw */
        glutPostRedisplay();
        glutMainLoopEvent();
#endif

#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
    }

    /* Copy results */
    n[0] = g_star_result[0];
    n[1] = g_star_result[1];
    n[2] = g_star_result[2];

    /* Validate coordinates */
    if (n[0] < 0 || n[1] < 0 || n[1] >= STARSQ || n[2] < 0 || n[2] >= STARSQ) {
        return 0;
    }

    return 1;
}
