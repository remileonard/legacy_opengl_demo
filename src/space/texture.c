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

extern t_stopwatch Counter ;

/**********************************************************************
*  spTevDef()  -
**********************************************************************/
void spTevDef(void)
{
#ifdef SP_IRIS_GL
   flot32 tevps[4];

   tevps[0] = TV_NULL;
   tevdef(1,0,tevps);
#endif

#ifdef SP_OPEN_GL
   glTexEnvf(GL_TEXTURE_ENV_MODE,GL_TEXTURE_ENV_MODE,GL_MODULATE);
#endif
}

/**********************************************************************
*  spTexDef()  -
**********************************************************************/
uint32 spTexDef(uint32 comp,uint32 dx,uint32 dy,void *arr,uint32 flag)
{
   static uint32 texid = 1;

#ifdef SP_IRIS_GL
   flot32 texps[8];

   texps[0] = TX_MAGFILTER ;
   texps[1] = TX_BILINEAR ;
   texps[2] = TX_MINFILTER ;
   texps[3] = (flag) ? TX_MIPMAP_TRILINEAR : TX_BILINEAR ;
   texps[4] = TX_WRAP ;
   texps[5] = TX_REPEAT ;
   texps[6] = TX_NULL ;

   texdef2d(texid,comp,dx,dy,arr,0,texps);
   return(texid++);
#endif

#ifdef SP_OPEN_GL
   uint32 format,i,*lptr,r,g,b,a;
   uint16 *sptr;

   texid = glGenLists(1);
   glNewList(texid,GL_COMPILE) ;

   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
   glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

   switch (comp) {
      case 1 : format = GL_LUMINANCE;
               break;

      case 2 : format = GL_LUMINANCE_ALPHA;
               for (sptr=arr,i=dx*dy; i>0; sptr++,i--) {
                 r = (*sptr     ) & 0xff;
                 a = (*sptr >> 8) & 0xff;
                 *sptr = (r<<8) | a; 
                 }
               break;

      case 4 : format = GL_RGBA;
               for (lptr=arr,i=dx*dy; i>0; lptr++,i--) {
                 r = (*lptr      ) & 0xff;
                 g = (*lptr >>  8) & 0xff;
                 b = (*lptr >> 16) & 0xff;
                 a = (*lptr >> 24) & 0xff;
                 *lptr = (r<<24) | (g<<16) | (b<<8) | a; 
                 }
               break;
      }
   glTexImage2D(GL_TEXTURE_2D,0,comp,dx,dy,0,format,GL_UNSIGNED_BYTE,arr);

   glEndList() ;
   return(texid);
#endif
}

/**********************************************************************
*  spFlipTex()  -
**********************************************************************/
void spFlipTex(uint32 flag,uint32 obj)

{
   glEnable(GL_TEXTURE_2D);
   glCallList(obj) ;
   if (!(Counter.flags & TEXTR_FLAG))
     return;

#ifdef SP_IRIS_GL
   if (flag) {
     texbind(0,obj);
     tevbind(0,1);
     }
   else {
     texbind(0,0);
     tevbind(0,0);
     }
#endif

#ifdef SP_OPEN_GL
   if (flag) {
     glEnable(GL_TEXTURE_2D);
     glCallList(obj) ;
     }
   else glDisable(GL_TEXTURE_2D);
#endif
}

