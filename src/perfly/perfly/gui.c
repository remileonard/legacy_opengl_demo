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
 * file: gui.c
 * --------------
 * $Revision: 1.105 $
 * $Date: 1996/04/09 09:01:19 $
 */

#include <stdio.h>
#include <string.h>

#include <Performer/pf.h>
#include <Performer/pfutil.h>
#include <Performer/pfui.h>
#include "main.h"
#include "perfly.h"
#include "gui.h"

/*-----------------------------------------------------------*/

static pfuPanel *initPanel(void);

static pfuGUIString styleStrs[] = 
{
    "Points",
    "Lines", 
    "Dashed",
    "Haloed",
    "Hidden",
    "Filled",
    "Scribed"
    };
static int styleVals[] = 
{ 
    PFUSTYLE_POINTS,
    PFUSTYLE_LINES,
    PFUSTYLE_DASHED,
    PFUSTYLE_HALOED,
    PFUSTYLE_HIDDEN,
    PFUSTYLE_FILLED,
    PFUSTYLE_SCRIBED,
};

static pfuGUIString textureStrs[] = 
{
    "Off",
    "Default",
};

static int textureVals[] = {
    0,
    1,
};

static pfuGUIString lightingStrs[] = 
{
    "Off", 
    "Eye", 
    "Sun", 
};

static int lightingVals[] = {
    LIGHTING_OFF, 
    LIGHTING_EYE, 
    LIGHTING_SUN, 
};

static pfuGUIString xformerStrs[] = 
{
    "Trackball", 
    "Drive", 
    "Fly"
    };
static int xformerVals[] = {
    PFITDF_TRACKBALL, 
    PFITDF_DRIVE, 
    PFITDF_FLY
    };

static pfuGUIString eskyStrs[] = 
{
    "Sky/Clear",
    "Sky/Grnd",
    "Sky", 
    "Clear",
    "Tag"
    };
static int eskyVals[] = 
{ 
    PFES_SKY_CLEAR,
    PFES_SKY_GRND,
    PFES_SKY,
    PFES_FAST,
    PFES_TAG
    };

static pfuGUIString cullStrs[] = 
{
    "All", 
    "View",
    "View, GSet",
    "Sort", 
    "ignore lsources", 
    "View, Sort", 
    "View, GSet, Sort", 
    "View (no lsources)",
    "Sort (ignore lsources)", 
    "View, GSet (ignore lsources)",
    "View, Sort (ignore lsources)", 
    "View, GSet, Sort (ignore lsources)", 
    "None"
    };
static int cullVals[] = 
{ 
    PFCULL_ALL,
    PFCULL_VIEW,
    PFCULL_VIEW | PFCULL_GSET,
    PFCULL_SORT, 
    PFCULL_IGNORE_LSOURCES, 
    PFCULL_VIEW               | PFCULL_SORT, 
    PFCULL_VIEW | PFCULL_GSET | PFCULL_SORT, 
    PFCULL_VIEW                             | PFCULL_IGNORE_LSOURCES,
    PFCULL_SORT | PFCULL_IGNORE_LSOURCES,
    PFCULL_VIEW | PFCULL_GSET               | PFCULL_IGNORE_LSOURCES,
    PFCULL_VIEW               | PFCULL_SORT | PFCULL_IGNORE_LSOURCES,
    PFCULL_VIEW | PFCULL_GSET | PFCULL_SORT | PFCULL_IGNORE_LSOURCES,
    0
    };

static pfuGUIString highStrs[] = 
{
    "Lines", 
    "Line BBox", 
    "Fill BBox", 
    "Normals",
    "Points", 
    "Fill",  
    "Pattern",  
    "Texture"  
    };
static int highVals[] = 
{
    PFHL_LINES,
    PFHL_BBOX_LINES | PFHL_LINES,
    PFHL_BBOX_LINES | PFHL_BBOX_FILL | PFHL_LINES_R | PFHL_SKIP_BASE,
    PFHL_NORMALS,
    PFHL_POINTS,
    PFHL_FILL | PFHL_LINES | PFHL_LINES_R | PFHL_SKIP_BASE,
    PFHL_FILLPAT | PFHL_LINESPAT,
    PFHL_FILLTEX
    };

static pfuGUIString fogStrs[] = 
{
    "Off", 
    "Linear",  
    "Exp", 
    "Exp^2", 
    "Spline"
    };
static int fogVals[] = 
{
    PFFOG_OFF,
    PFFOG_PIX_LIN,
    PFFOG_PIX_EXP,
    PFFOG_PIX_EXP2,
    PFFOG_PIX_SPLINE
    };

#define NUM_STATSVALS 4
static pfuGUIString statsStrs[] = 
{
    "Dft", 
    "Gfx", 
    "Pipe", 
    "Fill"
    };
static int statsVals[] = 
{
    PFFSTATS_ENPFTIMES,
    PFFSTATS_ENPFTIMES | PFSTATS_ENGFX | PFFSTATS_ENDB | PFFSTATS_ENCULL,
    PFFSTATS_ENGFXPFTIMES,
    PFSTATSHW_ENGFXPIPE_FILL | PFFSTATS_ENPFTIMES,
    };

/*-----------------------------------------------------------*/


void
initGUI(pfChannel *masterChan)
{
    float 	a, b, c, d;
    pfuPanel	*panel;
    
    /* Set up size and position of GUI control panel */
    pfGetChanViewport(masterChan, &a, &b, &c, &d);
    
    if (ViewState->guiFormat == GUI_VERTICAL)
    {
	pfuGUIViewport(a, a + 0.22 * (b-a), c, d);
	pfChanViewport(masterChan, a + 0.22 * (b-a), b, c, d);
    } 
    else 
    {
	pfuGUIViewport(a, b, c, c + 0.22 * (d-c));
	pfChanViewport(masterChan, a , b, c + 0.22 * (d-c), d);
    }
    
    panel = initPanel();
    pfuEnablePanel(panel);

    updateGUI(ViewState->gui);
}


void
updateWidget(int widget, float val)
{
    if (widget == GUI_STATS)
	pfuWidgetOnOff(ViewState->guiWidgets[widget], val);
    else
	pfuWidgetValue(ViewState->guiWidgets[widget], val);

    (*pfuGetWidgetActionFunc(ViewState->guiWidgets[widget]))
			    (ViewState->guiWidgets[widget]);
}


void
updateGUIViewMsg(void)
{
    static char     posBuf[256];
    pfCoord	    *viewpt;
    
    /* put in position */
    viewpt =  &ViewState->viewCoord;
    sprintf(posBuf," X%5.1f Y%5.1f Z%5.1f H%5.1f P%5.1f R%5.1f",
	    viewpt->xyz[PF_X],viewpt->xyz[PF_Y],viewpt->xyz[PF_Z],
	    viewpt->hpr[PF_H],viewpt->hpr[PF_P],viewpt->hpr[PF_R]);
    pfuWidgetLabel(ViewState->guiWidgets[GUI_MSG], posBuf);
}


/*
 *    updateGUI-
 *	    This routine is called when channels are updated,
 *	    i.e. the GUI is turned ON/OFF
 */
void
updateGUI(int guiOn)
{
    
    if( guiOn )
    {
	float l, r, b, t;
	float ca, cb, cc, cd;
	
	pfuEnableGUI(TRUE);
    	pfuGetGUIViewport(&l, &r, &b, &t);
	pfGetChanViewport(ViewState->masterChan,&ca,&cb,&cc,&cd);
	
	if(ViewState->guiFormat == GUI_VERTICAL)
	    pfChanViewport(ViewState->masterChan, r, cb, cc, cd);
	else
	    pfChanViewport(ViewState->masterChan, ca, cb, t, cd);
    }
    else    /* turn off GUI */
    {
	pfuEnableGUI(FALSE);
	
	if (Multipipe || (ViewState->numChans == 1))
	    pfChanViewport(ViewState->masterChan, 0.0f, 1.0f, 0.0f, 1.0f);
	else if (ViewState->numChans > 1)
	    pfuConfigMCO(ViewState->chan,ViewState->numChans);
    }
}

static void
controlPanel(pfuWidget *w)
{
    switch(pfuGetWidgetId(w))
    {
    case GUI_COLLIDE:
	xformerModel(ViewState->xformerModel, 
		     ViewState->collideMode = (int)pfuGetWidgetValue(w));
	break;
	
    case GUI_CULL:
	ViewState->cullMode = (int)pfuGetWidgetValue(w);
	break;
	
    case GUI_ESKY:
	ViewState->earthSkyMode = (int)pfuGetWidgetValue(w);
	break;
	
    case GUI_FAR:
	ViewState->far = pfuGetWidgetValue(w);
	pfChanNearFar(ViewState->masterChan, ViewState->near, ViewState->far);
	break;
	
    case GUI_XFORMER:
        {
            int newValue        = (int)pfuGetWidgetValue(w);
            int newSelection    = (int)pfuGetWidgetSelection(w);
            ViewState->xformerModel = newValue;
            ViewState->rotateCenter = (newSelection == 0);
        }
        xformerModel(ViewState->xformerModel, ViewState->collideMode);
        break;
	
    case GUI_HIGH_STYLE:
	{
	    pfHighlight *hlight = pfuGetGUIHlight();
	    int mode = (int)pfuGetWidgetValue(w);
	    if (hlight != NULL)
		pfHlightMode(hlight, mode);
	}
	break;
	
    case GUI_FOG:
	ViewState->fog = pfuGetWidgetValue(w);
	if (ViewState->fog)
	{
	    pfuEnableWidget(ViewState->guiWidgets[GUI_NEAR_FOG_RANGE]);
	    pfuEnableWidget(ViewState->guiWidgets[GUI_FAR_FOG_RANGE]);
	}
	else
	{
	    pfuDisableWidget(ViewState->guiWidgets[GUI_NEAR_FOG_RANGE]);
	    pfuDisableWidget(ViewState->guiWidgets[GUI_FAR_FOG_RANGE]);
	}
	break;
	
	/* Update Fog Far Range according to slider value */
    case GUI_FAR_FOG_RANGE:
	ViewState->farFogRange = pfuGetWidgetValue(w);
	if (ViewState->farFogRange < ViewState->nearFogRange)
	{
	    ViewState->farFogRange = 1.01f * ViewState->nearFogRange;
	    pfuWidgetValue(ViewState->guiWidgets[GUI_FAR_FOG_RANGE], 
			   ViewState->farFogRange);
	}
	break;
	
    case GUI_NEAR_FOG_RANGE:
	ViewState->nearFogRange = pfuGetWidgetValue(w);
	if (ViewState->farFogRange < ViewState->nearFogRange)
	{
	    ViewState->nearFogRange = (1.0f/1.01f) * ViewState->farFogRange;
	    pfuWidgetValue(ViewState->guiWidgets[GUI_NEAR_FOG_RANGE], 
			   ViewState->nearFogRange);
	}
	break;
	
	/* Update Field of View according to slider value */
    case GUI_FOV:  
	{
	    int		 c;
	    float	 fov	= pfuGetWidgetValue(w)/ViewState->numChans;
	    pfVec3	 xyz;
	    pfVec3	 hpr;
	    
	    /* all channels share this new FOV */
	    if (InitFOV) { /* don't modify the fov */
		pfChanFOV (ViewState->masterChan, ViewState->fov[0],
			   ViewState->fov[1]);
		pfNotify(PFNFY_DEBUG, PFNFY_PRINT,
			 "Using user-supplied FOV for masterChan = %f %f\n",
			 ViewState->fov[0], ViewState->fov[1]);
	    } else
		pfChanFOV(ViewState->masterChan, fov, -1.0f);
	    
	    /* all channels have a positional offset of zero */
	    pfSetVec3(xyz, 0.0f, 0.0f, 0.0f);
	    
	    /* set each channel's rotational offset */
	    for (c = 0; c < ViewState->numChans; c++)
	    {
		if (InitFOV) {
		    pfSetVec3(hpr, 
			      (((ViewState->numChans - 1)*0.5f) - c) *
			      ViewState->fov[2],
			      0.0f, 0.0f);
		    pfNotify(PFNFY_DEBUG, PFNFY_PRINT,
			     "Using user-supplied offset for channel %d %f\n",
			     c, hpr[0]);
		} else
		    pfSetVec3(hpr, 
			      (((ViewState->numChans - 1)*0.5f) - c) *
			      fov, 0.0f, 0.0f);

		pfChanViewOffsets(ViewState->chan[ChanOrder[c]], xyz, hpr);
	    }
	}
	break;
	
	/* Update Cull Field of View Delta according to slider value */
    case GUI_FOV_CULL:  
	ViewState->cullDelta = pfuGetWidgetValue(w);
	break;
	
	/* Turns off GUI- F1 to turn back on */
    case GUI_GUI: 
	ViewState->gui = 0;
	ViewState->updateChannels = 1;
	break;
	
	/* Toggle Lighting */
    case GUI_LIGHTING:  
	{
	    ViewState->lighting = (int)pfuGetWidgetValue(w);

	    if (ViewState->lighting == LIGHTING_OFF)
		pfuDisableWidget(ViewState->guiWidgets[GUI_TOD]);
	    else
		pfuEnableWidget(ViewState->guiWidgets[GUI_TOD]);
	    
	    drawModesChanged();
	}
	break;
	
    case GUI_AA:
	{
	    static int prevFade = 1;
	    ViewState->aa = (int)pfuGetWidgetValue(w);
	    if (!ViewState->aa)
	    {
		prevFade = ViewState->fade;
		if (ViewState->fade)
		{
		    ViewState->fade = 0;
		    pfChanLODAttr(ViewState->masterChan, PFLOD_FADE, 0.0f);
		    pfuWidgetValue(ViewState->guiWidgets[GUI_LOD_FADE],0.0f);
		    pfuDisableWidget(ViewState->guiWidgets[GUI_LOD_FADE]);
		    pfuDisableWidget(ViewState->guiWidgets[GUI_LOD_FADE_RANGE]);
		}
	    }
	    else if (ViewState->aa && prevFade && ViewState->haveMultisample)
	    {
		ViewState->fade = 1;
		pfChanLODAttr(ViewState->masterChan, PFLOD_FADE, ViewState->fadeRange);
		pfuWidgetValue(ViewState->guiWidgets[GUI_LOD_FADE],1.0f);
		pfuEnableWidget(ViewState->guiWidgets[GUI_LOD_FADE]);
		pfuEnableWidget(ViewState->guiWidgets[GUI_LOD_FADE_RANGE]);
	    }
	    drawModesChanged();
	    break;
	}
	/* Update LOD Fade Range according to slider value */
    case GUI_LOD_FADE:
	ViewState->fade =  pfuGetWidgetValue(w);
	if (ViewState->haveMultisample)
	{
	    if (ViewState->fade)
	    {
		pfChanLODAttr(ViewState->masterChan, PFLOD_FADE, ViewState->fadeRange);
		pfuEnableWidget(ViewState->guiWidgets[GUI_LOD_FADE_RANGE]);
	    }
	    else 
	    {
		pfChanLODAttr(ViewState->masterChan, PFLOD_FADE, 0.0f);
		pfuDisableWidget(ViewState->guiWidgets[GUI_LOD_FADE_RANGE]);
	    }
	}
	break;
	
    case GUI_LOD_FADE_RANGE:
	if (ViewState->haveMultisample)
	{
	    if (ViewState->fade)
		pfChanLODAttr(ViewState->masterChan, PFLOD_FADE, 
			      ViewState->fadeRange = pfuGetWidgetValue(w));
	    else
		pfChanLODAttr(ViewState->masterChan, PFLOD_FADE, 0.0f);
	}
	break;
	
	/* Update LOD Scale according to slider value */
    case GUI_LOD_SCALE:
	ViewState->LODscale = pfuGetWidgetValue(w);
	break;
	
	/* Update Near Clip Plane according to slider value */
    case GUI_NEAR:
	ViewState->near = pfuGetWidgetValue(w);
	pfChanNearFar(ViewState->masterChan, ViewState->near, ViewState->far);
	break;
	
    case GUI_PHASE:
	pfPhase((int)pfuGetWidgetValue(w));
	break;
	
    case GUI_PIX_LIMIT:
	pfChanLODAttr(ViewState->masterChan, PFLOD_STRESS_PIX_LIMIT,
		      (int)pfuGetWidgetValue(w));
	break;
	
    case GUI_REPOSITION:
	ViewState->resetPosition = POS_ORIG;
	break;
	
    case GUI_CENTER:
	ViewState->resetPosition = POS_CENTER;
	break;
	
    case GUI_QUIT:
	ViewState->exitFlag = TRUE;
	break;
	
    case GUI_STATS:
	ViewState->stats = pfuIsWidgetOn(w) ? 1 : 0;
	ViewState->statsEnable = (uint) pfuGetWidgetValue(w);
	break;
	
    case GUI_STRESS:
	ViewState->stress = (int)pfuGetWidgetValue(w);
	if (ViewState->stress)
	{
	    pfuEnableWidget(ViewState->guiWidgets[GUI_STRESS_LOW]);
	    pfuEnableWidget(ViewState->guiWidgets[GUI_STRESS_HIGH]);
	    pfuEnableWidget(ViewState->guiWidgets[GUI_STRESS_SCALE]);
	    pfuEnableWidget(ViewState->guiWidgets[GUI_STRESS_MAX]);
	    pfuEnableWidget(ViewState->guiWidgets[GUI_PIX_LIMIT]);
	}
	else
	{
	    pfuDisableWidget(ViewState->guiWidgets[GUI_STRESS_LOW]);
	    pfuDisableWidget(ViewState->guiWidgets[GUI_STRESS_HIGH]);
	    pfuDisableWidget(ViewState->guiWidgets[GUI_STRESS_SCALE]);
	    pfuDisableWidget(ViewState->guiWidgets[GUI_STRESS_MAX]);
	    pfuDisableWidget(ViewState->guiWidgets[GUI_PIX_LIMIT]);
	}
	break;
	
    case GUI_STRESS_LOW:
	{
	    float low = pfuGetWidgetValue(w);
	    
	    if (low < ViewState->highLoad)
		ViewState->lowLoad = low;
	    else
	    {
		ViewState->lowLoad = ViewState->highLoad -0.1f;
		pfuWidgetValue(w,ViewState->lowLoad);
	    }
	}
	break;
	
    case GUI_STRESS_HIGH:
	{
	    float  high = pfuGetWidgetValue(w);
	    
	    if (high > ViewState->lowLoad)
		ViewState->highLoad = high;
	    else
	    {
		ViewState->highLoad = ViewState->lowLoad + 0.1f;
		pfuWidgetValue(w,ViewState->highLoad);
	    }
	}
	break;
	
    case GUI_STRESS_MAX:
	ViewState->stressMax = pfuGetWidgetValue(w);
	break;
	
    case GUI_STRESS_SCALE:
	ViewState->stressScale = pfuGetWidgetValue(w);
	break;
	
    case GUI_TEXTURE:
	ViewState->texture = (int)pfuGetWidgetValue(w);
	drawModesChanged();
	break;
	
    case GUI_TOD:
	ViewState->timeOfDay = pfuGetWidgetValue(w);
	updateTimeOfDay(ViewState->timeOfDay);
	break;
	
    case GUI_TREE:
	ViewState->tree = (int) pfuGetWidgetValue(w);
	if (ViewState->tree)
	{
	    pfuEnableWidget(ViewState->guiWidgets[GUI_PANX]);
	    pfuEnableWidget(ViewState->guiWidgets[GUI_PANY]);
	    pfuEnableWidget(ViewState->guiWidgets[GUI_SCALE]);
	}
	else
	{
	    pfuDisableWidget(ViewState->guiWidgets[GUI_PANX]);
	    pfuDisableWidget(ViewState->guiWidgets[GUI_PANY]);
	    pfuDisableWidget(ViewState->guiWidgets[GUI_SCALE]);
	}
	break;
	
    case GUI_PANX:
	ViewState->panScale[0] = pfuGetWidgetValue(w);
	break;
	
    case GUI_PANY:
	ViewState->panScale[1] = pfuGetWidgetValue(w);
	break;
	
    case GUI_SCALE:
	ViewState->panScale[2] = pfuGetWidgetValue(w);
	break;
	
    case GUI_DRAW_STYLE:
	ViewState->drawStyle = (int)pfuGetWidgetValue(w);
	drawModesChanged();
	break;
	
    case GUI_SNAP:
	ViewState->snapImage = 1;
	break;
	
    default:
	break;
    }
}


void resetAll(pfuWidget *w)
{
    w; /* suppress compiler warn */
    
    pfuResetGUI();
    ViewState->resetPosition = POS_ORIG;
}


static pfuPanel *
initPanel(void)
{
    float  	xo, yo, ys, xs;
    int    	x, y;
    int   	xSize, ySize;
    pfuWidget 	*wid;
    pfuPanel 	*panel;
    
    /* init the GUI panel for perfly */
    panel = pfuNewPanel();
    pfuGetPanelOriginSize(panel, &xo, &yo, &xs, &ys);
    
    x = (int)xo;
    y = (int)ys;

    y -= PFUGUI_BUTTON_YINC;

    xSize = PFUGUI_BUTTON_VLONG_XSIZE + PFUGUI_BUTTON_VLONG_XINC;
    ySize = PFUGUI_MESSAGE_YSIZE;
    
    /* the message bar with positions */
    wid = pfuNewWidget(panel, PFUGUI_MESSAGE, 0);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    ViewState->guiWidgets[GUI_MSG] = wid;
    
    y -= PFUGUI_BUTTON_YINC;

    xSize = PFUGUI_BUTTON_VLONG_XSIZE;
    ySize = PFUGUI_BUTTON_YSIZE;

    /* a quit button */
    wid = pfuNewWidget(panel, PFUGUI_BUTTON, GUI_QUIT);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Quit");
    pfuWidgetActionFunc(wid, controlPanel);
    
    x += PFUGUI_BUTTON_VLONG_XINC;
    
    /* kill the gui */
    wid = pfuNewWidget(panel, PFUGUI_BUTTON, GUI_GUI);
    pfuWidgetLabel(wid,"GUI Off");
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetActionFunc(wid, controlPanel);
    
    x = xo;
    y -= PFUGUI_BUTTON_YINC;
    xSize = PFUGUI_BUTTON_VLONG_XSIZE;
    
    /* Reset to origin button */
    wid = pfuNewWidget(panel, PFUGUI_BUTTON, GUI_REPOSITION);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Reposition");
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_REPOSITION] = wid;
    
    x += PFUGUI_BUTTON_VLONG_XINC;
    
    /* Reset to center */
    wid = pfuNewWidget(panel, PFUGUI_BUTTON, GUI_CENTER);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Center");
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_CENTER] = wid;
    
    x = xo;
    y -= PFUGUI_BUTTON_YINC;
    
    /* Snap screen */
    wid = pfuNewWidget(panel, PFUGUI_BUTTON, GUI_SNAP);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Snap Screen");
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_SNAP] = wid;
    
    x += PFUGUI_BUTTON_VLONG_XINC;
    
    /* Reset all */
    wid = pfuNewWidget(panel, PFUGUI_BUTTON, GUI_RESET_ALL);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Reset All");
    pfuWidgetActionFunc(wid, resetAll);
    ViewState->guiWidgets[GUI_RESET_ALL] = wid;
    
    x = xo;
    y -= PFUGUI_BUTTON_YINC;
    
    /* select (fancy) draw style */
    wid = pfuNewWidget(panel, PFUGUI_MENU_BUTTON, GUI_DRAW_STYLE);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid,"Style");
    pfuWidgetActionFunc(wid, controlPanel);
    pfuWidgetSelections(wid, styleStrs, styleVals, NULL, 
			sizeof(styleStrs)/sizeof(styleStrs[0]));
    pfuWidgetDefaultValue(wid, PFUSTYLE_FILLED);
    ViewState->guiWidgets[GUI_DRAW_STYLE] = wid;
    
    x += PFUGUI_BUTTON_VLONG_XINC;

    /* select (fancy) highlight style */
    wid = pfuNewWidget(panel, PFUGUI_MENU_BUTTON, GUI_HIGH_STYLE);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Hlight");
    pfuWidgetActionFunc(wid, controlPanel);
    pfuWidgetSelections(wid, highStrs, highVals, NULL, 
			sizeof(highStrs)/sizeof(highStrs[0]));
    pfuWidgetDefaultValue(wid, PFHL_LINES);
    ViewState->guiWidgets[GUI_HIGH_STYLE] = wid;
    
    x = xo;
    y -= PFUGUI_BUTTON_YINC;
    
    /* toggle Texture */
    wid = pfuNewWidget(panel, PFUGUI_MENU_BUTTON, GUI_TEXTURE);
    pfuWidgetLabel(wid,"Texture");
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetActionFunc(wid, controlPanel);
    pfuWidgetSelections(wid, textureStrs, textureVals, NULL,
			sizeof(textureStrs)/sizeof(textureStrs[0]));
    pfuWidgetDefaultValue(wid,ViewState->texture);
    ViewState->guiWidgets[GUI_TEXTURE] = wid;
    
    x += PFUGUI_BUTTON_VLONG_XINC;
    
    
    /* Earth/Sky Model*/
    wid = pfuNewWidget(panel, PFUGUI_MENU_BUTTON, GUI_ESKY);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "BG");
    pfuWidgetActionFunc(wid, controlPanel);
    /* if on RE, add PFES_TAG option */
    if (ViewState->haveTagClear) 
	pfuWidgetSelections(wid, eskyStrs, eskyVals, NULL,
			    sizeof(eskyStrs)/sizeof(eskyStrs[0]));
    else
	pfuWidgetSelections(wid, eskyStrs, eskyVals, NULL,
			    sizeof(eskyStrs)/sizeof(eskyStrs[0]) - 1);
    pfuWidgetDefaultValue(wid, ViewState->earthSkyMode);
    ViewState->guiWidgets[GUI_ESKY] = wid;
    
    x = xo;
    y -= PFUGUI_BUTTON_YINC;
    /*xSize = PFUGUI_BUTTON_VLONG_XSIZE + PFUGUI_BUTTON_VLONG_XINC;*/
    /* toggle Collision */
    wid = pfuNewWidget(panel, PFUGUI_SWITCH, GUI_COLLIDE);
    pfuWidgetLabel(wid,"Collide");
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetActionFunc(wid, controlPanel);
    pfuWidgetDefaultValue(wid,ViewState->collideMode);
    ViewState->guiWidgets[GUI_COLLIDE] = wid;

    x += PFUGUI_BUTTON_VLONG_XINC;
    
    /* Xformer model */
    wid = pfuNewWidget(panel, PFUGUI_MENU_BUTTON, GUI_XFORMER);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Motion");
    pfuWidgetActionFunc(wid, controlPanel);
    pfuWidgetSelections(wid, xformerStrs, xformerVals, NULL, 
			sizeof(xformerStrs)/sizeof(xformerStrs[0]));
    pfuWidgetDefaultValue(wid, ViewState->xformerModel);
    ViewState->guiWidgets[GUI_XFORMER] = wid;
    
    x = xo;
    y -= PFUGUI_BUTTON_YINC;
    xSize = PFUGUI_BUTTON_VLONG_XSIZE + PFUGUI_BUTTON_VLONG_XINC;
    
    /* Stats mode */
    wid = pfuNewWidget(panel,
		       PFUGUI_RADIO_BUTTON_TOGGLE | PFUGUI_HORIZONTAL, GUI_STATS);
    pfuWidgetLabel(wid, "Stats");
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetActionFunc(wid, controlPanel);
    pfuWidgetSelections(wid, statsStrs, statsVals, NULL,
			sizeof(statsStrs)/sizeof(statsStrs[0]));
    ViewState->guiWidgets[GUI_STATS] = wid;
    pfuWidgetDefaultValue(wid, PFFSTATS_ENPFTIMES);
    
    if (ViewState->guiFormat == GUI_HORIZONTAL)
    {
	xo += 2*PFUGUI_BUTTON_VLONG_XINC;
	y = ys;
    }
    x = xo;
    y -= PFUGUI_BUTTON_YINC;

    ySize = PFUGUI_BUTTON_YSIZE;
    
    /* cull mode */
    wid = pfuNewWidget(panel, PFUGUI_MENU_BUTTON, GUI_CULL);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Cull Mode");
    pfuWidgetActionFunc(wid, controlPanel);
    pfuWidgetSelections(wid, cullStrs, cullVals, NULL, 
			sizeof(cullStrs)/sizeof(cullStrs[0]));
    pfuWidgetDefaultValue(wid, ViewState->cullMode);
    ViewState->guiWidgets[GUI_CULL] = wid;

    ySize = PFUGUI_SLIDER_YSIZE;
    y -= PFUGUI_SLIDER_YINC;
     
    /* Slider for FOV */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_FOV);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Field of View");
    switch (ViewState->numChans)
    {
    case 1:
	pfuWidgetRange(wid, 1, 10.0f, 160.0f, 45.0f);
	break;
    case 2:
	pfuWidgetRange(wid, 1, 10.0f, 320.0f, 90.0f);
	break;
    default:
	pfuWidgetRange(wid, 1, 10.0f, 360.0f, PF_MIN2(ViewState->numChans * 45.0f, 360.0f));
	break;
    }
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_FOV] = wid;
    
    y -= PFUGUI_SLIDER_YINC;
    
    /* Slider to set far clip plane distance */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER_LOG, GUI_FAR);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Far Clip");
    pfuWidgetRange(wid, 1, 
		   0.5f * ViewState->near,
		   10.0f * ViewState->far,
		   ViewState->far);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_FAR] = wid;
    
    y -= PFUGUI_SLIDER_YINC;
    
    /* Slider to set near clip plane distance */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER_LOG, GUI_NEAR);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Near Clip");
    pfuWidgetRange(wid, 1, 
		   0.1f * ViewState->near/10.0f,
		   2.0f * ViewState->far,
		   ViewState->near);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_NEAR] = wid;

    y -= PFUGUI_SLIDER_YINC;

    /* Slider for cull-view FOV */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_FOV_CULL);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Cull View FOV");
    pfuWidgetRange(wid, 1, 0.0f, 1.0f, 1.0f);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_FOV_CULL] = wid;
    
    if (ViewState->guiFormat == GUI_HORIZONTAL)
    {
	xo += 2*PFUGUI_BUTTON_VLONG_XINC;
	y = ys;
    }
    
    x = xo;
    y -= PFUGUI_BUTTON_YINC;

    xSize = PFUGUI_BUTTON_VLONG_XSIZE;
    ySize = PFUGUI_BUTTON_YSIZE;
    
    
    /* preset fogs */
    wid = pfuNewWidget(panel, PFUGUI_MENU_BUTTON, GUI_FOG);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Fog");
    pfuWidgetActionFunc(wid, controlPanel);
    {
	int haveSplineFog;
	pfQueryFeature(PFQFTR_FOG_SPLINE, &haveSplineFog);
	if (haveSplineFog)
	    pfuWidgetSelections(wid, fogStrs, fogVals, NULL, 
				sizeof(fogStrs)/sizeof(fogStrs[0]));
	else
	    pfuWidgetSelections(wid, fogStrs, fogVals, NULL, 
				sizeof(fogStrs)/sizeof(fogStrs[0]) - 1);
    }
    pfuWidgetDefaultValue(wid, ViewState->fog);
    ViewState->guiWidgets[GUI_FOG] = wid;

    x += PFUGUI_BUTTON_VLONG_XINC;

    /* toggle Lighting*/
    wid = pfuNewWidget(panel, PFUGUI_MENU_BUTTON, GUI_LIGHTING);
    pfuWidgetLabel(wid,"Lighting");
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetActionFunc(wid, controlPanel);
    pfuWidgetSelections(wid, lightingStrs, lightingVals, NULL, 
			sizeof(lightingStrs)/sizeof(lightingStrs[0]));
    pfuWidgetDefaultValue(wid, ViewState->lighting);
    ViewState->guiWidgets[GUI_LIGHTING] = wid;
    
    x = xo;
    y -= PFUGUI_SLIDER_YINC;
    ySize = PFUGUI_SLIDER_YSIZE;
    xSize = PFUGUI_BUTTON_VLONG_XSIZE + PFUGUI_BUTTON_VLONG_XINC;
    
    /* Slider for fog near range */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER_LOG, GUI_NEAR_FOG_RANGE);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Near Fog");
    pfuWidgetRange(wid, 1, 0.0f, ViewState->far*8.0f, ViewState->nearFogRange);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_NEAR_FOG_RANGE] = wid;
    if (!ViewState->fog)
	pfuDisableWidget(ViewState->guiWidgets[GUI_NEAR_FOG_RANGE]);
    
    y -= PFUGUI_SLIDER_YINC;
    
    /* Slider for fog far range */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER_LOG, GUI_FAR_FOG_RANGE);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Far Fog");
    pfuWidgetRange(wid, 1, 0.0f, ViewState->far*8.0f, ViewState->farFogRange);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_FAR_FOG_RANGE] = wid;
    if (!ViewState->fog)
	pfuDisableWidget(ViewState->guiWidgets[GUI_FAR_FOG_RANGE]);
    
    y -= PFUGUI_SLIDER_YINC;
    
    /* Slider for Time of Day */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_TOD);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Time Of Day");
    pfuWidgetRange(wid, 1, 0.0f, 1.0f, 0.8f);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_TOD] = wid;
    
    y -= PFUGUI_SLIDER_YINC;

    /* Global LOD Scale Slider */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER_LOG, GUI_LOD_SCALE);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "LOD Scale");
    pfuWidgetRange(wid, 1, 0.0f, 500.0f, ViewState->LODscale);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_LOD_SCALE] = wid;

    if (ViewState->haveMultisample)
    {
	
	if (ViewState->guiFormat == GUI_HORIZONTAL)
	{
	    xo += 2*PFUGUI_BUTTON_VLONG_XINC;
	    y = ys;
	}
	
	x = xo;
	y -= PFUGUI_BUTTON_YINC;

	xSize = PFUGUI_BUTTON_VLONG_XSIZE;
	ySize = PFUGUI_BUTTON_YSIZE;

	/* toggle Anti-Aliasing*/
	wid = pfuNewWidget(panel, PFUGUI_SWITCH, GUI_AA);
	pfuWidgetLabel(wid,"Anti-Alias");
	pfuWidgetDim(wid, x, y, xSize, ySize);
	pfuWidgetActionFunc(wid, controlPanel);
    	pfuWidgetDefaultValue(wid, ViewState->aa);
	
	x += PFUGUI_BUTTON_VLONG_XINC;
	
	/* LOD Fade button */
	wid = pfuNewWidget(panel, PFUGUI_SWITCH, GUI_LOD_FADE);
	pfuWidgetDim(wid, x, y, xSize, ySize);
	pfuWidgetLabel(wid, "Fade LOD");
	pfuWidgetActionFunc(wid, controlPanel);
    	pfuWidgetDefaultValue(wid, ViewState->fade);
	if (!ViewState->fade)
	    pfuDisableWidget(wid);
	ViewState->guiWidgets[GUI_LOD_FADE] = wid;
	
	x = xo;
	y -= PFUGUI_SLIDER_YINC;
	
	xSize = PFUGUI_BUTTON_VLONG_XSIZE + PFUGUI_BUTTON_VLONG_XINC;
	ySize = PFUGUI_SLIDER_YSIZE;
	wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_LOD_FADE_RANGE);
	pfuWidgetDim(wid, x, y, xSize, ySize);
	pfuWidgetLabel(wid, "LOD Fade Scale");
	pfuWidgetRange(wid, 1, 0.0f, 200.0f, ViewState->fadeRange);
	pfuWidgetActionFunc(wid, controlPanel);
	ViewState->guiWidgets[GUI_LOD_FADE_RANGE] = wid;
	if (!ViewState->fade)
	    pfuDisableWidget(wid);
    }
    
    xSize = PFUGUI_BUTTON_VLONG_XSIZE + PFUGUI_BUTTON_VLONG_XINC;
    ySize = PFUGUI_SLIDER_YSIZE;
    
    y -= PFUGUI_SLIDER_YINC;
    
    /* Slider for Show Tree X-Pan */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_PANX);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Show Tree X Pan");
    pfuWidgetRange(wid, 1, -1.0f, 1.0f, 0.0f);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_PANX] = wid;
    if (!ViewState->tree)
	pfuDisableWidget(wid);
    y -= PFUGUI_SLIDER_YINC;
    
    /* Slider for Show Tree Y-Pan */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_PANY);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Show Tree Y Pan");
    pfuWidgetRange(wid, 1, 0.0f, 1.0f, 0.0f);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_PANY] = wid;
    if (!ViewState->tree)
	pfuDisableWidget(wid);
    y -= PFUGUI_SLIDER_YINC;
    
    /* Slider for Show Tree Scale */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_SCALE);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Show Tree Scale");
    pfuWidgetRange(wid, 1, 0.01f, 2.0f, 1.0f);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_SCALE] = wid;
    if (!ViewState->tree)
	pfuDisableWidget(wid);
    
    y -= PFUGUI_BUTTON_YINC;

    xSize = PFUGUI_BUTTON_VLONG_XSIZE;
    ySize = PFUGUI_BUTTON_YSIZE;

    /* Tree display */
    wid = pfuNewWidget(panel, PFUGUI_SWITCH, GUI_TREE);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Show Tree");
    pfuWidgetActionFunc(wid, controlPanel);
    pfuWidgetDefaultValue(wid, ViewState->tree);
    ViewState->guiWidgets[GUI_TREE] = wid;

    x += PFUGUI_BUTTON_VLONG_XINC;
    
    /* Stress button */
    wid = pfuNewWidget(panel, PFUGUI_SWITCH, GUI_STRESS);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Stress");
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_STRESS] = wid;

    if (ViewState->guiFormat == GUI_HORIZONTAL)
    {
	x = xo += 2*PFUGUI_BUTTON_VLONG_XINC;
	y = ys;
    } 
    
    y -= PFUGUI_SLIDER_YINC;
    x = xo;
    
    /* Stress Low Slider */
    xSize = PFUGUI_BUTTON_VLONG_XSIZE + PFUGUI_BUTTON_VLONG_XINC;
    ySize = PFUGUI_SLIDER_YSIZE;
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_STRESS_LOW);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Stress Low");
    pfuWidgetRange(wid, 1, 0.0f, 1.0f, ViewState->lowLoad);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_STRESS_LOW] = wid;
    if (!ViewState->stress)
	pfuDisableWidget(wid);
    
    y -= PFUGUI_SLIDER_YINC;
    
    /* Stress High Slider */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_STRESS_HIGH);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Stress High");
    pfuWidgetRange(wid, 1, 0.0f, 1.0f, ViewState->highLoad);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_STRESS_HIGH] = wid;
    if (!ViewState->stress)
	pfuDisableWidget(wid);
    
    y -= PFUGUI_SLIDER_YINC;
    
    /* Stress Max Slider */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_STRESS_MAX);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Stress Max");
    pfuWidgetRange(wid, 1, 0.0f, 500.0f, ViewState->stressMax);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_STRESS_MAX] = wid;
    if (!ViewState->stress)
	pfuDisableWidget(wid);
    
    y -= PFUGUI_SLIDER_YINC;
    
    /* Stress Scale Slider */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_STRESS_SCALE);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Stress Scale");
    pfuWidgetRange(wid, 1, 0.0f, 4.0f, ViewState->stressScale);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_STRESS_SCALE] = wid;
    if (!ViewState->stress)
	pfuDisableWidget(wid);
    
    y -= PFUGUI_SLIDER_YINC;
    
    /* Pixel Limit Slider */
    wid = pfuNewWidget(panel, PFUGUI_SLIDER, GUI_PIX_LIMIT);
    pfuWidgetDim(wid, x, y, xSize, ySize);
    pfuWidgetLabel(wid, "Stress Pixel Limit");
    pfuWidgetRange(wid, 1, -1.0f, 1280.0f, -1.0f);
    pfuWidgetActionFunc(wid, controlPanel);
    ViewState->guiWidgets[GUI_PIX_LIMIT] = wid;
    
    /* Reset GUI so that it will come up with the specified default values */
    pfuResetGUI();
    
    return(panel);
}
