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
 * keybd.c -- keyboard commands
 */

#include <stdio.h>
#include <Performer/pfutil.h>
#include <Performer/pfui.h>

#include "perfly.h"
#include "gui.h"


void 
printCommands(void)
{
    fprintf(stderr, " Key     Function\n");
    fprintf(stderr, "-----    --------------\n");
    fprintf(stderr, " ESC     Exit program \n");
    fprintf(stderr, "  ?      Print help\n");
    fprintf(stderr, "SPACE    Stop moving\n");
    fprintf(stderr, " F1      Toggle graphical user interface\n");
    fprintf(stderr, " F12     Print current view position to stderr\n");
    fprintf(stderr, "  b      Toggle backface mode\n");
    fprintf(stderr, "  c      Toggle collision detection\n");
    fprintf(stderr, "  C      Change culling mode\n");
    fprintf(stderr, "  d      make and use pfGeoSet display lists\n");
    fprintf(stderr, "  D      make and use pfScene display list\n");
    fprintf(stderr, "  f      Decrease the target frame rate\n");
    fprintf(stderr, "  F      Increase the target frame rate\n");
    fprintf(stderr, "  g      Toggle stats and stats graph\n");
    fprintf(stderr, "  G      Print stats graph contents to stderr\n");
    fprintf(stderr, "  i      use immediate mode\n");
    fprintf(stderr, "  l      Toggle lighting\n");
    fprintf(stderr, "  k      dynamic video resize: manual->auto->off\n");
    fprintf(stderr, "  p      Change frame phase\n");
    fprintf(stderr, "  r      Reset positions\n");
    fprintf(stderr, "  s      Toggle diagnostics display\n");
    fprintf(stderr, "  S      Print stats graph contents to stderr\n");
    fprintf(stderr, "  t      Toggle texturing\n");
    fprintf(stderr, "  w      Toggle draw mode between solid/wire\n");
    fprintf(stderr, "  W      Cycle draw mode between all styles\n");
    fprintf(stderr, "  x      Snapshot RGB screen image to file\n");
    fprintf(stderr, "  X      Snapshot RGBA screen image to file\n");
    fprintf(stderr, "  z      Show culling frustum\n");
    fflush(stderr);
}

void
toggleDVR(void)
{
    /* can't toggle DVR mode when fill stats are enabled since we
     * are using a different window than the one the video channel
     * is bound to.
     */
    if (ViewState->statsEnable & PFSTATSHW_ENGFXPIPE_FILL) 
    {
pfNotify(PFNFY_NOTICE,PFNFY_PRINT,"DVR - can't change mode with fill stats enabled.");
	return;
    }
    switch(ViewState->doDVR)
    {
	case 0: 
	    ViewState->doDVR = DVR_MANUAL; 
pfNotify(PFNFY_NOTICE,PFNFY_PRINT,"DVR - MANUAL [up/down arrow keys control size]");
	    break;
	case DVR_MANUAL: 
	    ViewState->doDVR = DVR_AUTO; 
pfNotify(PFNFY_NOTICE,PFNFY_PRINT,"DVR - AUTO");
	    break;
	case DVR_AUTO: 
	    ViewState->doDVR = 0; 
pfNotify(PFNFY_NOTICE,PFNFY_PRINT,"DVR - OFF");
	    break;
    }
}

void
updateManualDVR(int dir)
{
    if (dir > 0)
    {
	if (ViewState->doDVR)
	{
	   ViewState->dvrXSize += 4;
	   ViewState->dvrYSize += 4;
	}
    }
    else if (dir < 0)
    {
	if (ViewState->doDVR)
	{
	   ViewState->dvrXSize -= 4;
	   ViewState->dvrYSize -= 4;
	}
    }
}

void
updateDrawMode (int newStyle)
{
    if (newStyle == ViewState->drawMode)
	return;
    ViewState->drawMode = newStyle;
}

void 
toggleBackface(void)
{
    ViewState->backface = !ViewState->backface;
    drawModesChanged();
}


void 
toggleDrawStyle(int justToggle)
{
    if (justToggle)
    {
	if (ViewState->drawStyle == PFUSTYLE_FILLED)
	    ViewState->drawStyle = PFUSTYLE_LINES;
	else
	    ViewState->drawStyle = PFUSTYLE_FILLED;
    }
    else
    {
	ViewState->drawStyle = (ViewState->drawStyle + 1) % PFUSTYLE_COUNT;
    }

    /* The GUI Controls this mode also, so update the Widget */
    updateWidget(GUI_DRAW_STYLE, (float)ViewState->drawStyle);
}


void 
toggleLighting(void)
{
    ViewState->lighting = (ViewState->lighting + 1) % LIGHTING_COUNT;
    /* The GUI Controls this mode also, so update the Widget */
    updateWidget(GUI_LIGHTING, (float)ViewState->lighting);
}


void 
toggleTexture(void)
{
    ViewState->texture = !ViewState->texture;
    /* The GUI Controls this mode also, so update the Widget */
    updateWidget(GUI_TEXTURE, (float)ViewState->texture);
}


void 
toggleStats(void)
{
    ViewState->stats = !ViewState->stats;
    /* The GUI Controls this mode also, so update the Widget */
    updateWidget(GUI_STATS,(float)ViewState->stats);
}

void 
toggleTree(void)
{
    ViewState->tree = !ViewState->tree;
    /* The GUI Controls this mode also, so update the Widget */
    updateWidget(GUI_TREE, (float)ViewState->tree);
}


void 
changeFieldRate(int inc)
{
    int fields, vrate;

    vrate = pfGetVideoRate();

    fields =  pfGetFieldRate();		/* Get current field rate */

    fields += inc;

    /* Clamp fields */
    if (fields <= 0)
	fields = 1;
    if (fields > vrate)
	fields = vrate;

    pfFieldRate(fields);

    ViewState->frameRate = pfGetFrameRate();	/* Get current frame rate */
}


void
toggleCullMode(void)
{
    int	oldMode	= 0;
    int	newMode	= 0;

    /* get old cull mode */
    oldMode = ViewState->cullMode;

    /* advance to next feature combination */
    newMode = ((oldMode + 1) & 0x0f) | (oldMode & ~0x0f);

    /* GeoSet culling only makes sense when view culling is enabled */
    if (newMode & PFCULL_GSET) newMode |= PFCULL_VIEW;

    /* set new cull mode */
    ViewState->cullMode = newMode;
    
    updateWidget(GUI_CULL, newMode);
}

void
togglePhase(void)
{
    pfPhase((pfGetPhase() + 1) % 4);
}

void
toggleCollide(void)
{
    ViewState->collideMode = !ViewState->collideMode;
    updateWidget(GUI_COLLIDE, ViewState->collideMode);
}

void 
toggleGUI(void)
{
    int i;
    ViewState->gui = !ViewState->gui;
    ViewState->updateChannels = 1;
    ViewState->redrawOverlay = 3;
    for (i=0; i<NumPipes; i++)
	ViewState->drawFlags[i] |= REDRAW_WINDOW;
}

/* This is defined in clipcenter.c in libpfutil */
extern int GLOBALBorder;

/* process input in the app process space */
void
processKeybdInput(void)
{
    int 	i;
    int		j;
    int	 	key;
    int 	dev;
    int	 	val;
    int	 	numDevs;
    pfuEventStream *events;
    
    events = &ViewState->events;
    numDevs = events->numDevs;

    for (j = 0; j < numDevs; j++)
    {
	dev = events->devQ[j];
	val = events->devVal[j];

	if (events->devCount[dev] > 0)
	{
	    switch(dev)
	    {
	    /* Window-system events */
	    case PFUDEV_REDRAW:
		events->devCount[dev] = 0; /* mark device done */
		pfuRedrawGUI();
		ViewState->redrawOverlay = 3;
		for (i=0; i<NumPipes; i++)
		    ViewState->drawFlags[i] |= REDRAW_WINDOW;
		break;
		
	    case PFUDEV_WINQUIT:
		ViewState->exitFlag = 1;
		break;
		
	    /* Main keyboard */
	    case PFUDEV_KEYBD:
		for (i = 0; i < events->numKeys; i++)
		{
		    key = events->keyQ[i];
		    if (events->keyCount[key])
		    {
			switch(key)
			{
			case 27:	/* ESC  exit program */
			    ViewState->exitFlag = 1;
			    break;

			case '?':	/* print commands */
			    printCommands();
			    break;
		
			case ' ':	/* Stop moving */
			    pfiStopXformer(ViewState->xformer);
			    break;
		
			case 'b':	/* Toggle backface mode */
			case 'B':
			    toggleBackface();
			    break;

			case 'c':	/* Change collide mode */
			    toggleCollide();
			    break;

			case 'C':	/* Change culling mode */
			    toggleCullMode();
			    break;

			case 'd':	/* make/use pfGeoState display lists */
			    updateDrawMode(DLIST_GEOSET);
			    break;

			case 'D':	/* make/use pfScene display list */
			    updateDrawMode(DLIST_SCENE);
			    break;

			case 'f':	/* Increment fields per frame */
			    changeFieldRate(1);
			    break;

			case 'F':	/* Decrement fields per frame */
    			    changeFieldRate(-1);
			    break;

			case 'h':
			    toggleTree();
			    break;

			case 'i':	/* use immediate mode */
			    updateDrawMode(DLIST_OFF);
			    break;

			case 'k':	/* Toggle lighting */
			case 'K':
			    toggleDVR();
			    break;

			case 'l':	/* Toggle lighting */
			case 'L':
			    toggleLighting();
			    break;
			
			case 'p':	/* Change frame phase */
			case 'P':
			    togglePhase();
			    break; 

			case 'r':	/* Reset positions */
			case 'R':
			    ViewState->resetPosition = POS_ORIG;
			    break;
			
			case 's':	/* Toggle diagnostics display */
			case 'g':
			    toggleStats();
			    break;
			
			case 'S':	/* Print stats display to stderr */
			case 'G':
			    if (!ViewState->stats)
				toggleStats();
			    ViewState->printStats = 1;
			    break;
			    
			case 't':	/* Toggle texturing */
			case 'T':
			    toggleTexture();
			    break;
	
			case 'w':	/* Toggle draw mode */
			case 'W':
			    toggleDrawStyle((key == 'w') ? 1 : 0);
			    break;
			    
			case 'x':	/* Snap RGB screen image */
			    ViewState->snapImage = 1;
			    break;

			case 'X':	/* Snap RGBA screen image */
			    ViewState->snapImage = 2;
			    break;

			case 'z':	/* toggle cull-frustum visualization */
			case 'Z':
			    if (ViewState->cullDelta < 1.0f)
				ViewState->cullDelta = 1.0f;
			    else
				ViewState->cullDelta = .5f;
			    updateWidget(GUI_FOV_CULL, ViewState->cullDelta);
			    break;

			default:	/* Unsupported keyboard input */ 
			    break;
			} 			
			/* reset the key as done */
			events->keyCount[key] = 0;
		    } 		    
		} 
		break; 

	    case PFUDEV_UPARROWKEY:
		updateManualDVR(1);
		break;

	    case PFUDEV_DOWNARROWKEY:
		updateManualDVR(-1);
		break;

	    case PFUDEV_RIGHTARROWKEY:
               	GLOBALBorder -= 8;
		if (GLOBALBorder < 16)
		    GLOBALBorder = 16;
		break;

            case PFUDEV_LEFTARROWKEY:
	        GLOBALBorder += 8;
	        if (GLOBALBorder > 1024)
	             GLOBALBorder = 1024;
	        break;
	
	    case PFUDEV_PRINTSCREENKEY:
		ViewState->snapImage = 1;
		break;
		
	    /* Function keys */
	    case PFUDEV_F1KEY:		/* GUI on / off */
		toggleGUI();
		break;
	    case PFUDEV_F12KEY:		/* Print current position */
	    {
	        pfCoord coord;
		pfiGetXformerCoord(ViewState->xformer, &coord);
		fprintf(stderr, "Viewpoint: XYZ(%f,%f,%f) HPR(%f,%f,%f)\n",
			coord.xyz[PF_X], coord.xyz[PF_Y], coord.xyz[PF_Z],
			coord.hpr[PF_H], coord.hpr[PF_P], coord.hpr[PF_R]);
		break;
	    }
	    }
	}
    }
    events->numDevs = 0;
}
