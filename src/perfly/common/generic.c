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
 *	generic.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <sys/sysmp.h>

/* X Window includes */
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef IRISGL
/* GL includes */
#include <gl/glws.h>
#endif /* GL type */

/* IRIS Performer includes */
#include <Performer/pf.h>
#include <Performer/pfutil.h>
#include <Performer/pfdu.h>
#include <Performer/pfui.h>

/* common includes */
#include "main.h"
#include "custom.h"

/******************************************************************************
 *			    Global Variables
 *****************************************************************************/

/* Data shared between processes */
SharedViewState	*ViewState;

/*
 * Command line optional settings for initialization. These variables are
 * set before the call to pfConfig and are only used for initialization.
 * So they don't need to be in the ViewState shared memory structure. They
 * are initialized in the function InitSharedMem().
*/
int	WinSizeX		= -1;
int	WinSizeY		= -1;
int	Multipipe		= -1;
int	NumPipes		= -1;
int	NumChans		= -1;
int	NotifyLevel		= PFNFY_INFO;
int	ProcSplit		= PFMP_DEFAULT;
int	PrioritizeProcs		= 0;
int	GangDraw		= 0;
int	InitHPR			= FALSE;
int	InitXYZ			= FALSE;
char*	WriteFile		= NULL;
int	ChanOrder[MAX_CHANS]	= {-1};
int	InitFOV			= FALSE;

/*********************************************************************
*								     *
*		APPLICATION PROCESS ROUTINES			     *
*								     *
**********************************************************************/

/* The following globals are used by the Application process only */
int	    NumFiles	    = 0;
char	    **DatabaseFiles = NULL;

static int	numScreens;


/*---------------------------------------------------------------------*/


/* Determine if Simulation is finished */
int
SimDone(void)
{
    return(ViewState->exitFlag);
}


/* Initialize MP, system config */
void
InitConfig(void)
{
    int 	numCPUs = 0;
    int 	availCPUs;

    /* Set multiprocessing mode. */
    pfMultiprocess(ProcSplit);

    /* Set number of pfPipes desired. */
    pfMultipipe(NumPipes);

    /* Enable hyperpipe mode */
    if (Multipipe == HYPERPIPE)
    	pfHyperpipe(NumPipes);

    /* Set intersection callback. */
    pfIsectFunc(IsectFunc);

    /* Check for restricted CPUs */
    availCPUs = sysmp(MP_NAPROCS);
    numCPUs = sysmp(MP_NPROCS);

    if (numCPUs != availCPUs)
    {
	pfNotify(PFNFY_NOTICE, PFNFY_RESOURCE, 
	    "Only %d out of %d processors available on this machine !!!",
			availCPUs, numCPUs);

	if ((geteuid() == 0) && FreeInitCPUs)
	    pfuFreeCPUs();
	else
	    pfNotify(PFNFY_NOTICE, PFNFY_PRINT, "Not freeing CPUs.");
    } 
    else
	pfNotify(PFNFY_INFO, PFNFY_PRINT, 
	    "All %d processors available on this machine.", numCPUs);
}


/*
 * Allocate Global pointers and set Global variables-
 * This must be called before pfConfig() if forked processes
 * must share globals and/or pointers to shared memory.
 * Initialize shared memory structures
 * 
 * This queries X for machine config info and currently assumes the 
 * default display and screen.
 * 
 * XXX we really need to do config info after we have the desired gl context
 * so that we can do the necessary OpenGL queries on extensions.
*/
void
InitSharedMem(int argc, char *argv[])
{
    void 	*arena;
    int		i;
    int 	maxScreenX, maxScreenY;
    Display	*dsp;
    int		screen; 

    /* Get pointer to shared memory malloc arena created by pfInit()*/
    arena = pfGetSharedArena();
    dsp = pfGetCurWSConnection(); /* get display for machine config queries */
    screen = DefaultScreen( dsp );
    /*
     * Malloc structures now, before pfConfig(), so forked processes have same
     * pointers.
    */
    ViewState = (SharedViewState *) pfCalloc(1, sizeof(SharedViewState), arena);

    /* Find out how big the window can be */
    maxScreenX = DisplayWidth(dsp, screen);
    maxScreenY = DisplayHeight(dsp, screen);
    /* get number of screens */
    numScreens = ScreenCount(dsp);

    /*
     * Set window size - this is set here, before the call to pfConfig,
     * so that all processes will get the same copy. These do not need
     * to be in shared memory, because they are initialization values
     * which are not changed during the course of the application
    */
    if ((WinSizeX < 0) || (WinSizeY < 0))
    {	
	/* If no window size specified, set to be max possible */
	WinSizeX   = maxScreenX;
	WinSizeY   = maxScreenY;
    }
    else
    {	
	/* If the window size was set by command line arguments */
	WinSizeX = PF_MIN2(WinSizeX,maxScreenX);
	WinSizeY = PF_MIN2(WinSizeY,maxScreenY);
    }

    /* Initialize the channel-pipe assignment table: 0->0, 1->1, ... */
    for (i = 0; i < MAX_CHANS; i++)
	ChanOrder[i] = i;

    /* Initialize Keyboard and GUI ViewState data */
    initViewState();

    /* Set default multipipe mode */
    Multipipe = (numScreens > 1);

    /* Process the command line args */
    NumFiles = processCmdLine(argc, argv, &DatabaseFiles);

    /* set a path to everywhere we want to look for data.
     * append to any path set from the command line.
     */
    {
    int oldLength = 0;
    int newLength = 0;
    int fullLength = 0;
    const char *oldPath = pfGetFilePath();
    char *fullPath = NULL;
    char *newPath = 
       "."
       ":/usr/share/Performer/data"
       ":../data"
       ":../../data"
       ":../../data/polyhedra"
       ":../../../data"
       ":/usr/demos/models"
       ":/usr/demos/data/flt";

    if (oldPath != NULL)
	oldLength = strlen(oldPath);
    if (newPath != NULL)
	newLength = strlen(newPath);
    fullLength = oldLength + newLength;

    if (fullLength > 0)
    {
	/* allocate space for old, ":", new, and ZERO */
	fullPath = (char *)pfMalloc(fullLength + 2, NULL);
	fullPath[0] = '\0';
	if (oldPath != NULL)
	    strcat(fullPath, oldPath);
	if (oldPath != NULL && newPath != NULL)
	    strcat(fullPath, ":");
	if (newPath != NULL)
	    strcat(fullPath, newPath);
	pfFilePath(fullPath);
	pfFree(fullPath);
    }
    }

    /* Set Debug Level */
    pfNotifyLevel(NotifyLevel);

    /* Open loader DSOs before pfConfig() */
    for (i = 0 ; i < NumFiles ; i++)
	pfdInitConverter(DatabaseFiles[i]);

    if (!ViewState->guiFormat)
	ViewState->guiFormat = GUI_VERTICAL;

    /* Figure out the number of channels and pipelines required */
    switch (Multipipe) 
    {
    case TRUE:
	if (NumChans < 0)
	    NumChans = numScreens;
	NumPipes = NumChans;
	if (!ViewState->guiFormat)
	    ViewState->guiFormat = GUI_HORIZONTAL;
	break;
    case FALSE:
	if (NumChans < 0)
	    NumChans = 1;
	else if ((NumChans > 1) && (!ViewState->guiFormat))
		ViewState->guiFormat = GUI_HORIZONTAL;
	NumPipes = 1;
	break;
#ifdef IRISGL		
    case HYPERPIPE:    
	/* MUXPIPES not implemented for RealityEngine */

	if (0 && !getgdesc(GD_MUXPIPES))
	{
	    pfNotify(PFNFY_WARN, PFNFY_RESOURCE,
			"This machine not is not capable of Hyperpipe mode");

	    Multipipe = TRUE;
	}
	else
	{
	    NumPipes = numScreens;
	    NumChans = 1;
	}
	if (!ViewState->guiFormat)
	    ViewState->guiFormat = GUI_HORIZONTAL;
	break;
#else /* OPENGL */ /* XXX - Need query for HYPERPIPE extension */
#endif /* GL type */    	
    }
    ViewState->numChans = NumChans;
}


/* Initialize the visual database */
void
InitScene(void)
{
    int		 loaded		= 0;
    pfScene	*scene		= NULL;
    double	 startTime	= 0.0f;
    double	 elapsedTime	= 0.0f;

    /* Read in any databases specified on cmd line */
    ViewState->scene = scene = pfNewScene();

    startTime = pfGetTime();
	/* Read all files mentioned on command line */
	loaded = initSceneGraph(scene);

	/* Set up scene graph for collisions */
	if (loaded)
	    pfuCollideSetup(scene, PFUCOLLIDE_STATIC, 0xffffffff);

	if (loaded)
	    ViewState->texList = pfuMakeSceneTexList(scene);

	/* Initialize EarthSky, fog, and sun model */
	initEnvironment();
    elapsedTime = pfGetTime() - startTime;

    if (loaded)
    {
	pfNotify(PFNFY_INFO, PFNFY_PRINT, "Total scene-graph statistics");
	pfdPrintSceneGraphStats((pfNode *)scene, elapsedTime);
    }
    else
    {
    	pfSphere	bsphere;

	pfNotify(PFNFY_INFO, PFNFY_PRINT, "No Databases loaded");
	pfNotify(PFNFY_INFO, PFNFY_MORE, NULL);

	/* Set a default bounding sphere */
	pfSetVec3(bsphere.center, 0.0f, 0.0f, 0.0f);
	bsphere.radius = BOUND;
	pfNodeBSphere(scene, &bsphere, PFBOUND_DYNAMIC);
    }

    /* Write out nodes in scene (for debugging) */
    if (WriteFile)
    {
	if (!strstr(WriteFile, ".out") && pfdInitConverter(WriteFile))
	{
	    /* it's a valid extension, go for it */
	    if (!pfdStoreFile((pfNode *)scene, WriteFile))
		pfNotify(PFNFY_WARN, PFNFY_RESOURCE,
			 "No store function available for %s\n", WriteFile);
	}
	else 
	{
	    /* write the file as ASCII */
	    FILE *fp;
	    if (fp = fopen(WriteFile, "w"))
	    {
		pfPrint(scene, PFTRAV_SELF|PFTRAV_DESCEND, PFPRINT_VB_OFF, fp);
		fclose(fp);
	    }
	    else
		pfNotify(PFNFY_WARN, PFNFY_RESOURCE,
			 "Could not open scene.out for debug printing.");
	}
    }

    /* Set initial view position and angles */
    initView(scene);
}


/* Initialize graphics pipeline(s) */
void
InitPipe(void)
{
    pfPipeWindow *pw;
    int i;
    char str[PF_MAXSTRING];
    
    for (i=0; i < NumPipes; i++)
    {
	if ((NumPipes > 1) || (numScreens == 1))
	    pfPipeScreen(pfGetPipe(i), PF_MIN2(i, numScreens-1));
	ViewState->pw[i] = pw = pfNewPWin(pfGetPipe(i));
	sprintf(str,"IRIS Performer [pipe %d]", i);
	pfPWinName(pw,str);
	pfPWinOriginSize(pw, 0, 0, WinSizeX, WinSizeY);
	if (ViewState->input != PFUINPUT_GL)
	    pfPWinConfigFunc(pw, OpenXWin);
	else
	    pfPWinConfigFunc(pw, OpenWin);
	if (ViewState->visualID > -1)
	    pfPWinFBConfigId(pw, ViewState->visualID);
        pfConfigPWin(pw);
    }
    
    pfStageConfigFunc(-1, PFPROC_DRAW, OpenPipeline);
    pfStageConfigFunc(-1, PFPROC_CULL, ConfigCull);
    pfConfigStage(-1, PFPROC_CULL | PFPROC_DRAW);

    pfNotify(PFNFY_INFO,  PFNFY_PRINT,
		"Initialized %d Pipe%s", NumPipes, (NumPipes < 2) ? "" : "s");
    pfNotify(PFNFY_INFO,  PFNFY_MORE, NULL);
}


/* Initialize GUI Panel and widget callback routines */
void
InitGUI(void)
{
    pfPipe		*p;
    int		   	masterPipe;

    if (Multipipe)
    	masterPipe = ChanOrder[(NumChans - 1) >> 1];
    else
    	masterPipe = 0;

    /* Init GUI and internal data for pipe of master channel */
    p = pfGetPipe(masterPipe);
    pfuInitGUI(pfGetPipePWin(p, 0));

    /* Set the initial enable for the GUI */
    pfuEnableGUI(ViewState->gui);
}


/* Initialize Viewing Channel(s) */
void
InitChannel(void)
{
    int		    	i;
    pfFrameStats    	*frameStats;
    pfChannel	    	*chan;
    pfVec3   		xyz, hpr;
    float		fov;

    if (Multipipe)
    {
	/* Distribute channels across pipes as defined by ChanOrder table */
	for (i=0; i< NumChans; i++)
	    ViewState->chan[i] = pfNewChan(pfGetPipe(i));
    }
    else
    {
	/* Create all channels on the same pipe */
	for (i=0; i< NumChans; i++)
	    ViewState->chan[i] = pfNewChan(pfGetPipe(0));
    }
	
    /* 
     * Disable sorting for OPAQUE bin in CULLoDRAW mode to 
     * improve overlap and parallelism between CULL and DRAW.
     * Still draw transparent geometry last though.
    */
    if (ProcSplit != PFMP_DEFAULT &&
	(ProcSplit & (PFMP_CULLoDRAW | PFMP_FORK_DRAW)) == 
		     (PFMP_CULLoDRAW | PFMP_FORK_DRAW))
    {
	for (i=0 ; i < NumChans ; i++)
	    pfChanBinOrder(ViewState->chan[i], PFSORT_OPAQUE_BIN, PFSORT_NO_ORDER);
    }

    /* Keep user-provided channel ordering within bounds */
    for (i=0 ; i < NumChans ; i++)
	ChanOrder[i] = PF_MIN2(NumChans-1, ChanOrder[i]);

    /* Master channel is source for keyboard and mouse input */
    ViewState->masterChan = ViewState->chan[ChanOrder[(NumChans - 1) >> 1]];
    
    /* If multipipe, then we want channels to share their viewports */
    if (Multipipe)
    {
   	int		share;

	/* Get the default */
	share = pfGetChanShare(ViewState->masterChan);

	/* Add in the viewport share bit */
	share |= PFCHAN_VIEWPORT;

	if (GangDraw)
	{
	    /* Add GangDraw to share mask */
	    share |= PFCHAN_SWAPBUFFERS_HW;
	}
	pfChanShare(ViewState->masterChan, share);
    }

    /* Attach channels to master channel to form a channel group. */
    for (i=0; i< NumChans; i++)
	if (ViewState->chan[i] != ViewState->masterChan)
	    pfAttachChan(ViewState->masterChan, ViewState->chan[i]);

    /* Make sure we don't wrap field-of-view */
    if (NumChans * 45.0f > 360.0f)
	fov = 360.0f / NumChans;
    else
    	fov = 45.0f;

    /* if channel offset is supplied, use it instead of default */
    if(InitFOV) {
	fov = ViewState->fov[2];
	pfNotify(PFNFY_DEBUG, PFNFY_PRINT,
		 "Using user-supplied offset of %f\n",
		 fov);
    }
    if (Multipipe != HYPERPIPE)
    {
    	/* Set each channel's viewing offset */
    	pfSetVec3(xyz, 0.0f, 0.0f, 0.0f);

	/* Horizontally tile the channels from left to right */
    	for (i=0; i< NumChans; i++)
    	{
	    pfSetVec3(hpr, (((NumChans - 1)*0.5f) - i)*fov, 0.0f, 0.0f);
	    pfChanViewOffsets(ViewState->chan[ChanOrder[i]], xyz, hpr);
    	}
    }

    chan = ViewState->masterChan;

    /* Set the callback routines for the pfChannel */
    pfChanTravFunc(chan, PFTRAV_APP, AppFunc);
    pfChanTravFunc(chan, PFTRAV_CULL, CullFunc);
    pfChanTravFunc(chan, PFTRAV_DRAW, DrawFunc);

    /* Attach the visual database to the channel */
    pfChanScene(chan, ViewState->scene);

    /* Attach the EarthSky model to the channel */
    pfChanESky(chan, ViewState->eSky);

    /* Initialize the near and far clipping planes */
    pfChanNearFar(chan, ViewState->near, ViewState->far);

    /* Vertical FOV is matched to window aspect ratio. */
    pfChanAutoAspect(chan, PFFRUST_CALC_VERT);
    pfMakeSimpleChan(chan, fov);

    /* Initialize the viewing position and direction */
    pfChanView(chan, ViewState->initView.xyz, ViewState->initView.hpr);

    /* custom intiailization */
    initChannel(chan);

    /* Configure channel viewports for the Multichannel Option board */
    if (!Multipipe && (NumChans > 1))
	pfuConfigMCO(ViewState->chan, NumChans);

    /* Set up Stats structure */
    frameStats = pfGetChanFStats(chan);
    pfFStatsAttr(frameStats, PFFSTATS_UPDATE_SECS, 2.0f);
    pfFStatsClass(frameStats, PFSTATS_ENGFX, PFSTATS_ON);
    pfFStatsClass(frameStats, PFFSTATS_ENDB, PFSTATS_ON);

    /* For GLX windows and X input, get the window info on the master chan */
    if (ViewState->input == PFUINPUT_X)
    {
	pfuGetGLXWin(pfGetChanPipe(ViewState->masterChan), 
		&(ViewState->xWinInfo));
    }
    /* Call custom initGUI routine with the pipe of the master chan */
    initGUI(ViewState->masterChan);

    /* Initialize model for interactive motion */
    initXformer();

    pfNotify(PFNFY_INFO,  PFNFY_PRINT,
	"Initialized %d Channel%s", NumChans, (NumChans < 2) ? "" : "s");
    pfNotify(PFNFY_INFO,  PFNFY_MORE, NULL);

}


/*
 * All latency critical processing should be done here. This is
 * typically the viewing position. One should also read latency critical
 * devices here.
*/
void
PreFrame(void)
{
    /* Poll the mouse buttons and position */
    pfuGetMouse(&ViewState->mouse);

    /* Update the simulation */
    updateView(ViewState->masterChan);
}


/* Application simulation update is done here */
void
PostFrame(void)
{
    /* Get snapshot of event/key queues */
    pfuGetEvents(&ViewState->events);

    /* Update the simulation state */
    updateSim(ViewState->masterChan);
}

/*************************************************************************
 *									 *
 *		    ISECT PROCESS ROUTINES				 *
 *									 *
 *************************************************************************/

/*
 * Collide the viewing position represented by the pfiXformer with the
 * scene.
*/
void
IsectFunc(void *data)
{
    data;  /* suppress compiler warns */

    if (ViewState->xformer)
	pfiCollideXformer(ViewState->xformer);
}


/*************************************************************************
 *									 *
 *		    APP PROCESS ROUTINES				 *
 *									 *
 *************************************************************************/

/* In App callback routine, pre-pfApp() processing */
void
PreApp(pfChannel *chan, void *data)
{
    preApp(chan, data);	/* To avoid compiler warnings */
    return;
}


/* In App callback routine, post-pfApp() processing */
void
PostApp(pfChannel *chan, void *data)
{
    postApp(chan, data);	/* To avoid compiler warnings */
    return;
}

/*************************************************************************
 *									 *
 *		    CULL PROCESS ROUTINES				 *
 *									 *
 *************************************************************************/

void
ConfigCull(int pipe, unsigned int stage)
{
    /* 
     * Force CULL to run on a specific CPU if process locking 
     * is enabled.
    */
    if ((pfGetMultiprocess() & PFMP_FORK_CULL) &&
	(ViewState->procLock & PFMP_FORK_CULL))
    {
	if (CullCPU > -1)
	    pfuLockDownProc(CullCPU);
	else
	    pfuLockDownCull(pfGetPipe(pipe));
    }
}

/* In Cull callback routine, pre-pfCull() processing */
void
PreCull(pfChannel *chan, void *data)
{
    preCull(chan, data);	/* To avoid compiler warnings */
    return;
}


/* In Cull callback routine, post-pfCull() processing */
void
PostCull(pfChannel *chan, void *data)
{
    postCull(chan, data);	/* To avoid compiler warnings */
    return;
}


/*****************************************************************************
 *									     *
 *		DRAW PROCESS ROUTINES					     *
 *									     *
 *****************************************************************************/

/* Initialize window */

char const WinName[80] = "IRIS Performer";

/*
 * Initialize Draw Process and Pipe
 */
void
OpenPipeline(int pipe, unsigned int stage )
{
    int		xo, yo, xmax, ymax;
    
    /* 
     * Force DRAW process to run on a specific CPU for better real-time
     * behavior.
    */
    if (ViewState->procLock & PFMP_FORK_DRAW)
    {
	if (DrawCPU > -1)
	    pfuLockDownProc(DrawCPU);
	else	    
	    pfuLockDownDraw(pfGetPipe(pipe));
    }  
}

/* 
 * Create and configure an X window.
 */

void
OpenXWin(pfPipeWindow *pw)
{
    /* -1 -> use default screen or that specified by shell DISPLAY variable */
    pfuGLXWindow 	*win;
    pfPipe  *p;
    
    p = pfGetPWinPipe(pw);
    if (!(win = pfuGLXWinopen(p, pw, WinName)))
        pfNotify(PFNFY_FATAL, PFNFY_RESOURCE,
	    "OpenXPipeline: Could not create GLX Window.");

    /* set initial modes and welcome screen */
    InitGfx(pw);

}

/* 
 * Create and configure a GL window.
*/
void
OpenWin(pfPipeWindow *pw)
{
    pfOpenPWin(pw);
    /* set initial modes and welcome screen */
    InitGfx(pw);
}


/* Initialize graphics state */
void
ConfigPWin(pfPipeWindow *pw)
{
    /* set up default lighting parameters */
    pfLightModel	*lm;
    pfMaterial 		*mtl;

    /* 
     * Set up default material in case the database does not
     * have any. Use PFMTL_CMODE_AD so pfGeoSet colors supply the
     * ambient and diffuse material color.
    */
    mtl = pfNewMtl(NULL);
    pfMtlColorMode(mtl, PFMTL_FRONT, PFMTL_CMODE_AD);
    pfApplyMtl(mtl);

    /* Set up default lighting model. */
    lm = pfNewLModel(NULL);
    pfLModelAmbient(lm, 0.0f, 0.0f, 0.0f);

    /* 
     * Local viewer is required so that lighting matches across adjacent
     * channels since rotational offsets are encoded in the viewing 
     * matrix because GL will not fog channels properly if the offsets
     * are encoded in the projection matrix. 
    */
    if (NumChans > 1)
	pfLModelLocal(lm, 1);

    pfApplyLModel(lm);
    if (!ViewState->lighting)
	pfDisable(PFEN_LIGHTING);
    
    /* Prebind textures to be used in simulation */
    if (!(pfGetPWinType(pw) & PFPWIN_TYPE_STATS))
	pfuDownloadTexList(ViewState->texList, PFUTEX_SHOW);
}

void
InitGfx(pfPipeWindow *pw)
{

    /* Set up default state values.  */
    ConfigPWin(pw);

    /* call custom routine to initialize pipe */
    initPipe(pw);
    
}


/* In Draw callback routine, pre-pfDraw() processing */
void
PreDraw(pfChannel *chan, void *data)
{
    pfPipe 	*pipe = pfGetChanPipe(chan);
    int 	pipeId = pfGetId(pipe);

    /* Clear the frame buffer */
    pfClearChan(chan);

    /*
     * If this pipe does not display the GUI, then clear the space where
     * the GUI would ordinarily be.
    */
    /* XXX should really only do this once per pipe !!! */
    if ((Multipipe == 1) && ViewState->gui &&
	(chan != ViewState->masterChan) && 
	(ViewState->redrawOverlay || 
	 (ViewState->drawFlags[pipeId] & REDRAW_WINDOW)))
    {
	pfChannel	*chan;
	int		xo, yo, xs, ys;
	
	chan = pfuGetGUIChan();
	if (chan)
	{
	    pfGetChanOutputOrigin(chan, &xo, &yo);
	    pfGetChanOutputSize(chan, &xs, &ys);
	}
	else
	{
	    xo = yo = 0;
	    pfGetPipeSize(pipe, &xs, &ys);
	}
	if (xs > 0)
	{
#ifdef IRISGL
	    pushviewport();
	    viewport(xo, xo + xs -1, yo, yo + ys - 1);
	    cpack(0xff808080);
	    clear();
	    popviewport();
#else /* OPENGL */
	    glPushAttrib(GL_SCISSOR_BIT);
	    glScissor(xo, yo, xs, ys);
	    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	    glClear(GL_COLOR_BUFFER_BIT);
	    glPopAttrib();
#endif /* GL type */
	}
    }

    /* Update Draw modes that have been changed in ViewState structure. */
    localPreDraw(chan, data);
}


/* In Draw callback routine, post-pfDraw() processing */
void
PostDraw(pfChannel *chan, void *data)
{
    data;	/* suppress compiler warns */

    /* Display Stats, if active */
    if (ViewState->stats)
	pfDrawChanStats(chan);

    /*
     * Read GL keyboard, mouse. Make sure we only collect input on
     * master channel. If hyperpiping we must collect on all pipes.
    */
    if (chan == ViewState->masterChan)
    	pfuCollectInput();

    /* Call local post draw routine */
    localPostDraw(chan, data);
}
