/*
 * Copyright (C) 1992, 1993, 1994, Silicon Graphics, Inc.
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
#include "space.h"

#ifdef SP_IRIS_GL
static flot32 planet_material[] = {
    AMBIENT,   0.08, 0.08, 0.08,
    DIFFUSE,   1.00, 1.00, 1.00,
    SPECULAR,  0.00, 0.00, 0.00,
    SHININESS, 0.00,
    LMNULL
};

static flot32 infinite_viewer[] = {
    AMBIENT, 0.08,  0.08, 0.08,
    LOCALVIEWER, 0.0,
    LMNULL
};

static flot32 infinite_light[] = {
    AMBIENT,  0.08, 0.08, 0.08,
    LCOLOR,   1.00, 1.00, 1.00,
    POSITION, 0.00, 0.00, -1.00, 0.00,
    LMNULL
};
#endif

#ifdef SP_OPEN_GL
static flot32 mat_emi[] = { 0.00, 0.00, 0.00, 0.00 };
static flot32 mat_amb[] = { 0.08, 0.08, 0.08, 1.00 };
static flot32 mat_dif[] = { 1.00, 1.00, 1.00, 1.00 };
static flot32 mat_spe[] = { 0.00, 0.00, 0.00, 1.00 };
static flot32 mat_shi[] = { 0.00 };

static flot32 mod_amb[] = { 0.08, 0.08, 0.08, 1.00 };
static flot32 mod_loc[] = { 0.00 };
static flot32 mod_two[] = { 0.00 };

static flot32 lig_amb[] = { 0.08, 0.08, 0.08, 1.00 };
static flot32 lig_dif[] = { 1.00, 1.00, 1.00, 1.00 };
static flot32 lig_spe[] = { 0.00, 0.00, 0.00, 1.00 };
static flot32 lig_pos[] = { 0.00, 0.00,-1.00, 0.00 };
#endif

/********************************************************************** 
*  spInitLight()  -  
**********************************************************************/
void spInitLight(void)

{
#ifdef SP_IRIS_GL
   lmdef(DEFMATERIAL, 1, 0, planet_material);
   lmbind(MATERIAL,  0);

   lmdef(DEFLMODEL,   1, 0, infinite_viewer);
   lmbind(LMODEL,  1);
#endif

#ifdef SP_OPEN_GL
   glMaterialfv(GL_FRONT,GL_EMISSION,mat_emi);
   glMaterialfv(GL_FRONT,GL_AMBIENT,mat_amb);
   glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_dif);
   glMaterialfv(GL_FRONT,GL_SPECULAR,mat_spe);
   glMaterialfv(GL_FRONT,GL_SHININESS,mat_shi);

   glLightModelfv(GL_LIGHT_MODEL_AMBIENT,mod_amb);
   glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER,mod_loc);
   glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE,mod_two);

   glLightfv(GL_LIGHT0,GL_AMBIENT,lig_amb);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,lig_dif);
   glLightfv(GL_LIGHT0,GL_SPECULAR,lig_spe);
   glLightfv(GL_LIGHT0,GL_POSITION,lig_pos);

   glEnable(GL_LIGHT0);
#endif
}

/********************************************************************** 
*  spSetLight()  -  
**********************************************************************/
void spSetLight(V3 *vec)

{
#ifdef SP_IRIS_GL
   infinite_light[ 9] = vec->x;
   infinite_light[10] = vec->y;
   infinite_light[11] = vec->z;

   lmdef(DEFLIGHT,1,0,infinite_light);
   lmbind(LIGHT1,  1);
#endif

#ifdef SP_OPEN_GL
   flot32 arr[4];

   arr[0] = vec->x;
   arr[1] = vec->y;
   arr[2] = vec->z;
   arr[3] = 0.0;
   glLightfv(GL_LIGHT0,GL_POSITION,arr);
#endif
}

/********************************************************************** 
*  spLightMaterial()  -  
**********************************************************************/
void spLightMaterial(uint32 flag,uint32 col)

{
#ifdef SP_IRIS_GL
   if (flag) {
     lmbind(MATERIAL,1);
     lmcolor(LMC_DIFFUSE) ;
     cpack(col);
     }
   else {
     lmbind(MATERIAL,0);
     lmcolor(LMC_COLOR) ;
     cpack(col);
     }
#endif

#ifdef SP_OPEN_GL
   flot32 arr[4];

   if (flag) {
     glEnable(GL_LIGHTING);
     glEnable(GL_NORMALIZE) ;

     arr[0] = ((col >>  0) & 0xff)/255.0;
     arr[1] = ((col >>  8) & 0xff)/255.0;
     arr[2] = ((col >> 16) & 0xff)/255.0;
     arr[3] = ((col >> 24) & 0xff)/255.0;
     glMaterialfv(GL_FRONT,GL_DIFFUSE,arr) ;
     }
   else {
     glDisable(GL_LIGHTING);
     glDisable(GL_NORMALIZE) ;
     }
#endif
}

