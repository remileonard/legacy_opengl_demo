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
 * env.c	environment functions, ie. weather, time of day, etc
 *
 * $Revision: 1.38 $
 * $Date: 1995/11/22 14:33:16 $
 *
 */

#include <math.h>
#include <Performer/pf.h>
#include <Performer/pfutil.h>

#include "custom.h"

static pfFog	 *eskyFog;
static pfVec3	fogColor;

#define	TOTAL_HORIZ_ANGLE	7.0f

	/*-----------------------------------------------------*/

void
initEnvironment(void)
{
    pfEarthSky 	*es;

    /* Initialize sun */
    ViewState->sun = pfNewLSource();
    ViewState->sunDCS = pfNewDCS();

    /* Point in direction of view for easier LIGHTING_EYE computation */
    pfLSourcePos(ViewState->sun, 0.0f, -1.0f, 0.0f, 0.0f);

    pfAddChild(ViewState->scene, ViewState->sunDCS);
    pfAddChild(ViewState->sunDCS, ViewState->sun);

    es = pfNewESky();
    pfESkyMode(es, PFES_BUFFER_CLEAR, ViewState->earthSkyMode); 
    pfESkyAttr(es, PFES_GRND_HT, -0.5f * ViewState->sceneSize);
    pfESkyColor(es, PFES_CLEAR, 
		    ViewState->earthSkyColor[0],
		    ViewState->earthSkyColor[1],
		    ViewState->earthSkyColor[2],
		    ViewState->earthSkyColor[3]);
    pfESkyAttr(es, PFES_HORIZ_ANGLE, TOTAL_HORIZ_ANGLE);

    /* setup the fog for general visibility */
    eskyFog = pfNewFog(pfGetSharedArena());
    pfSetVec3(fogColor, .6f, .6f, .6f);

    if(ViewState->fog != PFFOG_OFF)
    {
	pfFogType(eskyFog, ViewState->fog);
	pfFogColor(eskyFog, 
		   fogColor[0] * ViewState->timeOfDay, 
		   fogColor[1] * ViewState->timeOfDay, 
		   fogColor[2] * ViewState->timeOfDay); 
	pfFogRange(eskyFog, ViewState->nearFogRange, ViewState->farFogRange);
    }
    ViewState->eSky = es;
}

void
updateEnvironment(void)
{
    pfEarthSky 		*es = ViewState->eSky;
    static int 	 	earthSkyMode = -1;
    static int 	 	fog = -1;
    static float 	nearFogRange = 0.0f, farFogRange = 0.0f;
    
    if ((earthSkyMode == ViewState->earthSkyMode) && 
	(fog == ViewState->fog) && 
	(nearFogRange == ViewState->nearFogRange) &&
	(farFogRange == ViewState->farFogRange))
	return;
 
    pfESkyMode(es, PFES_BUFFER_CLEAR, ViewState->earthSkyMode); 

    if (ViewState->fog != PFFOG_OFF)
    {
	pfFogType(eskyFog, ViewState->fog);
	pfFogRange(eskyFog, ViewState->nearFogRange, ViewState->farFogRange);
	pfESkyFog(es, PFES_GENERAL, eskyFog);
	pfFogColor(eskyFog, 
		   fogColor[0] * ViewState->timeOfDay, 
		   fogColor[1] * ViewState->timeOfDay, 
		   fogColor[2] * ViewState->timeOfDay); 
	pfESkyColor(es, PFES_CLEAR, 
		    fogColor[0] * ViewState->timeOfDay, 
		    fogColor[1] * ViewState->timeOfDay, 
		    fogColor[2] * ViewState->timeOfDay,
		    ViewState->earthSkyColor[3]);
    }
    else
    {
	pfESkyFog(es, PFES_GENERAL, NULL);
	pfESkyColor(es, PFES_CLEAR, 
		    ViewState->earthSkyColor[0],
		    ViewState->earthSkyColor[1],
		    ViewState->earthSkyColor[2],
		    ViewState->earthSkyColor[3]);
    }

    /* update static values */
    earthSkyMode = ViewState->earthSkyMode;
    fog = ViewState->fog;
    nearFogRange = ViewState->nearFogRange;
    farFogRange = ViewState->farFogRange;
}


void
updateTimeOfDay(float newTod)
{
    float 		r, g, b;
    float 		s;
    pfVec3 		t;
    pfEarthSky		*es;
    static int		lighting = -1;
    static float  	tod = -1.0f;
    static pfVec3	noon = {-.05f, -.1f, 1.0f};
    static pfVec3	night = {-.15f, 1.0f, .2f};
    static pfVec3	prevViewDir;

    /* only do if necessary */
    if (ViewState->lighting == lighting && newTod == tod && 
	(ViewState->lighting == LIGHTING_SUN || 
	 PFEQUAL_VEC3(ViewState->viewMat[1], prevViewDir)))
        return;

    tod = newTod;
    lighting = ViewState->lighting;
    pfCopyVec3(prevViewDir, ViewState->viewMat[1]); 

    es = ViewState->eSky;

    /* make horizon band angle directly proportional to time of day */
    pfESkyAttr(es, PFES_HORIZ_ANGLE, 1.0f + (TOTAL_HORIZ_ANGLE - 1.0f)*tod);

    /* scale it for the following ramp calcs */
    s = tod * 4.0f;	
    
    /* top of sky */
    r = (0.4f * s) - 1.6f;   r = PF_MAX2(0.0f, r);  r = PF_MIN2(0.7f, r);
    g = (0.4f * s) - 1.6f;   g = PF_MAX2(0.0f, g);  g = PF_MIN2(0.7f, g); 
    b = (0.8f * s) - 1.6f;   b = PF_MAX2(0.0f, b);  b = PF_MIN2(1.0f, b);
    pfESkyColor(es, PFES_SKY_TOP, r,g,b, 1.0f);
    
    /* bottom of sky */
    r = (0.6f * s) - 1.6f;   r = PF_MAX2(0.0f, r);  r = PF_MIN2(0.7f, r);
    g = (0.6f * s) - 1.6f;   g = PF_MAX2(0.0f, g);  g = PF_MIN2(0.7f, g); 
    b = (1.0f * s) - 1.6f;   b = PF_MAX2(0.0f, b);  b = PF_MIN2(1.0f, b);
    pfESkyColor(es, PFES_SKY_BOT, r,g,b, 1.0f);
	
    /* horizon */
    r = (1.45f * s) - 0.4f;   r = PF_MAX2(0.0f, r);  r = PF_MIN2(0.7f, r);
    g = (0.65f * s) - 1.4f;   g = PF_MAX2(0.0f, g);  g = PF_MIN2(0.7f, g); 
    b = (0.65f * s) - 1.6f;   b = PF_MAX2(0.0f, b);  b = PF_MIN2(1.0f, b);
    pfESkyColor(es, PFES_HORIZ, r,g,b, 1.0f);
    
    /* ground */
    r = (0.0181f * (s - 0.8f));   r = PF_MAX2(0.0f, r);  r = PF_MIN2(0.058f, r);
    g = (0.0840f * (s - 0.8f));   g = PF_MAX2(0.0f, g);  g = PF_MIN2(0.270f, g); 
    b = (0.0181f * (s - 0.8f));   b = PF_MAX2(0.0f, b);  b = PF_MIN2(0.058f, b); 
    pfESkyColor(es, PFES_GRND_NEAR, r ,g, b, 1.0f);
    pfESkyColor(es, PFES_GRND_FAR,  r ,g, b, 1.0f);

    /* Scale fog color based on time of day */
    if (ViewState->fog != PFFOG_OFF)
    {
	pfFogColor(eskyFog, 
		   fogColor[0] * ViewState->timeOfDay, 
		   fogColor[1] * ViewState->timeOfDay, 
		   fogColor[2] * ViewState->timeOfDay); 
	pfESkyColor(es, PFES_CLEAR, 
		    fogColor[0] * ViewState->timeOfDay, 
		    fogColor[1] * ViewState->timeOfDay, 
		    fogColor[2] * ViewState->timeOfDay,
		    ViewState->earthSkyColor[3]);
    }
    
    /* Here we set the light position. */
    pfCombineVec3(t, tod, noon, 1.0f - tod, night);
    pfNormalizeVec3(t);
    if (ViewState->lighting == LIGHTING_SUN)
    {
	pfMatrix	mat;
	static pfVec3	nyaxis = {0.0f, -1.0f, 0.0f};

	pfMakeVecRotVecMat(mat, nyaxis, t);
    	pfDCSMat(ViewState->sunDCS, mat);
	pfLSourceColor(ViewState->sun, PFLT_AMBIENT,
		       0.8f*tod, 0.8f*tod, 0.8f*tod); 
    }
    else /* LIGHTING_EYE */	
    {
	pfLSourceColor(ViewState->sun, PFLT_AMBIENT, 0.0f, 0.0f, 0.0f);
    	pfDCSMat(ViewState->sunDCS, ViewState->viewMat);
    }

    pfLSourceColor(ViewState->sun, PFLT_DIFFUSE,  
		   0.8f*tod, 0.8f*tod, 0.8f*tod); 
    pfLSourceColor(ViewState->sun, PFLT_SPECULAR, 
		   0.8f*tod, 0.8f*tod, 0.8f*tod); 
}
