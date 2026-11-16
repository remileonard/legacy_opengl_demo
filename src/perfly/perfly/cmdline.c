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
 * cmdline.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include <Performer/pf.h>
#include <Performer/pfutil.h>
#include <Performer/pfui.h>
#include <Performer/pfdu.h>

#include "perfly.h"

/* **************************************************************************
 *		Process the command line options
 ****************************************************************************/

static void
usage(char *progName)
{
    char	*baseName;
    static char *helpList =
	"usage:\n"
	"	%s [options] model.ext ...\n"
	"\n"
	"options:\n"
	"    -A <string>                   -welcome text\n"
	"    -a <string>                   -overlay text\n"
	"    -b r,g,b[,a]                  -Earth/Sky clear color\n"
	"    -c <numChannels>              -Number of channels\n"
	"    -C a,b,c,...                  -Pipes for channels (L-to-R)\n"
	"    -d                            -Drive\n"
	"    -D                            -Lock down draw process\n"
	"    -e <h,p,r>                    -Initial view angles\n"
	"    -E <string>                   -Earth sky mode\n"
	"        tag      -> tag clear\n"
	"        clear    -> regular clear\n"
	"        sky      -> sky only\n"
	"        skygrnd  -> sky and ground\n"
	"        skyclear -> sky and clear\n"
	"    -f                            -Fly\n"
	"    -F <path>                     -Set file path\n"
	"    -g <format>                   -GUI format\n"
	"        0 -> GUI OFF\n"
	"        1 -> GUI_VERTICAL\n"
	"        2 -> GUI_HORIZONTAL\n"
	"    -G                            -Use GANGDRAW with Multipipe\n"
	"    -h                            -Print command list\n"
	"    -H <horiz_fov>,<vert_fov>,<offset> -Set Custom Multipipe Field of View\n"
	"    -i <count>                    -Set file loading repeat count\n"
	"    -I <count>                    -Exit after printing stats for count frames\n"
	"    -j <font_type>                -Set font type\n"
	"        0 -> PFDOBJFNT_TEXTURED\n"
	"        1 -> PFDOBJFNT_OUTLINED\n"
	"        2 -> PFDOBJFNT_FILLED\n"
	"        3 -> PFDOBJFNT_EXTRUDED\n"
	"    -J <font_name>                -Set Font to Named Font\n"
	"    -k r,g,b[,a]                  -Scribed-style draw color\n"
	"    -K <count>                    -Combine billboards up to size count\n"
	"    -l <style>                    -Set lighting enable\n"
	"        0 -> lighting off\n"
	"        1 -> lighting eye\n"
	"        2 -> lighting sky\n"
	"    -l <x,y,z,r,g,b>              -Add a light source to the scene\n"
	"    -L                            -Free then Lock down processes (requires root ID)\n"
	"    -m <mode>                     -Multiprocessing mode\n"
	"       -1 -> PFMP_DEFAULT\n"
	"        0 -> PFMP_APPCULLDRAW\n"
	"        2 -> PFMP_APP_CULLDRAW\n"
	"        4 -> PFMP_APPCULL_DRAW\n"
	"        6 -> PFMP_APP_CULL_DRAW\n"
	"    -M <mode>                     -Multipipe mode\n"
	"        0 -> single pipe mode\n"
	"        1 -> multipipe mode\n"
	"        2 -> hyperpipe mode\n"
	"    -n <notify>                   -Debug level\n";
    static char *helpList2 =
	"    -N                            -Assign non-degrading priorities to Performer processes\n"
	"    -o <mode>,<value>             -Set builder mode (default value is OFF)\n"
	"    -O <mode>,<value>             -Set builder mode (default value is ON)\n"
	"        0 -> PFDBLDR_MESH_ENABLE\n"
	"        1 -> PFDBLDR_MESH_SHOW_TSTRIPS\n"
	"        2 -> PFDBLDR_MESH_INDEXED\n"
	"        3 -> PFDBLDR_MESH_MAX_TRIS\n"
	"        4 -> PFDBLDR_MESH_RETESSELLATE\n"
	"         5 -> PFDBLDR_MESH_LOCAL_LIGHTING\n"
	"        10 -> PFDBLDR_AUTO_COLORS\n"
	"             0 -> PFDBLDR_COLORS_PRESERVE  + leave colors alone\n"
	"             1 -> PFDBLDR_COLORS_MISSING   - make missing colors\n"
	"             2 -> PFDBLDR_COLORS_GENERATE  - make all colors\n"
	"             3 -> PFDBLDR_COLORS_DISCARD   - toss existing colors\n"
	"       11 -> PFDBLDR_AUTO_NORMALS\n"
	"             0 -> PFDBLDR_NORMALS_PRESERVE - leave normals alone\n"
	"             1 -> PFDBLDR_NORMALS_MISSING  + make missing normals\n"
	"             2 -> PFDBLDR_NORMALS_GENERATE - make all normals\n"
	"             3 -> PFDBLDR_NORMALS_DISCARD  - toss existing normals\n"
	"       12 -> PFDBLDR_AUTO_TEXTURE\n"
	"             0 -> PFDBLDR_TEXTURE_PRESERVE + leave texture coord alone\n"
	"             1 -> PFDBLDR_TEXTURE_MISSING  - make missing texture coord\n"
	"             2 -> PFDBLDR_TEXTURE_GENERATE - make all texture coord\n"
	"             3 -> PFDBLDR_TEXTURE_DISCARD  - toss existing texture coord\n"
	"       13 -> PFDBLDR_AUTO_ORIENT\n"
	"             0 -> PFDBLDR_ORIENT_PRESERVE  - leave normal and order alone\n"
	"             1 -> PFDBLDR_ORIENT_NORMALS   - make normal match vertex order\n"
	"             2 -> PFDBLDR_ORIENT_VERTICES  + make vertex order match normal\n"
	"        15 -> PFDBLDR_AUTO_DISABLE_TCOORDS_BY_STATE\n"
	"              0 - dont remove tcoords if no texture specified\n"
	"              1 - remove tcoords if no texture specified\n"
	"        16 -> PFDBLDR_AUTO_DISABLE_NCOORDS_BY_STATE\n"
	"              0 - dont remove ncoords if no material specified\n"
	"              1 - remove ncoords if no material specified\n"
	"        17 -> PFDBLDR_AUTO_LIGHTING_STATE_BY_NCOORDS\n"
	"              0 - dont automatically set lighting enable based on presence of ncoords\n"
	"              1 - automatically set lighting enable based on presence of ncoords\n"
	"        18 -> PFDBLDR_AUTO_LIGHTING_STATE_BY_MATERIALS\n"
	"              0 - dont automatically set lighting enable based on presence of material\n"
	"              1 - automatically set lighting enable based on presence of material\n"
	"        19 -> PFDBLDR_AUTO_TEXTURE_STATE_BY_TEXTURES\n"
	"              0 - dont automatically set texture enable based on presence of tcoords\n"
	"              1 - automatically set texture enable based on presence of tcoords\n"
	"        20 -> PFDBLDR_AUTO_TEXTURE_STATE_BY_TCOORDS\n"
	"              0 - dont automatically set texture enable based on presence of texture\n"
	"              1 - automatically set texture enable based on presence of texture\n"
	"       30 -> PFDBLDR_BREAKUP\n"
	"       31 -> PFDBLDR_BREAKUP_SIZE\n"
	"       32 -> PFDBLDR_BREAKUP_BRANCH\n"
	"       33 -> PFDBLDR_BREAKUP_STRIP_LENGTH\n"
	"       34 -> PFDBLDR_SHARE_MASK\n"
	"       35 -> PFDBLDR_ATTACH_NODE_NAMES\n"
	"       36 -> PFDBLDR_DESTROY_DATA_UPON_BUILD\n"
	"       37 -> PFDBLDR_PF12_STATE_COMPATIBLE\n"
	"       38 -> PFDBLDR_BUILD_LIMIT  -maximum number of tris/strip\n"
	"    -p <x,y,z>                    -Initial view position\n"
	"    -P <phase>                    -Phase\n"
	"        0 -> PFPHASE_FLOAT\n"
	"        1 -> PFPHASE_LOCK\n"
	"        2 -> PFPHASE_FREE_RUN\n"
	"        3 -> PFPHASE_LIMIT\n"
	"    -q <optimize>                 -Optimize Scene GStates into single Scene pfGeoState\n"
	"    -Q <optimize>                 -Flatten tree and remove empty nodes\n"
	"        0 -> Off, 1 -> On\n"
	"    -r <frameRate>                -Target frame rate in Hertz\n"
	"    -R <mode>                     -Retessellate input geometry\n"
	"        0 -> Off, 1 -> On\n"
	"    -s <LODscale>                 -Set LOD scale factor\n"
	"    -S <mode>                     -Show triangle strips via color coding\n"
	"        0 -> Off, 1 -> On\n"
        "    -t <0xVisual_id>              -specify GLX Visual ID has Hex Integer(OpenGL Only)\n" 
	"    -T <mode>                     -use gl display lists\n"
	"        0 -> Off, 1 -> On\n"
	"    -u                            -Inflate input files\n"
	"    -v <type>                     -Use vertex arrays of specified type\n"
	"	 0 -> Off \n"
	"	 1 -> PFGS_PA_C4UBN3ST2FV3F - all attrs packed in arrays\n"
	"	 2 -> PFGS_PA_C4UBN3ST2F - all attrs but verts packed in arrays\n"
	"    -V <mode>			   -Set Dynamic Resolution (DVR) mode\n"
	"    	<mode,xsize,ysize>	   - Set DVR mode and initial size.\n"
	"        0 -> PFPVC_DVR_OFF \n"
	"        1 -> PFPVC_DVR_MANUAL \n"
	"        2 -> PFPVC_DVR_AUTOMATIC \n"
	"    -w <file>.out                 -Write scene in ASCII\n"
	"    -w <file.ext>                 -Write scene in .ext format\n"
	"    -W <Size>                     -Window size\n"
	"    -W <xSize,ySize>              -Window size\n"
	"    -x <fork>                     -Set use of GLX input handling, forked != 0-> asynch\n"
	"    -X <radius>                   -Explode input files\n"
	"    -y ext,mode,value             -Set loader mode for file extension ext to value\n"
	"    -Y ext,alias                  -Substitute file extension alias for file extension ext\n"
	"    -z near,far                   -Set near/far clip ranges\n"
	"    -Z                            -Don't initially free CPUs before lock\n"
        "\n";
    
    /* just use program's base name: /usr/fred/base -> base */
    if (progName == NULL)
        baseName = "UNKNOWN";
    else if ((baseName = strchr(progName, '/')) == NULL)
	    baseName = progName;
    
    /* print list of command-line options and arguments */
    fprintf(stderr, helpList, baseName);
    fprintf(stderr, helpList2);
    fflush(stderr);
    exit(1);
}

char optionStr[] = 
    "A:a:b:B:c:C:dDe:E:fF:g:GhH:i:I:j:J:k:K:l:Lm:M:"
    "o:O:Nn:p:P:q:Q:r:R:s:S:t:T:uv:V:W:w:x:X:y:Y:z:Z?";

int
processCmdLine(int argc, char *argv[], char ***files)
{
    extern char *optarg;
    extern int        optind;
    int                opt;
    int                guiFormatSet = 0;
    
    /* process command-line arguments */
    while ((opt = getopt(argc, argv, optionStr)) != -1)
    {
        switch (opt)
        {
	    /* Specify introductory 3D welcome text */
	case 'A':	
	    strcpy(ViewState->welcomeText, optarg);
	    break;
	    
	    /* Specify overlay-plane attribution text */
	case 'a':	
	    strcpy(ViewState->overlayText, optarg);
	    break;
	    
	    /* Specify mapping of processes to processors for locked CPUs */
	case 'B': 
	    {
		int i1, i2, i3;
		if (sscanf(optarg, "%d,%d,%d", &i1, &i2, &i3) == 3)
		{
		    AppCPU = i1;
		    CullCPU = i2;
		    DrawCPU = i3;
		}
		else
		    usage(argv[0]);
	    }
	    break;
	    
	    /* Specify the Earth/Sky clear color */
	case 'b':
	    if (sscanf(optarg, "%f,%f,%f,%f",
		       &ViewState->earthSkyColor[0],
		       &ViewState->earthSkyColor[1],
		       &ViewState->earthSkyColor[2],
		       &ViewState->earthSkyColor[3]) != 4 &&
		sscanf(optarg, "%f,%f,%f",
		       &ViewState->earthSkyColor[0],
		       &ViewState->earthSkyColor[1],
		       &ViewState->earthSkyColor[2]) != 3)
		usage(argv[0]);
	    break;
	    
	    /* Specify number of channels to use */
	case 'c':	
	    if (atoi(optarg) < 1)
		usage(argv[0]);
	    NumChans = atoi(optarg);
	    if (!guiFormatSet && (NumChans > 1))
		ViewState->guiFormat = GUI_HORIZONTAL;
	    break;
	    
	    /* Specify the pipe to use for each channel */
	case 'C':	
	    {
		int	 	index = 0;
		int	 	count = 0;
		char	*start = optarg;
		
		while (sscanf(start, "%d%n", &ChanOrder[index++], &count) == 1)
		    if (*(start += count) != '\0')
			++start;
	    }
	    break;
	    
	    /* Set pfuXformer default to Drive */
	case 'd':
	    ViewState->xformerModel = PFITDF_DRIVE;
	    break;
	    
	    /* lock down processor for just draw process */
	case 'D':
	    ViewState->procLock |= PFMP_FORK_DRAW;
	    break;
	    
	    /* Set the initial view Euler angles (h p r) */
	case 'e':
	    if (sscanf(optarg, "%f,%f,%f",
		       &ViewState->initView.hpr[PF_H],
		       &ViewState->initView.hpr[PF_P],
		       &ViewState->initView.hpr[PF_R]) == 3)
		InitHPR = TRUE;
	    else
		usage(argv[0]);
	    break;
	    
	    /* Set Earth Sky mode */
	case 'E':
	    if (strcmp(optarg, "tag") == 0)
		if (ViewState->haveTagClear)
		    ViewState->earthSkyMode = PFES_TAG;
		else
		    ViewState->earthSkyMode = PFES_FAST;
	    else if (strcmp(optarg, "sky") == 0)
		ViewState->earthSkyMode = PFES_SKY;
	    else if (strcmp(optarg, "skygrnd") == 0)
		ViewState->earthSkyMode = PFES_SKY_GRND;
	    else if (strcmp(optarg, "skyclear") == 0)
		ViewState->earthSkyMode = PFES_SKY_CLEAR;
	    else if (strcmp(optarg, "clear") == 0)
		ViewState->earthSkyMode = PFES_FAST;
	    else 
		usage(argv[0]);
	    break;
	    
	    /* Set pfuXformer default to Fly */
	case 'f':
	    ViewState->xformerModel = PFITDF_FLY;
	    break;
	    
	    /* Specify the file search path -- look here first */
	case 'F':
	    {
	    int oldLength = 0;
	    int newLength = 0;
	    int fullLength = 0;
	    const char *oldPath = pfGetFilePath();
	    char *newPath = optarg;
	    char *fullPath = NULL;

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
	    break;
	    
	    /* Specify the GUI format */
	case 'g':
	    ViewState->gui = ((ViewState->guiFormat = atoi(optarg)) ? 1 : 0);
	    if( ViewState->guiFormat != GUI_HORIZONTAL )
		ViewState->guiFormat = GUI_VERTICAL;
	    guiFormatSet = 1;
	    break;
	    
	    /* Initialize with GUI Panel disabled */
	case 'G':
	    GangDraw ^= 1;
	    break;
	    
	    /* set repeat count for file loading */
	case 'H':
	    if(sscanf(optarg, "%f,%f,%f", 
		      &ViewState->fov[0],
		      &ViewState->fov[1],
		      &ViewState->fov[2]) == 3)
		InitFOV = TRUE;
	    else
		usage(argv[0]);
	    break;
	case 'i':
	    ViewState->iterate = atoi(optarg);
	    break;
	    
	    /* set repeat count for file loading */
	case 'I':
	    ViewState->printStats = atoi(optarg);
	    ViewState->exitCount = atoi(optarg);
	    break;
	    
	case 'j':
	    ViewState->objFontType = atoi(optarg);
	    break;

	case 'J':
	    ViewState->objFontName = strdup(optarg);
	    break;
	    
	    /* Specify the scribed-style draw color */
	case 'k':
	    if (sscanf(optarg, "%f,%f,%f,%f",
		       &ViewState->scribeColor[0],
		       &ViewState->scribeColor[1],
		       &ViewState->scribeColor[2],
		       &ViewState->scribeColor[3]) != 4 &&
		sscanf(optarg, "%f,%f,%f",
		       &ViewState->scribeColor[0],
		       &ViewState->scribeColor[1],
		       &ViewState->scribeColor[2]) != 3)
		usage(argv[0]);
	    break;
	    
	case 'K':
	    ViewState->combineBillboards = atoi(optarg);
	    break;

	    /* inflate files */
	case 'u':
	    ViewState->explode = -1;
	    break;
	    
	    /* remember light source definition */
	case 'l':
	    {
		float lxyz[3], lrgb[3], l;
		if (sscanf(optarg, "%f,%f,%f,%f,%f,%f", 
			   &lxyz[0], &lxyz[1], &lxyz[2], &lrgb[0], &lrgb[1], &lrgb[2]) == 6)
		{
		    if (ViewState->lamps < (MAX_LAMPS - 1))
		    {
			/* set default default values */
			pfSetVec3(ViewState->lampXYZ[ViewState->lamps], 0.0f,0.0f,0.0f);
			pfSetVec3(ViewState->lampRGB[ViewState->lamps], 1.0f,1.0f,1.0f);
			
			/* read specific values */
			sscanf(optarg, "%f,%f,%f,%f,%f,%f",
			       &ViewState->lampXYZ[ViewState->lamps][0],
			       &ViewState->lampXYZ[ViewState->lamps][1],
			       &ViewState->lampXYZ[ViewState->lamps][2],
			       &ViewState->lampRGB[ViewState->lamps][0],
			       &ViewState->lampRGB[ViewState->lamps][1], 
			       &ViewState->lampRGB[ViewState->lamps][2]);
			
			/* advance lamp-definition count */
			ViewState->lamps += 1;
		    }
		}
		else if (sscanf(optarg, "%f", &l) == 1)
		    ViewState->lighting = l;
		else
		    usage(argv[0]);
	    }
	    break;
	    
	    /* lock down processors for app, cull, and draw */
	case 'L':
	    ViewState->procLock = ((uint)~0);
	    break;
	    
	    /* Specify multiprocessing mode */
	case 'm':
	    ProcSplit = atoi(optarg);
	    break;
	    
	    /* Enable multipipe mode */
	case 'M':
	    Multipipe = atoi(optarg);
	    break;
	    
	    /* Set the notification (debug) level */
	case 'n':
	    NotifyLevel = atoi(optarg);
	    break;
	    
	    /* Assign non-degrading priorities to Performer processes. */
	case 'N':
	    PrioritizeProcs = 1;
	    break;
	    
        case 'o':
            {
                int mode;
                int value;
		int found;
		
                found = sscanf(optarg, "%d,%d", &mode, &value);
                if (found == 2)
		    pfdBldrMode(mode, value);
		else if (found == 1)
		    pfdBldrMode(mode, 0);
		else
		    usage(argv[0]);
            }
            break;
	    
	case 'O':
            {
                int mode;
                int value;
		int found;
		
                found = sscanf(optarg, "%d,%d", &mode, &value);
                if (found == 2)
		    pfdBldrMode(mode, value);
		else if (found == 1)
		    pfdBldrMode(mode, 1);
		else
		    usage(argv[0]);
            }
	    break;
	    
	    /* Set the initial view position (x y z)*/
	case 'p':
	    if (sscanf(optarg, "%f,%f,%f",
		       &ViewState->initView.xyz[PF_X],
		       &ViewState->initView.xyz[PF_Y],
		       &ViewState->initView.xyz[PF_Z]) == 3)
		InitXYZ = TRUE;
	    else
		usage(argv[0]);
	    break;
	    
	    /* Set the frame phase */
	case 'P':
	    ViewState->phase = atoi(optarg);
	    break;
	    
	    /* Whether to optimize into a scene geostate */
	case 'q':
	    ViewState->optimizeGStates = atoi(optarg);
	    break;
	    
	    /* Scene cleaning */
	case 'Q':
	    ViewState->optimizeTree = atoi(optarg);
	    break;
	    
	    /* Set the frame rate */
	case 'r':
	    ViewState->frameRate = (float)atof(optarg);
	    if( ViewState->frameRate <= 0.0000001f)
		ViewState->frameRate = 30.0f;
	    break;
	    
	    /* Enable retessellation of input geometry */
	case 'R':
	    pfdBldrMode(PFDBLDR_MESH_RETESSELLATE, atoi(optarg));
	    break;
	    
	    /* set LOD scale factor */
	case 's':
	    ViewState->LODscale = atof(optarg);
	    break;
	    
	    /* Enable color-coding of individual triangle strips */
	case 'S':
	    pfdBldrMode(PFDBLDR_MESH_SHOW_TSTRIPS, atoi(optarg));
	    break;
	case 't':
	    sscanf(optarg,"0x%x",&ViewState->visualID);
	    break;
	    /* set pfGeoSet packed attribute format for vertex arrays */

	/* set display-list mode */
	case 'T':
	    if (atoi(optarg))
		ViewState->drawMode = DLIST_GEOSET;
	    else
		ViewState->drawMode = DLIST_OFF;
	    break;

	case 'v':
	    PackedAttrFormat = atoi(optarg);
	    break;
	    /* change DVR mode */
	case 'V':
	    if (sscanf(optarg, "%d,%d,%d", &ViewState->doDVR, &ViewState->dvrXSize, &ViewState->dvrYSize) != 3)
		ViewState->doDVR = atoi(optarg);
	    break;
	    /* Write scene to a file */
	case 'w':
	    if (optarg[0] == '-')
		usage(argv[0]);
	    WriteFile = strdup(optarg);
	    break;
	    
	    /* Set the window size */
	case 'W':
	    if (sscanf(optarg, "%d,%d", &WinSizeX, &WinSizeY) != 2)
		if (sscanf(optarg, "%d", &WinSizeX) == 1)
		    WinSizeY = WinSizeX;
		else
		    usage(argv[0]);
	    break;
	    
	case 'x':
	    if (atoi(optarg))
		ViewState->input = PFUINPUT_X;
	    else
		ViewState->input = PFUINPUT_NOFORK_X;
	    break;
	    
	    /* explode files */
	case 'X':
	    ViewState->explode = (float)atof(optarg);
	    break;
	    /* Set specific loader mode */
        case 'y':
            {
                int mode;
                float val;
                char buf[64], *c;
		if ((c=strchr(optarg, ',')) != NULL) *c = ' ';
	    	if (sscanf(optarg,"%s %d,%f",buf,&mode,&val) == 3)
		    pfdConverterMode(buf,mode,val);
		else 
		    usage(argv[0]);
	    }
	    break;
	    /* Add an file extension alias */
	case 'Y':
	    {
		char buf[64],buf2[64], *c;
		
		if ((c=strchr(optarg, ',')) != NULL) *c = ' ';
	    	if (sscanf(optarg,"%s %s",buf,buf2) == 2) 
		    pfdAddExtAlias(buf,buf2);
		else
		    usage(argv[0]);
	    }
	    break;

	case 'z':
	    {
		float near, far;
		if (sscanf(optarg, "%f,%f", &near, &far) == 2)
		{
		    ViewState->near = near;
		    ViewState->far = far;
		    ViewState->nearFogRange = ViewState->near;
		    ViewState->farFogRange = ViewState->far;
		}
		else
		    usage(argv[0]);
	    }
	    
	    break;
	    
	case 'Z':
	    FreeInitCPUs = 0;
	    break;
	    
	    /* Unknown option is no option at all */
	case '?':
	case 'h':
	default:
	    usage(argv[0]);
	}
    }
    
    /* return address of database filename list through "files" argument */
    *files = &(argv[optind]);
    
    /* function return value is number of database files to be read */
    return argc - optind;
}
