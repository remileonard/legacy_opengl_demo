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
 *	perfly.c -- The Performer database fly-thru application
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bstring.h>
#include <math.h>
#include <ctype.h>

#include <Performer/pf.h>
#include <Performer/prmath.h>
#include <Performer/pfdu.h>
#include <Performer/pfutil.h>
#include <Performer/pfui.h>

#include "generic.h"
#include "perfly.h"
#include "gui.h"
#include "keybd.h"

#ifndef	IRISGL
#include <GL/glu.h>
#endif

	/*---------------------- Globals ------------------------*/

int PackedAttrFormat = 0 /* check out PFGS_PA_C4UBN3ST2FV3F == 1 */;

	/*----------------------------------------------------*/

/*
 * No custom channel initialization 
 */
void
initChannel(pfChannel *chan)
{
    if (chan == ViewState->masterChan)
    {
	pfPipeVideoChannel *vChan = pfGetChanPVChan(chan);
	pfGetPVChanSize(vChan, &ViewState->vChanXSize, 
				&ViewState->vChanYSize);
	if (ViewState->dvrXSize < 0)
	{
	    ViewState->dvrXSize = ViewState->vChanXSize;
	    ViewState->dvrYSize = ViewState->vChanYSize;
	}
    }
}

/*
 * Initialize ViewState shared memory structure
 */
void
initViewState(void)
{
    pfWindow	*win;
    int		i, iR=0;
    int 	tmp;
    int		queryvals[4];
static int querytoks[4] = {PFQFTR_MULTISAMPLE, PFQFTR_TAG_CLEAR, PFQFTR_TEXTURE, 0};

    /* window needed for queries and getting RENDERER string */
    win = pfOpenNewNoPortWin("Query", -1);

    /* query machine features */
    pfMQueryFeature(querytoks, queryvals);
    ViewState->haveMultisample = queryvals[0];
    ViewState->haveTagClear = queryvals[1];
    tmp = queryvals[2];

#ifndef IRISGL
    iR = (strncasecmp(glGetString(GL_RENDERER), "IR", 2) == 0);
#endif

    if (tmp == PFQFTR_FAST)
	ViewState->texture = TRUE;
    else
	ViewState->texture = FALSE;

    /* Set up defaults */
    ViewState->aa		= ViewState->haveMultisample;
    ViewState->fade		= ViewState->haveMultisample;
    ViewState->visualID		= -1;

    ViewState->exitFlag		= FALSE;
    ViewState->updateChannels	= TRUE;
    ViewState->gui		= TRUE;
    ViewState->guiFormat	= 0; /* marks unset */
    ViewState->phase		= PFPHASE_FREE_RUN;
    ViewState->frameRate	= 30.0f;
    ViewState->backface		= PF_ON;
    ViewState->collideMode	= PF_ON;
    ViewState->earthSkyMode	= PFES_FAST;
#ifdef	PERFORMER_1_2_BACKGROUND_COLOR
    pfSetVec4(ViewState->earthSkyColor, 0.5f, 0.5f, 0.8f, 1.0f);
#else
    pfSetVec4(ViewState->earthSkyColor, 0.5f*0.14f, 0.5f*0.22f, 0.5f*0.36f, 1.0f);
#endif
    pfSetVec4(ViewState->scribeColor, 1.0f, 1.0f, 1.0f, 1.0f);
    ViewState->fadeRange	= 15.0f;
    ViewState->near		= 0.0f;
    ViewState->far		= 0.0f;
    ViewState->nearFogRange	= ViewState->near;
    ViewState->farFogRange	= ViewState->far;
    ViewState->fog		= PFFOG_OFF;
    ViewState->timeOfDay	= 0.8f;
    ViewState->highLoad		= 0.8f;
    ViewState->lighting		= LIGHTING_EYE;
    ViewState->lowLoad		= 0.5f;
    ViewState->stats		= PF_ON;
    ViewState->statsEnable	= PFSTATS_OFF;
    ViewState->stress		= PF_OFF;
    ViewState->stressMax	= 100.0f;
    ViewState->stressScale	= 1.0f;
    ViewState->drawStyle	= PFUSTYLE_FILLED;
    ViewState->xformerModel	= PFITDF_TRACKBALL;
    ViewState->resetPosition	= POS_ORIG;
#ifdef IRISGL
    ViewState->input		= PFUINPUT_GL;
#else /* OPENGL */
    ViewState->input		= PFUINPUT_X;
#endif /* GL type */
    ViewState->explode		= 0.0f;
    for (i=0; i < MAX_PIPES; i++)
	ViewState->drawFlags[i] = REDRAW_WINDOW;
    ViewState->redrawOverlay	= 3;
    ViewState->procLock		= 0;
    ViewState->cullMode		= PFCULL_ALL;
    ViewState->LODscale		= 1.0f;
    ViewState->lamps		= 0;
    strcpy(ViewState->welcomeText, "IRIS\nPerformer");
    strcpy(ViewState->overlayText, "IRIS Performer");
    ViewState->objFontType 	= PFDFONT_EXTRUDED;
    ViewState->objFontName      = strdup("Mistr");
    ViewState->optimizeGStates  = TRUE;    
    ViewState->optimizeTree     = 0;

    pfSetVec3(ViewState->panScale, 0.0f, 0.0f, 1.0f); 

    ViewState->rotateCenter	= 1;
    ViewState->iterate		= 1;

    ViewState->combineBillboards= 0;

    ViewState->printStats	= FALSE;
    ViewState->exitCount	= -1;

    ViewState->drawMode         = (iR ? DLIST_GEOSET : DLIST_OFF);

    ViewState->doDVR		= DVR_OFF;
    ViewState->winXSize		= WinSizeX;
    ViewState->winYSize		= WinSizeY;
    ViewState->vChanXSize	= 1280; /* defaults - updated in initChan */
    ViewState->vChanYSize	= 1024;
    ViewState->dvrXSize		= -1;
    ViewState->dvrYSize		= -1;

    pfMakeIdentMat(ViewState->viewMat);

    pfCloseWin(win);
    pfDelete(win);
}

/*
 * Initialize the view
 */
void
initView(pfScene *scene)
{
    pfSphere	bsphere;

    /* If asked to run for zero frames, exit now */    
    if (ViewState->exitCount == 0)
	pfExit();

    pfGetNodeBSphere(scene, &bsphere);

    /* Set initial view position from command line values */
    if (InitXYZ)
        pfCopyVec3(ViewState->viewCoord.xyz, ViewState->initView.xyz);

    /* Otherwise fit view to encompass scene */
    else
    {
        pfCopyVec3(ViewState->viewCoord.xyz, bsphere.center);

        /* Offset view so all visible */
        ViewState->viewCoord.xyz[PF_Y] -= ViewState->sceneSize;

        /* Store initial XYZ position for future reset actions */
        pfCopyVec3(ViewState->initView.xyz, ViewState->viewCoord.xyz);
    }

    if (ViewState->near == 0.0f)
    {
	/* Calculate near, far based on scene extent */
	ViewState->near = PF_MIN2(ViewState->sceneSize / 10.0f, 1.0f);
	ViewState->far = PF_MAX2(ViewState->sceneSize * 2.0f, 1000.0f);
	ViewState->nearFogRange = ViewState->near;
	ViewState->farFogRange = ViewState->far;
    }

    /* Set initial view angles from command line values ? */
    if (InitHPR)
        pfCopyVec3(ViewState->viewCoord.hpr, ViewState->initView.hpr);
    else
    {
	pfSetVec3(ViewState->viewCoord.hpr, 0.0f, 0.0f, 0.0f);

	/* Store initial HPR angles for future reset actions */
	pfCopyVec3(ViewState->initView.hpr, ViewState->viewCoord.hpr);
    }
}

int
initSceneGraph(pfScene *scene)
{
    pfNode     *root;
    int		i;
    int		j;
    int		loaded	= 0;
    pfSphere    bsphere;

    /* Create a DCS for TRACKBALL pfiXformer */
    ViewState->sceneDCS = pfNewDCS();
    ViewState->sceneGroup = pfNewGroup();
    pfAddChild(ViewState->sceneDCS, ViewState->sceneGroup);
    if (ViewState->xformerModel == PFITDF_TRACKBALL)
    {
	pfAddChild(scene, ViewState->sceneDCS);
    }
    else
	pfAddChild(scene, ViewState->sceneGroup);

    /* Load each of the files named on the command line */
    for (i = 0;	i < NumFiles; i++)
    {
	for (j = 0; j < ViewState->iterate; j++)
	{
	    /* Load the database. create a hierarchy under node "root" */
	    root = pfdLoadFile(DatabaseFiles[i]);

	    if (root == NULL)
	    {
		pfNotify(PFNFY_NOTICE, PFNFY_PRINT,
			 "WARNING: could not load \"%s\"", DatabaseFiles[i]);
		continue;
	    }
	    
	    /* explode the object (move it from the origin) */
	    if (ViewState->explode > 0.0f)
	    {
		float	 x;
		float	 y;
		float	 z;
		pfMatrix matrix;
		pfSCS	*scs;
		
		x = ViewState->explode*((random() & 0xffff)/65535.0f - 0.5f);
		y = ViewState->explode*((random() & 0xffff)/65535.0f - 0.5f);
		z = ViewState->explode*((random() & 0xffff)/65535.0f - 0.5f);
		
		/* move the object by placing it beneath an SCS node */
		pfMakeTransMat(matrix, x, y, z);
		scs = pfNewSCS(matrix);
		pfAddChild(scs, root);
		
		/* go ahead and apply the SCS matrix to the geometry */
		pfFlatten(scs, 0);
		
		/* detach (transformed) object from SCS; discard SCS */
		pfRemoveChild(scs, root);
		pfDelete(scs);
	    }
	    
	    /* optimize the newly loaded hierarchy */
	    if (ViewState->optimizeTree > 0)
	    {
		if (ViewState->optimizeTree > 1)
		{
		    /* convert ordinary pfDCS nodes to pfSCS nodes */
		    root = pfdFreezeTransforms(root, NULL);
		}
		/* discard all redundant nodes and identity pfSCS nodes */
		pfFlatten(root, 0);
		
		/* deinstance and apply all pfSCS transformations */
		root = pfdCleanTree(root, NULL);
		
 		/* try a parition for intersections */
		if (ViewState->optimizeTree > 2)
		{
		    pfPartition *part = pfNewPart();

		    if (ViewState->optimizeTree == 3)
		    {
			/* use approx 100x100 partition */
			/* don't try to build a perfect fit */

			pfVec3 spacing;
			pfSphere sphere;
			pfGetNodeBSphere(root, &sphere);
			
			PFSET_VEC3(spacing, 
				   0.01f*sphere.radius, 
				   0.01f*sphere.radius,
				   0.01f*sphere.radius);
			pfPartAttr(part, PFPART_ORIGIN, sphere.center);
			pfPartAttr(part, PFPART_MIN_SPACING, spacing);
			pfPartAttr(part, PFPART_MAX_SPACING, spacing);
		    }

		    pfNotify(PFNFY_INFO, PFNFY_PRINT,
			     "Creating pfPartition for %s", 
			     DatabaseFiles[i]);
		    
		    if (pfGetNotifyLevel() >= PFNFY_DEBUG)
			pfPartVal(part, PFPART_DEBUG, 1);

		    pfAddChild(part, root);
		    pfBuildPart(part);
		    pfUpdatePart(part);
		    root = (pfNode *)part;
		}
	    }
	    
	    pfAddChild(ViewState->sceneGroup, root);
	    ++loaded;
	}
    }

#if 0
    /* discard DSO-based loaders if any have been bound to executable */
    pfdFreeFileLoaders();
#endif

    /* 
     * optimize sharing of geostate structures and components. 
     * Wait until all files are loaded so sharing encompasses 
     * entire scene. 
     */
    pfdMakeShared((pfNode *)ViewState->sceneGroup);

    /* optimize pfLayer nodes via "DISPLACE POLYGON" */
    pfdCombineLayers((pfNode *)ViewState->sceneGroup);

    /* combine sibling pfBillboard nodes */
    if (ViewState->combineBillboards)
	pfdCombineBillboards((pfNode *)ViewState->sceneGroup, 
	    ViewState->combineBillboards);

    /* write out nodes in sceneGroup */
    if (WriteFile)
    {
        if (!strstr(WriteFile, ".out") && pfdInitConverter(WriteFile))
        {
            /* it's a valid extension, go for it */
            if (!pfdStoreFile((pfNode *)ViewState->sceneGroup, WriteFile))
                pfNotify(PFNFY_WARN, PFNFY_RESOURCE,
                         "No store function available for %s\n", WriteFile);
        }
        else
        {
            /* write the file as ASCII */
            FILE *fp;
            if (fp = fopen(WriteFile, "w"))
            {
                pfPrint(ViewState->sceneGroup, PFTRAV_SELF|PFTRAV_DESCEND, 
		    PFPRINT_VB_OFF, fp);
                fclose(fp);
            }
            else
                pfNotify(PFNFY_WARN, PFNFY_RESOURCE,
                         "Could not open scene.out for debug printing.");
        }

	/* remember that file's already been written */
	WriteFile = NULL;
    }

    /*
     * make an optimizing scene geostate that represents the
     * most common modes and attributes present in the scene
     * graph.
     */
    if (ViewState->optimizeGStates)
    {
	/* compute an optimum scene geostate */
	pfdMakeSharedScene(scene);
    }
    else
    {
	/* use default builder state as scene geostate */
	pfGeoState *gs;
	gs = pfNewGState(pfGetSharedArena());
	pfCopy(gs, (pfGeoState*)pfdGetDefaultGState());
	pfSceneGState(scene, gs);
    }

    /* Compute extent of scene */
    if (loaded)
    {
    	pfGetNodeBSphere(scene, &bsphere);
    	ViewState->sceneSize = PF_MIN2(2.5f*bsphere.radius, 80000.0f);
    }
    else
    	ViewState->sceneSize = 1000.0f;

    /* convert scene to vertex arrays */
    if (PackedAttrFormat)
    {
	pfNotify(PFNFY_NOTICE, PFNFY_PRINT, "Making Vertex Arrays.");
	/* don't delete any extra attribute arrays since they will be needed
	 * for wireframe mode
	 */
	pfuTravCreatePackedAttrs(scene, PackedAttrFormat, 0x0);
    }

    /* add light sources to the scene */
    for (i = 0; i < ViewState->lamps; i++)
    {
	pfLightSource *lamp = pfNewLSource();

	/* set direction of infinite light source */
	pfNormalizeVec3(ViewState->lampXYZ[i]);
	pfLSourcePos(lamp,
		     ViewState->lampXYZ[i][0],
		     ViewState->lampXYZ[i][1],
		     ViewState->lampXYZ[i][2],
		     0.0f);

	/* set light source color */
	pfLSourceColor(lamp,PFLT_DIFFUSE,
		       ViewState->lampRGB[i][0],
		       ViewState->lampRGB[i][1],
		       ViewState->lampRGB[i][2]);

	/* add lamp to scene */
	pfAddChild(scene, lamp);
    }

    /* Create geode/gset to draw box representing shrunken 
     * culling frustum.
     */
    {
	int		*lengths, i;
	pfGeoSet	*gset;
	pfGeoState	*gstate;
	pfVec3	*coords;
	pfVec4	*colors;
	ushort	*index;
	void	*arena = pfGetSharedArena();
	
	ViewState->cullVol = pfNewGeode();
	gset = pfNewGSet(arena);
	gstate = pfNewGState(arena);
	
	pfGSetPrimType(gset, PFGS_LINESTRIPS);
	pfGSetNumPrims(gset, 1);
	lengths = (int*)pfMalloc(sizeof(int), arena);
	lengths[0] = 5;
	pfGSetPrimLengths(gset, lengths);
	
	index = (ushort*)pfMalloc(sizeof(ushort)*5, arena);
	for (i=0; i<4; i++)
	    index[i] = i;
	index[i] = 0;
	coords = (pfVec3*)pfCalloc(4, sizeof(pfVec3), arena);
	pfGSetAttr(gset, PFGS_COORD3, PFGS_PER_VERTEX, coords, index);
	colors = (pfVec4*)pfMalloc(sizeof(pfVec4), arena);
	pfSetVec4(colors[0], 1.0f, 0.0f, 0.0f, 1.0f);
	pfGSetAttr(gset, PFGS_COLOR4, PFGS_OVERALL, colors, index);
	
	pfMakeBasicGState(gstate);
	pfGSetGState(gset, gstate);
	pfAddGSet(ViewState->cullVol, gset);
	ViewState->cullVolDCS = pfNewDCS();
	pfNodeTravMask(ViewState->cullVolDCS, PFTRAV_CULL, 0, PFTRAV_SELF, PF_SET);
	pfAddChild(ViewState->cullVolDCS, ViewState->cullVol);
    }
    
    return(loaded);
}

/*
 * Update the current view - latency critical operations
*/
void
updateView(pfChannel *chan)
{
    pfMatrix 		mat;

    if (ViewState->resetPosition)
    {
	resetPosition(ViewState->resetPosition);
	ViewState->resetPosition = 0;
    }

    pfiUpdateXformer(ViewState->xformer);
    
    ViewState->xformerModel = pfiGetXformerCurModelIndex(ViewState->xformer);

    /* if have moving-eyepoint motion model, update eyepoint */
    if (pfIsOfType(pfiGetXformerCurModel(ViewState->xformer), 
	    pfiGetIXformTravelClassType()))
    {
	pfiGetXformerCoord(ViewState->xformer, &ViewState->viewCoord);
        pfiGetXformerMat(ViewState->xformer, mat);
	pfChanViewMat(chan, mat);
	pfCopyMat(ViewState->viewMat, mat);
    
	/* move the sun to the eyepoint */
	if (ViewState->lighting ==  LIGHTING_EYE)
	    pfDCSMat(ViewState->sunDCS, ViewState->viewMat);
    }
    
    /* do video resize for all channels */
    if (pfGetFrameCount() > 3)
    {
	static int odvrMode = -1;
	static oxs=-1,  oys = -1;
	static int dvrMode = -1; 
	int i, newSize;

	newSize =  ((oxs != ViewState->dvrXSize) || (oys != ViewState->dvrYSize));
	if (dvrMode != ViewState->doDVR)
	{
	    ViewState->redrawOverlay	= 3;
	}
	dvrMode = ViewState->doDVR;
	
	for (i=0; i < ViewState->numChans; i++)
	{
	    pfChannel *chan;
	    pfPipe *pfpipe;
	    pfPipeVideoChannel *pvchan;
	    
	    chan = ViewState->chan[i];
	    pfpipe = pfGetChanPipe(chan);
	    pvchan = pfGetChanPVChan(chan);

	    if (pvchan)
	    {
		switch (dvrMode)
		{
		case DVR_OFF:
		    if (odvrMode != dvrMode)
		    {
			pfPVChanDVRMode(pvchan, PFPVC_DVR_OFF);
			pfuCursorType(PFU_CURSOR_X);
			pfChanProjMode(chan,PFCHAN_PROJ_VIEWPORT);
			pfuRedrawGUI();
		    }
		    break;
		case DVR_MANUAL:
		    /* set pfPipeVideoChannel scale factor for next draw */
		    {
			pfPVChanDVRMode(pvchan, PFPVC_DVR_MANUAL);
			pfPVChanStressFilter(pvchan,1.0f, 0.80, 0.95f, 0.0f, 1.0f, 1.0f);
			if (newSize)
			{
			    pfNotify(PFNFY_NOTICE,PFNFY_PRINT,"MANUAL RESIZE: %d %d", ViewState->dvrXSize, ViewState->dvrYSize);
			    pfPVChanOutputSize(pvchan, ViewState->dvrXSize, ViewState->dvrYSize);
			    pfChanProjMode(chan,PFCHAN_PROJ_WINDOW);
			}
			pfuCursorType(PFU_CURSOR_3D);
			if (ViewState->gui &&
				((newSize && (chan == ViewState->masterChan)) ||
				pfuInGUI(ViewState->mouse.xpos, ViewState->mouse.ypos)))
			    pfuRedrawGUI();
		    }
		    break;
		case DVR_AUTO:
		    {
			float frac=1.0f;
			if (!Multipipe)
			    frac = 1.0f / (float) NumChans;
			pfPVChanDVRMode(pvchan, PFPVC_DVR_AUTO);
			pfPVChanStressFilter(pvchan, frac, 0.80, 0.95f, 1.5f, 2.0f, 10.0f);
			/* indicates that pfPipe should calc stress automatically */
			if ((chan == ViewState->masterChan) && ViewState->gui)
			    pfuRedrawGUI();
			pfuCursorType(PFU_CURSOR_3D);
			pfChanProjMode(chan,PFCHAN_PROJ_WINDOW);
		    }
		    break;
		}
		pfGetPVChanOutputSize(pvchan, &ViewState->dvrXSize, &ViewState->dvrYSize);
	    }
	}
	odvrMode = dvrMode;
	oxs = ViewState->dvrXSize;
	oys = ViewState->dvrYSize;
    }
    
       
    {
    /*
     * Update box which represents shrunken culling frustum
     */
    static float	prevCullDelta = -1.0f;
    if (prevCullDelta != ViewState->cullDelta)
    {
	if (ViewState->cullDelta < 1.0f)
	{
	    pfVec3			*coords;
	    ushort			*foo;
	    int				i;
	    float			near, far;
	    static pfFrustum		*frust = NULL;

	    if (frust == NULL)
		frust = pfNewFrust(NULL);

	    pfGetGSetAttrLists(pfGetGSet(ViewState->cullVol, 0), 
			       PFGS_COORD3, (void**)&coords, &foo);

	    pfGetChanBaseFrust(ViewState->masterChan, frust);
	    pfGetFrustNearFar(frust, &near, &far);
	    pfGetFrustNear(frust, coords[0], coords[1], 
			          coords[3], coords[2]); 

	    for (i=0; i<4; i++)
	    {
		coords[i][0] *= ViewState->cullDelta;
		coords[i][1]  = near + 0.001f;
		coords[i][2] *= ViewState->cullDelta;
	    }

	    pfFrustNearFar(frust, near, far);
	    pfMakePerspFrust(frust, coords[0][0], coords[2][0], 
			            coords[0][2], coords[2][2]); 

	    pfChanCullPtope(ViewState->masterChan, (pfPolytope*)frust);

	    if (prevCullDelta >= 1.0f)
		pfAddChild(ViewState->scene, ViewState->cullVolDCS);
	}
	else	
	{
	    /* Don't draw culling box */
	    if (prevCullDelta < 1.0f)
		pfRemoveChild(ViewState->scene, ViewState->cullVolDCS);

	    /* Disable special cull region */
	    pfChanCullPtope(ViewState->masterChan, NULL);
	}

	prevCullDelta = ViewState->cullDelta;
    }

    /* Place cull volume geometry in front of viewer */
    if (ViewState->cullDelta < 1.0f)	
    {
	pfMatrix 		mat;
	pfGetChanOffsetViewMat(chan, mat);
	pfDCSMat(ViewState->cullVolDCS, mat);
    }
    }
}

/*
 * Reset view and Xformer to original XYZ position and HPR Euler angles
 */
void
resetPosition(int pos)
{
    pfiStopXformer(ViewState->xformer);
    switch (pos)
    {
    case POS_CENTER:
	pfiCenterXformer(ViewState->xformer);
	break;

    default:
    case POS_ORIG:
	pfiResetXformerPosition(ViewState->xformer);
	break;
    }
    pfiGetXformerMat(ViewState->xformer, ViewState->viewMat);
    pfChanViewMat(ViewState->masterChan, ViewState->viewMat);
}


static void
updateWin(int pipeId)
{
    static int	winIndex = PFWIN_GFX_WIN;
    int 	doWinIndex = 0;
    int 	i;

    /* Select proper visual in the window stack to render to  */
    if (ViewState->stats && 
	(ViewState->statsEnable & PFSTATSHW_ENGFXPIPE_FILL))
    {
	pfPipeVideoChannel *pvchan;
	ViewState->doDVR = PFPVC_DVR_OFF;
	for (i=0; i < ViewState->numChans; i++)
	{
	    pvchan = pfGetChanPVChan(ViewState->chan[i]);
	    pfPVChanDVRMode(pvchan, PFPVC_DVR_OFF);
	}
	if(winIndex != PFWIN_STATS_WIN)
	{
	    winIndex = PFWIN_STATS_WIN;
	    doWinIndex = 1;
	}
    }
    else 
    {
	if (winIndex != PFWIN_GFX_WIN)
	{
	    winIndex = PFWIN_GFX_WIN;
	    doWinIndex = 1;
	}
    }
    if (doWinIndex)
    {
	int	i;

	for (i=0; i < NumPipes; i++)
	{
	    pfPWinIndex(ViewState->pw[i], winIndex);
	    if (!(pfGetPWinSelect(ViewState->pw[i])))
	    {
		pfPWinConfigFunc(ViewState->pw[i], ConfigPWin);
		pfConfigPWin(ViewState->pw[i]);
	    }
	}
	ViewState->drawFlags[pipeId] |= UPDATE_MODES;
    }  
}


static void
updateStats(void)
{
    /*
     * Note: if turning on or off fill stats, we must redraw panel because
     * multisampling might be toggled as well.
    */
    static int     on = -1, en = 0;
    static int     first = 1;
    int i;

    pfFrameStats   *fsp;

    if (on && (!ViewState->stats))	/* Reset to minimal fast stats collection */
    {
	for (i=0; i < ViewState->numChans; i++)
	{
	    pfChannel *chan;
	    
	    chan = ViewState->chan[i];
	    fsp = pfGetChanFStats(chan);
	    /* put everything back to defaults */
	    pfFStatsClass(fsp, PFSTATS_ALL, PFSTATS_DEFAULT); 
	    /* set the fast process timing stats */
	    pfFStatsClassMode(fsp, PFFSTATS_PFTIMES, PFFSTATS_PFTIMES_BASIC, PFSTATS_SET);
	    on = 0;
	    en = 0;
	}
    }
    else if (ViewState->stats && (en != ViewState->statsEnable))
    {
	for (i=0; i < ViewState->numChans; i++)
	{
	    pfChannel *chan;

	    chan = ViewState->chan[i];

	    /* Get the stats structure for this channel */
	    fsp = pfGetChanFStats(chan);

	    /* First turn off old stats if not just gfx pipe stats */
	    if (en)
		pfFStatsClass(fsp, en & ~(PFSTATSHW_ENGFXPIPE_TIMES), PFSTATS_OFF);
	    if (ViewState->statsEnable & PFSTATSHW_ENGFXPIPE_FILL)
		pfFStatsClass(fsp, PFSTATSHW_ENGFXPIPE_TIMES, PFSTATS_OFF);

	    /* Now enable new stats graph */
	    if (ViewState->statsEnable)
	    {
		pfFStatsClass(fsp, ViewState->statsEnable, PFSTATS_ON);
		pfFStatsClassMode(fsp, PFFSTATS_PFTIMES, PFFSTATS_PFTIMES_MASK, PFSTATS_ON);
		if (ViewState->statsEnable & PFSTATSHW_ENGFXPIPE_TIMES) /* do just the status line */
		    pfChanStatsMode(chan, PFCSTATS_DRAW, 0);
		else
		    pfChanStatsMode(chan, PFCSTATS_DRAW, PFSTATS_ALL);
		if (first)
		{	/* set some stats class modes that are not
		     * on by default. this only needs to be done
		     * the first time */
		    pfFStatsClassMode(fsp, PFSTATSHW_GFXPIPE_FILL,
				     PFSTATSHW_GFXPIPE_FILL_DEPTHCMP |
				     PFSTATSHW_GFXPIPE_FILL_TRANSPARENT,
				     PFSTATS_ON);
		    /*
		     * This will also enable the tmesh statistics - not on by
		     * default because they are somewhat expensive
		     */
		    pfFStatsClassMode(fsp, PFSTATS_GFX,
				     PFSTATS_GFX_ATTR_COUNTS |
				     PFSTATS_GFX_TSTRIP_LENGTHS,
				     PFSTATS_ON);
		    pfFStatsAttr(fsp, PFFSTATS_UPDATE_SECS, 2.0f);
		    first = 0;
		}
		    
	    }
	}
	en = ViewState->statsEnable;
	on = 1;
    }
    if (ViewState->printStats)
    {
	if (!ViewState->stats)
	        ViewState->stats = TRUE;
	else
	{
	    pfPrint(pfGetChanFStats(ViewState->masterChan),
		    PFFSTATS_BUF_AVG | PFFSTATS_BUF_PREV |
		    pfGetFStatsClass(fsp, PFSTATS_ALL),
		    PFPRINT_VB_ON, 0);
	    ViewState->printStats=0;
	}
    }
}

    
/*
 * Perform non-latency-critical per-frame updates
 */
void
updateSim(pfChannel *chan)
{
    int 	i;
    int 	pipeId;    
    
    pipeId = pfGetId(pfGetChanPipe(chan));
    
    /* Process keyboard input */
    processKeybdInput();

    /* Adjust load management filter */
    if (ViewState->stress)
	pfChanStressFilter(chan, 1.0f, ViewState->lowLoad, ViewState->highLoad,
	    ViewState->stressScale, ViewState->stressMax);
    else
	pfChanStressFilter(chan,1.0f, ViewState->lowLoad, ViewState->highLoad,
	    0.0f, 1.0f);

    if (ViewState->updateChannels)
    {
	updateGUI(ViewState->gui);
	ViewState->updateChannels = 0;
    }
    else if (ViewState->gui || ViewState->tree)
	pfuUpdateGUI(&ViewState->mouse);
    else
	pfuUpdateGUICursor();

    if ((ViewState->cullMode != -1)&&
	    (ViewState->cullMode !=
	     pfGetChanTravMode(ViewState->masterChan, PFTRAV_CULL)))
	pfChanTravMode(ViewState->masterChan, PFTRAV_CULL, ViewState->cullMode);

    if (ViewState->LODscale != pfGetChanLODAttr(ViewState->masterChan, PFLOD_SCALE))
	pfChanLODAttr(ViewState->masterChan, PFLOD_SCALE, ViewState->LODscale);

    updateTimeOfDay(ViewState->timeOfDay);
    updateStats();
    updateEnvironment();
    updateWin(pipeId);

    pfGetChanViewMat(ViewState->masterChan, ViewState->viewMat);
 
    
    /* update display lists based on draw mode */
    {
    static int oldDrawMode = -1;

    if (ViewState->drawMode != oldDrawMode)
    {
	if (oldDrawMode == DLIST_GEOSET)
	{
	    pfuTravSetDListMode(ViewState->scene, 0, PFU_ATTRS_NONE);
	    /* printf("geoset display list disabled\n"); */
	}
	
	oldDrawMode = ViewState->drawMode;

	if (oldDrawMode == DLIST_GEOSET)
	{
	    pfuTravSetDListMode(ViewState->scene, 1, PFU_ATTRS_NONE);
	    /* printf("geoset display list enabled\n"); */
	}
    }
    }
}


void
xformerModel(int model, int collideMode)
{
    pfMatrix		mat;
    static int		oldModel = -1, oldCollMode = -1;

    if ((model == oldModel && oldCollMode == collideMode) || !ViewState->xformer)
	return;

    pfiSelectXformerModel(ViewState->xformer, model);

    /* If not in trackball mode, remove trackball DCS for better performance */
    if ((oldModel != model) && (oldModel == PFITDF_TRACKBALL))
    {
	pfRemoveChild(ViewState->scene, ViewState->sceneDCS);
	pfAddChild(ViewState->scene, ViewState->sceneGroup);
    }
    /* Restore trackball DCS to scene */
    else if ((oldModel != model) && (model == PFITDF_TRACKBALL))
    {
	pfRemoveChild(ViewState->scene, ViewState->sceneGroup);
	if (pfGetNumParents(ViewState->sceneDCS) == 0)
	    pfAddChild(ViewState->scene, ViewState->sceneDCS);
    }
    oldModel = model;
    
    /* Collide with objects in scene */
    if (oldCollMode != collideMode)
    {
	if (collideMode == PF_ON)
	    pfiEnableXformerCollision(ViewState->xformer);
	else
	    pfiDisableXformerCollision(ViewState->xformer);
	oldCollMode = collideMode;
    }
}


/******************************************************************************
 *			    Draw Process Routines
 *************************************************************************** */


static void
drawMessage(pfPipeWindow *pw)
{
    pfFont	*fnt;
    pfString	*str;
    pfLight	*lt;
    pfMatrix	mat;
    pfFrustum	*frust;
    const pfBox	*bbox;
    float	dist;
    double	start, t;

    fnt = pfdLoadFont_type1(ViewState->objFontName, ViewState->objFontType);

    if (!fnt)
	return;

    pfPushState();
    pfPushIdentMatrix();
    
    str = pfNewString(NULL);
    pfStringMode(str, PFSTR_JUSTIFY, PFSTR_MIDDLE);
    pfStringFont(str, fnt);
    pfStringColor(str, 1.0f, 0.0f, 0.8f, 1.0f);
    pfStringString(str, ViewState->welcomeText);
    pfMakeScaleMat(mat, 1.0f, 1.0f, 1.5f);
    pfStringMat(str, mat);
    pfFlattenString(str);
    bbox = pfGetStringBBox(str);
    dist = PF_MAX2(bbox->max[PF_Z] - bbox->min[PF_Z],
		   bbox->max[PF_X] - bbox->min[PF_X]);

    dist = (dist/2.0f) / pfTan(45.0f/2.0f);

    frust = pfNewFrust(NULL);

    if (InitFOV) {
	pfFrustNearFar(frust, 1.0f, 1000.0f);
	pfMakePerspFrust(frust, 
		- pfTan (ViewState->fov[0]/2.0f),
		  pfTan (ViewState->fov[0]/2.0f),
		- pfTan (ViewState->fov[1]/2.0f),
		  pfTan (ViewState->fov[1]/2.0f) );
    } else
	pfMakeSimpleFrust(frust, 45.0f);

    pfApplyFrust(frust);
    pfDelete(frust);

    /* Convert from GL to Performer space */
    pfRotate(PF_X, -90.0f);

    pfEnable(PFEN_LIGHTING);
    lt = pfNewLight(NULL);
    pfLightPos(lt, .2f, -1.0f, 1.0f, 0.0f);
    pfLightOn(lt);

    start = pfGetTime();

    /* Twirl text for 0.25f seconds */
    do
    {
	pfClear(PFCL_COLOR | PFCL_DEPTH, NULL);
	
	t = (pfGetTime() - start) / 0.25f;
	t = PF_MIN2(t, 1.0f);

	pfMakeRotMat(mat, (1.0f - 0.5f*t) * -90.0f, 
		     1.0f, 0.0f, 0.0f); 

	t *= t;
	pfPostTransMat(mat, mat, 
		       0.0f, 
		       3.0f * t * dist +  6.0f * (1.0f - t) * dist,
		       0.0f);

	pfPushMatrix();
	pfMultMatrix(mat);
	pfDrawString(str);
	pfPopMatrix();

	pfSwapPWinBuffers(pw);

    } while (t < 1.0f);

    /* Get image in both front and back buffers */
    pfClear(PFCL_COLOR | PFCL_DEPTH, NULL);
    pfPushMatrix();
    pfMultMatrix(mat);
    pfDrawString(str);
    pfPopMatrix();

    pfSwapPWinBuffers(pw);

    pfLightOff(lt);

    pfPopMatrix();
    pfPopState();

    pfDelete(lt);
    pfDelete(str);
}

/* this will only be initialized in the draw process */
static int Overlay = -1;

void
initPipe(pfPipeWindow *pw)
{
    char	path[PF_MAXSTRING];

    static char msg[] = "Welcome to IRIS Performer";


#ifdef IRISGL
    /* Check for overlay bitplanes */
    if (ViewState->input == PFUINPUT_GL)
        Overlay = (getgdesc(GD_BITS_OVER_SNG_CMODE) ? OVERDRAW :
	      	  (getgdesc(GD_BITS_PUP_SNG_CMODE)  ? PUPDRAW  : 0));
#else /* OPENGL */ /* XXX query for overlay planes in OpenGL ??? */
	Overlay = (pfGetPWinOverlayWin(pw) ? 1 : 0);
#endif /* GL type */

    if ((ViewState->welcomeText[0] == '\0') ||
	(!(pfFindFile("Times-Elfin.of", path, R_OK))))
    {

	pfClear(PFCL_COLOR, NULL);
	/* Message will be draw in in purple - the pfuDrawMessage() color */
	pfuDrawMessage(NULL, msg, PFU_MSG_PIPE, PFU_CENTER_JUSTIFIED,
	    0.5f, 0.5f, PFU_FONT_BIG, PFU_RGB);
	pfSwapPWinBuffers(pw);

	pfClear(PFCL_COLOR, NULL);
	/* Message will be draw in in purple - the pfuDrawMessage() color */
	pfuDrawMessage(NULL, msg, PFU_MSG_PIPE, PFU_CENTER_JUSTIFIED,
	    0.5f, 0.5f, PFU_FONT_BIG, PFU_RGB);
	pfSwapPWinBuffers(pw);
    }
    else
	drawMessage(pw);
}

static void
snapImage(int snapAlpha)
{
    FILE 	*fp;
    static  char str[80];
    static  count = 0;
    pfPipe* cpipe = pfGetChanPipe(ViewState->masterChan);
    int    	id = pfGetId(cpipe)*10 + count++;

    sprintf(str, "perfly.%d.%s", id,
	    (snapAlpha) ? "rgba" : "rgb");
    pfNotify(PFNFY_INFO, PFNFY_PRINT, "Saving pipe %d image in file %s\n",
	    pfGetId(cpipe), str);

    /* read from FRONT-buffer to get channel stats info */
    pfuSaveImage(str, 0, 0,
		 ViewState->mouse.winSizeX,
		 ViewState->mouse.winSizeY,
		 (snapAlpha) ? 1 : 0);

    /* create info file to go with image */
    sprintf(str, "perfly.%d.info", id);
    if (fp = fopen(str,"w"))
    {
	float h, v;
	fprintf(fp, "Viewing parameters for snap %d of perfly\n\n", id);
	fprintf(fp, "XYZ: %f %f %f\n",
		ViewState->viewCoord.xyz[0],
		ViewState->viewCoord.xyz[1],
		ViewState->viewCoord.xyz[2]);
	fprintf(fp, "HPR: %f %f %f\n",
		ViewState->viewCoord.hpr[0],
		ViewState->viewCoord.hpr[1],
		ViewState->viewCoord.hpr[2]);
	fprintf(fp, "NEAR/FAR: %f %f \n",
		ViewState->near, ViewState->far);
	pfGetChanFOV(ViewState->masterChan, &h, &v);
	fprintf(fp, "FOV: horiz=%f vert=%f\n", h, v);
	fclose(fp);
    }

    pfNotify(PFNFY_INFO, PFNFY_PRINT, "Done\n");
}

static void
drawOverlayName(pfPipeWindow *pw)
{
    pfWindow 		*pwOver = pfGetPWinOverlayWin(pw);
    static int 		mapped = 0;

    pfPushState();
    pfBasicState();
    
#ifndef IRISGL    
    if(ViewState->doDVR)
    {
	/* set the viewport to the window to completely clear the overlays */
	int xs, ys;
	pfGetPWinSize(pw, &xs, &ys);
	glViewport(0, 0, xs, ys);
    }
#endif

#ifdef IRISGL
    if (ViewState->input == PFUINPUT_GL)
	drawmode(Overlay);
    else  /* !!!!! */
#endif  /* GL type */
    {
	if (pwOver)
	    pfSelectWin(pwOver);
	else
	    return;
    }

#ifdef IRISGL
    if (ViewState->input == PFUINPUT_GL)
    {
	color(0);
	clear();
    }
    else /* !!!!! */
#endif  /* GL type */
	pfClear(PFCL_COLOR, NULL);
	
    if(!ViewState->doDVR)
    {

    if (!mapped)
    {
	static pfVec3 clrs[2] = { 
	            {0.4f, 0.0f, 0.5f},		/* tex - purple */
                    {0.125f, 0.06f, 00.125f}};	/* shadow - drk grey */
	pfuMapWinColors(pwOver, clrs, 1, 2);
	mapped = 1;
    }	
    pfuDrawMessageCI(ViewState->masterChan,
	    ViewState->overlayText, PFU_MSG_CHAN, PFU_RIGHT_JUSTIFIED,
	    1.0f, 1.0f, PFU_FONT_SMALL, 1, 2);
	    	    
#ifdef IRISGL
    if (ViewState->input == PFUINPUT_GL)
	drawmode(NORMALDRAW);
#endif /* GL type */

    }

    pfSelectPWin(pw);

    pfPopState();

}


/* convey draw style from localPreDraw to localPostDraw */
static int selectedDrawStyle = 0;

void
localPreDraw(pfChannel *chan, void *data)
{
    pfPipe *pipe = pfGetChanPipe(chan);
    pfPipeWindow *pw = pfGetChanPWin(chan);
    int pipeId = pfGetId(pipe);
    
    if (ViewState->exitCount == 0)
	ViewState->exitFlag = TRUE;

    if (ViewState->exitCount > 0)
	ViewState->exitCount--;

    if (ViewState->printStats > 0)
	ViewState->printStats--;

    /* take snapshot of image */
    if (ViewState->snapImage)
    {
	snapImage(ViewState->snapImage == 2);
	ViewState->snapImage = 0;
    }

    /*
     * Update new draw modes
     */
    if (ViewState->drawFlags[pipeId] & UPDATE_MODES)
    {
	{ /* put this first because it may destroy the old win and
	   * create a new one (in GLX mode) which will require a
	   * window redraw.
	   */
	    static aa=-1;
	    if (aa == -1)
		aa = ViewState->aa;
	    else if (aa != ViewState->aa)
	    {
		if (ViewState->aa)
		    pfAntialias(PFAA_ON);
		else
		    pfAntialias(PFAA_OFF);
		aa = ViewState->aa;
	    }
	}

	if (ViewState->backface)
	{
	    /* leave backface removal to loader state */
	    pfOverride(PFSTATE_CULLFACE, 0); 
	    pfCullFace(PFCF_BACK);
	}
	else
	{
	    pfCullFace(PFCF_OFF);
	    /* really force backface removal off */
	    pfOverride(PFSTATE_CULLFACE, 1); 
	}

	if (ViewState->lighting == LIGHTING_OFF)
	{
	    pfOverride(PFSTATE_ENLIGHTING | PFSTATE_FRONTMTL, 0);
	    pfDisable(PFEN_LIGHTING);
	    pfOverride(PFSTATE_ENLIGHTING | PFSTATE_FRONTMTL, 1);
	}
	else
	    pfOverride(PFSTATE_ENLIGHTING | PFSTATE_FRONTMTL, 0);

	if (ViewState->texture == FALSE)
	{
	    pfOverride(PFSTATE_ENTEXTURE | PFSTATE_TEXTURE, 0);
	    pfDisable(PFEN_TEXTURE);
	    pfOverride(PFSTATE_ENTEXTURE | PFSTATE_TEXTURE, 1);
	}
	else
	    pfOverride(PFSTATE_ENTEXTURE | PFSTATE_TEXTURE, 0);

	ViewState->drawFlags[pipeId] &= ~UPDATE_MODES;
    }

    /* Handle REDRAW event */
    if (ViewState->drawFlags[pipeId] & REDRAW_WINDOW)
    {
	/* master channel controls master pipe */
	if (pipeId == (pfGetId(pfGetChanPipe(ViewState->masterChan))))
	{
	    /* Only draw message into master channel */
	    if (chan == ViewState->masterChan)
	    {
		if (ViewState->redrawOverlay && Overlay)
		{
		    drawOverlayName(pw);
		    ViewState->redrawOverlay--;
		}
		else
		    ViewState->drawFlags[pipeId] &= ~REDRAW_WINDOW;
	    }
	}
	else
	    ViewState->drawFlags[pipeId] &= ~REDRAW_WINDOW;


	pfClear(PFCL_DEPTH, NULL); /* needed for Extreme when move window 
				       * with sky-ground earth-sky
				       */
    }

    if (ViewState->gui)
	updateGUIViewMsg();

    /* 
     * remember draw style in case it changes between now 
     * and the time localPostDraw() gets called.
     */
    selectedDrawStyle = ViewState->drawStyle;

    /* handle draw style */
    pfuPreDrawStyle(selectedDrawStyle, ViewState->scribeColor);
}

void
localPostDraw(pfChannel *chan, void *data)
{
    pfPipe *mPipe, *cPipe;
    
    /* handle draw style */
    pfuPostDrawStyle(selectedDrawStyle);

    mPipe = pfGetChanPipe(ViewState->masterChan);
    cPipe = pfGetChanPipe(chan);

    if (cPipe == mPipe)
    {

	/* draw scene-graph hierarchy */
	if (ViewState->tree && (chan == ViewState->masterChan))
	    pfuDrawTree(chan, (pfNode*)ViewState->scene, ViewState->panScale);

	/* draw 3D cursor */
	if ((pfuGetCursorType() == PFU_CURSOR_3D) && ViewState->mouse.inWin)
	{
	    pfPipeWindow *pw = pfGetChanPWin(chan);
	    pfuGUICursor(PFU_CURSOR_MAIN, PFU_CURSOR_OFF);
	    pfuGUICursor(PFU_CURSOR_GUI, PFU_CURSOR_OFF);
	    pfuDrawPWin3DCursor(pw, ViewState->mouse.xpos, ViewState->mouse.ypos);
	}
	else
	{
	    pfuGUICursor(PFU_CURSOR_MAIN, PFU_CURSOR_DEFAULT);
	    pfuGUICursor(PFU_CURSOR_GUI, PFU_CURSOR_DEFAULT);
	}

    }
}
	

void
initXformer(void)
{
    ViewState->xformer = pfiNewTDFXformer(pfGetSharedArena());

    pfiXformerAutoInput(ViewState->xformer, ViewState->masterChan,
	&ViewState->mouse, &ViewState->events);

    pfiXformerNode(ViewState->xformer, ViewState->sceneGroup);
    pfiXformerAutoPosition(ViewState->xformer, NULL, ViewState->sceneDCS);
    pfiXformerResetCoord(ViewState->xformer, &ViewState->initView);
    xformerModel(ViewState->xformerModel, ViewState->collideMode);

    resetPosition(POS_ORIG);
}

void
drawModesChanged(void)
{
    int	i;

    for (i=0; i<NumPipes; i++)
	ViewState->drawFlags[i] |= UPDATE_MODES;
}

void
preApp(pfChannel *chan, void *data)
{
    (chan, data);
    return;
}

void
postApp(pfChannel *chan, void *data)
{
    (chan, data);
    return;
}

void
preCull(pfChannel *chan, void *data)
{
    (chan, data);
    return;
}

void
postCull(pfChannel *chan, void *data)
{
    (chan, data);
    return;
}

