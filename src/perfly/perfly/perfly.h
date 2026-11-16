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
 * file: perfly.h	
 * --------------
 * $Revision: 1.88 $
 * $Date: 1996/04/03 21:41:54 $
 */

#ifndef __PERFLY_H__
#define __PERFLY_H__

/******************************************************************************
 *			    Constants           
 *******************************************************************************/

#include <Performer/pfui.h>
#include "generic.h"
#include "env.h"

#define UPDATE_MODES	0x1
#define REDRAW_WINDOW	0x2

/* tokens for resetPosition */
#define POS_ORIG	1
#define POS_CENTER	2

#define DEVLIST_LENGTH 	9
#define BOUND		1000

/* Window visual selection indices */
#define PERFLY_WIN_NOMS	0

/* limits and sizes */
#define	MAX_LAMPS	8

#define	LIGHTING_OFF	0
#define	LIGHTING_EYE	1
#define	LIGHTING_SUN	2
#define	LIGHTING_COUNT	3

#define	DLIST_OFF	0
#define	DLIST_GEOSET	1
#define	DLIST_SCENE	2


/* Dynamic Resolution controls */
#define DVR_OFF		0
#define DVR_MANUAL	1
#define DVR_AUTO	2

/*
 *  Structure for data shared between processes
 */
typedef struct
{
	/* required by common/main.c */
    pfChannel	    *masterChan;	/* Main channel */
    int	    	    phase;		/* phase: FREE, FLOAT, or LOCK */
    float	    frameRate;		/* target frame rate	 */
    uint	    procLock;		/* flags to lock down processors */

	/* required by common/generic.c */
    pfPipeWindow    *pw[MAX_PIPES];	/* one pfPipeWindow per pfPipe */
    pfChannel	    *chan[MAX_CHANS];	/* array of all of the channels */
    int		    numChans;		/* number of channels */
    int	    	    input;		/* input type: GL or X */
    int	    	    gui;		/* ui mode flag */
    int	    	    exitFlag;		/* exit application flag */
    int	    	    updateChannels;	/* update channel configuration */
    float	    near;		/* near clipping plane  */
    float	    far;		/* far clipping plane  */
    float	    sceneSize;		/* maximum scene bound */
    int	    	    stats;		/* diag graph on/ff flag */
    pfList	    *texList;		/* texture list to download */

	/* required by common/env.c */
    pfEarthSky	    *eSky;		/* current EarthSky model */
    int	    	    earthSkyMode;	/* Earth-Sky mode */
    pfVec4	    earthSkyColor;	/* Earth-Sky color */
    pfVec4	    scribeColor;	/* scribe-mode color */
    int	    	    fog;		/* fog on/off */
    float	    farFogRange;	/* far fog range */
    float	    nearFogRange;	/* near fog range */
    pfDCS	    *sunDCS;		/* position of the sun */
    pfLightSource   *sun;		/* sun light source */
    float	    timeOfDay;		/* time of day */


	/* used for libpfutil input handling */
    pfuWidget       *guiWidgets[PFUGUI_MAXWIDGETS];
    int	    	    guiFormat;		/* GUI vertical, or horizontal */
    pfuGLXWindow    xWinInfo;		/* GLX window info */
    pfuMouse	    mouse;		/* mouse structure */
    pfuEventStream  events;		/* event structure */
    pfiTDFXformer   *xformer;		/* interactive movement model */
    int	    	    xformerModel;	/* Fly, Drive, or Trackball   */

	/* used in perfly to define light sources */
    pfVec3	    lampXYZ[MAX_LAMPS];	/* light source direction */
    pfVec3	    lampRGB[MAX_LAMPS];	/* light source color */
    int		    lamps;		/* number of light sources */

	/* "IRIS Performer" overlay-plane text */
    char	    welcomeText[256];
    char	    overlayText[256];

	/* Viewpoint and modes control */
    pfScene	    *scene;		/* visual database */
    pfDCS	    *sceneDCS;		/* DCS for trackball interface */ 
    pfGroup	    *sceneGroup;	/* root of all loaded files */
#ifndef IRISGL  /* OPENGL */  
    const GLubyte   *glExtString;	/* OpenGL extensions string */ 
#endif    
    int	    	    haveMultisample;	/* does window support multisample */
    int	    	    haveTagClear;	/* does window support tag clear */
    pfMatrix	    viewMat;	
    pfCoord	    viewCoord;		/* current view position, direction */ 
    pfCoord	    initView;		/* initial view position, direction */
    int		    resetPosition;	/* flag for reset position  */
    int	    	    drawFlags[MAX_PIPES];/* Flags for draw process(es) */
    int	    	    redrawOverlay;	/* signal to redraw overlay */
    int	    	    stress;		/* Enable/disable stress */
    int	    	    stressMax;		/* stress bound */
    float	    lowLoad; 		/* stress parameters- low bound */
    float	    highLoad;		/* high bound */
    float	    stressScale;	/* stress scale amount */
    float	    LODscale;		/* LOD scale value */
    float	    fadeRange;		/* LOD fade range- RE Graphics */
    int	    	    fade;		/* Fade on/off -   RE Graphics */
    int	    	    collideMode;	/* collision & terrain following flag*/
    int	    	    statsEnable;	/* diag graph mode flag */
    int	    	    printStats;		/* flag to print stats to stderr */
    int	    	    exitCount;		/* exit after this many frames */
    int 	    drawStyle;		/* draw style */
    int 	    aa;			/* anti-alias flag */
    int 	    backface;		/* backface culling flag */
    int	    	    lighting;		/* lighting mode */
    int	    	    texture;		/* texture enable/disable */
    int	    	    cullMode;		/* culling mode */
    float	    explode;		/* explode files */
    int	    	    snapImage;		/* save screen to file */
    int		    tree;		/* flag for tree display */
    pfVec3	    panScale;		/* pan in x,y and scale for tree display*/

    float	     cullDelta;		/* How much to shrink cull vol */
    pfGeode	    *cullVol;		/* Geode shows culling volume */
    pfDCS	    *cullVolDCS;	/* Position geode along view */

    int		    rotateCenter;	/* 0=origin, 1=center */
    int		    iterate;		/* number of times to load files */
    int		    optimizeGStates;    /* Optimize All gstates into a scene gstate */
    int		    optimizeTree;       /* Flatten and clean tree of empty nodes */
    int		    objFontType;
    char*	    objFontName;

    int		    combineBillboards;	/* maximum */
    int 	    visualID;		/* GLX Visual ID */

    int		    drawMode;		/* immediate mode or display list style */
    int		    doDVR;
    int		    winXSize, winYSize; /* window size for master pipe */
    int		    vChanXSize, vChanYSize; /* master channel full size */
    int	    	    dvrXSize, dvrYSize; /* dvr control for master channel */
    float	    fov[3];             /* fovH, fovV, and channel offset */
} SharedViewState;

/* Structures visible to all processes 
 * - must be initialized before pfConfig() 
 */
extern SharedViewState	*ViewState;   /* holds data shared between processes */
extern int PackedAttrFormat;
/* 
 * Function prototypes  - called by common 
 */

    /* Application process */
extern int initSceneGraph(pfScene *scene);
extern void initViewState(void);
extern void initView(pfScene *scene);
extern void initChannel(pfChannel *chan);
extern void updateView(pfChannel *chan);
extern void resetPosition(int pos);
extern void initXformer(void);
extern void updateSim(pfChannel *chan);
extern void xformerModel(int mode, int collideMode);
extern void drawModesChanged(void);
extern void preApp(pfChannel *chan, void *data);
extern void postApp(pfChannel *chan, void *data);

    /* Cull process */
extern void preCull(pfChannel *chan, void *data);
extern void postCull(pfChannel *chan, void *data);

    /* Draw process */
extern void initPipe(pfPipeWindow *pw);
extern void localPreDraw(pfChannel *chan, void *data);
extern void localPostDraw(pfChannel *chan, void *data);

#endif
