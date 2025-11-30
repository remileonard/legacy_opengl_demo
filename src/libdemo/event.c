/*
 * Copyright 1991, 1992, 1993, 1994, Silicon Graphics, Inc.
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
/*
 *	event.c
 *	A more rational way to handle reading the event queue
 * Written by Wade Olsen
 */

#include <stdio.h>
#include "porting/iris2ogl.h"
#include "event.h"
#ifdef spaceball
#include <gl/spaceball.h>
#endif

int context, state, device;

typedef struct event_s
{
    int		window, device, state;
	void (*func)(void *, int);
	void *arg;
    struct event_s	*next;
} event_t, *event_p;

typedef struct update_s
{
    int *flag;
	void (*ufunc)(void *);
	void *arg;
    struct update_s *next;
} update_t, *update_p;


static event_p	    event_list;
static update_p    update_list;

/*
 * This routine adds an event to be checked for to the event queue.
 * window should be the wid of the window to respond in, or ANY if
 * this event applies to all windows.  device is the device, and state
 * is the device's value (e.g.  ANY, UP, DOWN, the window id (for
 * REDRAW), etc).  Func is the function that will be called, with
 * arguments 'arg' and the device's value.
 * 
 * NOTE:  the device must be queued for it to be found by the event()
 * routine-- add_event DOES NOT qdevice(device).
 */
void
add_event(int window, int device, int state, 
	void (*func)(void *, int), void *arg)
{
    event_p new_guy;

	new_guy = (event_p)malloc(sizeof(event_t));
    new_guy->window = window;
    new_guy->device = device;
    new_guy->state  = state;
    new_guy->func   = func;
    new_guy->arg    = arg;
    new_guy->next   = event_list;
    event_list = new_guy;
}

/*
 *	Specify a function to be called if there is nothing in the queue
 * and (*flag) is non-zero.  If no update function is active, or
 * (*flag) is 0, then event() will block on a qread.  If there is an
 * active update function, event() will continuously call the update
 * function, hogging the cpu.
 */
void
add_update(int *flag, void (*ufunc)(void *), void *arg)
{
    update_p	new_guy;

    new_guy = (update_p)malloc(sizeof(update_t));
    new_guy->flag = flag;
    new_guy->ufunc = ufunc;
	new_guy->arg  = arg;
    new_guy->next = update_list;
    update_list = new_guy;
}

void find_event(void);
int do_updates(void);
void event_inputchange(void);

static void
initialize()
{
    static int initialized = 0;

    if (initialized == 0)
    {
	    add_event(ANY, INPUTCHANGE, ANY, event_inputchange, NULL);
	    qdevice(INPUTCHANGE);
	    initialized = 1;
    }
}


/*
 * The main Event.  Call this repeatedly to automagically handle
 * reading the queue, and calling your functions to handle what
 * appears there.
 */
void
event()
{
    initialize();

	/* Something in the queue?  Handle it */
    if (qtest()) {
		while (qtest()) {
			find_event();
		}
	} else if (do_updates() == 0) {
		find_event();	
	}
	
}

/*
 * Similar to event, but does not block if there are no events
 */
void
event_noblock()
{
    initialize();

	/* Something in the queue?  Handle it */
    while (qtest())
	find_event();

	/*
	 * Or, if there's no update function, wait for something to appear
	 */
    do_updates();
}

static int
do_updates()
{
    update_p	scan;
    int		updated = 0;
    for (scan = update_list; scan; scan = scan->next)
	{
		if (*scan->flag)
		{
		    (*scan->ufunc)(scan->arg);
		    updated = 1;
		}
	}

    return(updated);
}

static void
event_inputchange()
{
	context = state;
#ifdef spaceball
	sbEvent(INPUTCHANGE, state);
#endif
}

void
find_event()
{
    int flag;
    event_p scan;
    short s;

    device = qread(&s);
    state = s;
    for (scan = event_list, flag=0; scan; scan = scan->next)
	{
		if ( ((scan->window == ANY) || (context == scan->window))
			&& ((scan->device == ANY) ||(device == scan->device))
			&& ((scan->state == ANY) || (state == scan->state)))
		{
		    (*scan->func)(scan->arg, state);
			flag = 1;
		}
	}
#ifdef spaceball
	if (flag == 0)
		sbEvent(device, s);
#endif
}

/*
 * Delete event handlers
 */

void
delete_events(int context, int device, int state, void *func)
{
    event_p scan, prev;

    prev = 0;
    scan = event_list; 
    while (scan)
	{
		if (	   ((context == ANY) || (context == scan->window))
			&& ((device == ANY) || (device == scan->device))
			&& ((state == ANY) || (state == scan->state))
			&& ((func == (void *)ANY) || (func == scan->func)))
		{
		    event_p nuke;

		    nuke = scan;
		    
		    if (prev)
			scan = prev->next = scan->next;
		    else
			scan = event_list = scan->next;

		    free(nuke);
		}
		else
		{
		    prev = scan;
		    scan = scan->next;
		}
	}
}


void
delete_updates(int * flag, void * func)
{
    update_p	scan, prev;

    prev = 0;
    scan = update_list;
    while (scan)
	{
		if (	((flag == scan->flag) || (flag == (int *)ANY))
		     && ((func == scan->ufunc) || (func == (void *)ANY)))
		{
		    update_p nuke;

		    nuke = scan;

		    if (prev)
			scan = prev->next = scan->next;
		    else
			scan = update_list = scan->next;

		    free(nuke);
		}
		else
		{
		    prev = scan;
		    scan = scan->next;
		}
	}
}


/*
 * Misc. event handlers
 */

void setInt(void * i)
{
    *(int *)i = 1;
}

void resetInt(void * i)
{
    *(int *)i = 0;
}

void setIntToVal(void * i, int v)
{
    *(int *)i = v;
}

