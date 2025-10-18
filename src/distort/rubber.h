/*
	rubber.h
	Drew Olbrich, 1992
*/

#ifndef _RUBBER
#define _RUBBER

void rubber_init();
void rubber_dynamics(int mousex, int mousey);
void rubber_redraw();
void rubber_click(int mousex, int mousey, int state);

#endif
