/*
 * flight.h - the flight of the F-18 demo: the airplane of the player, the
 * views, the enemies and the display of the flight
 */
#ifndef FLIGHT_H
#define FLIGHT_H

/* the mouse, in window coordinates (y down), set by the program */
typedef struct F18Mouse {
    int x, y;               /* position */
    int w, h;               /* size of the window */
    int left, middle, right;
    int faster, slower;     /* the speed keys (A and Z) are held */
    int wheel;              /* wheel steps since the last frame */
    int up, down, kleft, kright; /* the arrow keys are held (flight) */
    int no_steer;           /* the mouse does not fly the airplane (M) */
    int inside;             /* the pointer was seen in the window */
} F18Mouse;

extern F18Mouse f18_mouse;

/* registers the functions of the flight for the scripts (initFlight,
 * eyeFlight, watchEnemy, toggleWingman, restartFlight...) */
void flight_register(void);

#endif
