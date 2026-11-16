/*
 * Copyright 1993, 1995, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that (i) the above copyright notices and this
 * permission notice appear in all copies of the software and related
 * documentation, and (ii) the name of Silicon Graphics may not be
 * used in any advertising or publicity relating to the software
 * without the specific, prior written permission of Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SILICON GRAPHICS BE LIABLE FOR ANY SPECIAL,
 * INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY
 * THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 */


/*
 *	main.c
*/

#include <stdlib.h>
#include <stdio.h>

/* proceess control includes */
#include <sys/statfs.h>
#include <sys/types.h>


/* Performer include files */
#include <Performer/pf.h>
#include <Performer/pfutil.h>
#include <Performer/pfui.h>

/* Common Code include file */
#include "generic.h"

/* 'custom.h' is provided by the individual applications. It must contain
    the typedef for the ViewState structure
*/
#include "custom.h"


/**************************************************************************
 *									  *
 *		    APPLICATION PROCESS ROUTINES			  *
 *									  * 
 **************************************************************************/


/* frees any restricted/isolated CPUs when app starts up */
int FreeInitCPUs = 1;
int AppCPU=-1, CullCPU=-1, DrawCPU=-1;

int
main(int argc, char *argv[])
{
    /* 
     * Initialize IRIS Performer: create shared memory, initialize clocks, etc.
     * This must be the first libpf call in a libpf application.
    */
    pfInit();

    /* 
     * Allocate and initialize global shared memory before pfConfig 
     * so that forked processes share global pointers. 
    */
    InitSharedMem(argc, argv);
    
    /* Set multiprocessing mode and other configuration values. */
    InitConfig();

    /* 
     * Configure IRIS Performer and fork extra processes if 
     * configured for multiprocessing. 
     */
    pfConfig();
   
    /* Initialize IRIS Performer utility libraries. */
    pfuInitUtil();
    pfiInit();

    /* Create the visual database. */
    InitScene();

    /* Initialize the rendering pipeline(s). */
    InitPipe();
    
    /* set off first frame of cull and draw processes for initialization 
     * in parallel with app
     */
    pfFrame();
    
    /* 
     * Assign a non-degrading priority to all Performer processes. 
     * User processes spawned from this one will inherit the same priority.
    */
    if (PrioritizeProcs)
	pfuPrioritizeProcs(TRUE);
       
    /* 
     * Initialize the graphical user interface (GUI). 
     * Do this before InitChannel so that the GUI is the first channel.
    */
    InitGUI();

    /* Initialize pfChannel(s). */
    InitChannel();

    /* Initialize input handling (X or GL) for mouse and event inputs. */
    pfuInitInput(pfGetChanPWin(ViewState->masterChan), ViewState->input);
 
    /* 
     * Set up processor locking for APP - must be done after pfConfig() 
     * and should be done after InitInput which forks off an input handling
     * process.
    */
    if (ViewState->procLock == ((uint)~0))
    {
	if (AppCPU > -1)
	    pfuLockDownProc(AppCPU);
	else
	    pfuLockDownApp();
	pfuRunDiskProcsOn(-1);
    }
    
    /* Set the desired frame rate. */
    pfFrameRate(ViewState->frameRate);	

    /* Set the MP synchronization phase. */
    pfPhase(ViewState->phase);		

    /* Application main loop */
    while (!SimDone())
    {
	/* Sleep until next frame */
	pfSync();
	
	/* 
         * Should do all latency critical processing inbetween pfSync()
	 * and pfFrame(). Typically this is viewing position update.
	 */
	PreFrame();

	/* Trigger cull and draw processing for this frame. */
        pfFrame();

	/* Perform non-latency critical simulation updates. */
	PostFrame();
    }
   
    /* 
     * Cleanup and exit.  
     */
    pfuFreeCPUs();
    pfuExitInput();
    pfuExitUtil();

    pfExit();

    return(0);
}


/**************************************************************************
 *									  *
 *		    APP PROCESS ROUTINES				  *
 *									  * 
 **************************************************************************/

void
AppFunc(pfChannel *chan, void *data)
{
    PreApp(chan, data);
    pfApp();
    PostApp(chan, data);
}


/**************************************************************************
 *									  *
 *		    CULL PROCESS ROUTINES				  *
 *									  * 
 **************************************************************************/

/*
 * The cull function callback.  Any work that needs to be done
 * in the cull process should happen in this function.
*/
void
CullFunc(pfChannel * chan, void *data)
{
    PreCull(chan, data);

    pfCull();			/* Cull to the viewing frustum */

    PostCull(chan, data);
}


/**************************************************************************
 *									  *
 *		    DRAW PROCESS ROUTINES				  *
 *									  * 
 **************************************************************************/

/*
 * The draw function callback.  I/O with GL devices must happen here.
 * Any GL functionality outside Performer must be done here. 
 * All libpr routines which may be captured by a pfDispList, 
 * e.g.- pfEnable, pfApply*, pfCullFace, pfPushMatrix, should be called 
 * here since they send GL commands to the graphics hardware.
*/
void
DrawFunc(pfChannel *chan, void *data)
{
    PreDraw(chan, data);	/* e.g. - clear the viewport */

    pfDraw();			/* Render the frame */

    PostDraw(chan, data);	/* e.g. - draw HUD, read GL devices */

#ifndef IRISGL
   {
   int err;
   if ((err = glGetError()) != GL_NO_ERROR)
	pfNotify(PFNFY_NOTICE,PFNFY_USAGE,"OpenGL Error 0x%x - %s",err, gluErrorString(err));
   }
#endif /* GL Type */
}
