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

typedef struct  { flot32 x,y,z,s,t ; } P5 ;

typedef struct  {
   flot32  a,b,c,d,J,K,L ;
   P5      *u,*v,*w ;
} t_octa ;

extern t_stopwatch Counter ;
static sint32 A,B,loop ;
static t_octa octa[8],*o ;
static P5     dt[4][65][65],*d ;
static sint32 already_count = 0 ;
static sint32 already[64][2] ;

/**********************************************************************
*  world_mid()  -
**********************************************************************/
static void world_mid(sint32 x1,sint32 y1,sint32 x2,sint32 y2)

{  register sint32 xi,yi ;
   register flot32 a,b,x,y,z,w ;

   xi = (x1 + x2) >> 1 ;
   yi = (y1 + y2) >> 1 ;
   d = &dt[loop][yi][xi] ;

   x = dt[loop][y1][x1].x + dt[loop][y2][x2].x ;
   y = dt[loop][y1][x1].y + dt[loop][y2][x2].y ;
   z = dt[loop][y1][x1].z + dt[loop][y2][x2].z ;
   w = sqrt(x*x+y*y+z*z) ;
   d->x = x/w ; d->y = y/w ; d->z = z/w ;

   o = &octa[(yi < B-xi) ? loop+4 : loop] ;

   w = -o->d / (o->a*d->x + o->b*d->y + o->c*d->z) ;
   a = w * d->x ;
   b = w * d->y ;

   x = ((b-o->w->y)*(o->v->x-o->w->x) - (a-o->w->x)*(o->v->y-o->w->y)) * o->J ;
   y = ((b-o->u->y)*(o->w->x-o->u->x) - (a-o->u->x)*(o->w->y-o->u->y)) * o->K ;
   z = ((b-o->v->y)*(o->u->x-o->v->x) - (a-o->v->x)*(o->u->y-o->v->y)) * o->L ;

   d->s = x*o->u->s + y*o->v->s + z*o->w->s ;
   d->t = x*o->u->t + y*o->v->t + z*o->w->t ;
}

/**********************************************************************
*  generate_sphere()  -
**********************************************************************/
sint32 generate_sphere(sint32 level,sint32 flag,sint32 tex_size) 

{  register flot32 x,y,z,w ;
   register sint32 i,j,stage,step,dstep ;

   if (level < 0) level = 0 ;
   if (level > 6) level = 6 ;

   j = (level << 16) | (flag << 12) | (tex_size) ;

   for (i=0; i<already_count; i++)
     if (j == already[i][0])
       return(already[i][1]) ;

   already[already_count][0] = j ;

   for (B=1,A=i=0; i<level; A=B,B<<=1,i++) ;

   w = 1.0/(float)tex_size ;

   d = &dt[0][0][0]; d->x =  0; d->y = -1; d->z =  0; d->s = 1.0-w; d->t =     w;
   d = &dt[0][0][B]; d->x =  0; d->y =  0; d->z = -1; d->s = 1.0-w; d->t = 0.5-w;
   d = &dt[0][B][B]; d->x =  0; d->y =  1; d->z =  0; d->s = 0.5+w; d->t = 0.5-w;
   d = &dt[0][B][0]; d->x =  1; d->y =  0; d->z =  0; d->s = 0.5+w; d->t =     w;
   d = &dt[1][0][0]; d->x =  0; d->y = -1; d->z =  0; d->s = 1.0-w; d->t = 1.0-w;
   d = &dt[1][0][B]; d->x = -1; d->y =  0; d->z =  0; d->s = 0.5+w; d->t = 1.0-w;
   d = &dt[1][B][B]; d->x =  0; d->y =  1; d->z =  0; d->s = 0.5+w; d->t = 0.5+w;
   d = &dt[1][B][0]; d->x =  0; d->y =  0; d->z = -1; d->s = 1.0-w; d->t = 0.5+w;
   d = &dt[2][0][0]; d->x =  0; d->y = -1; d->z =  0; d->s =     w; d->t = 1.0-w;
   d = &dt[2][0][B]; d->x =  0; d->y =  0; d->z =  1; d->s =     w; d->t = 0.5+w;
   d = &dt[2][B][B]; d->x =  0; d->y =  1; d->z =  0; d->s = 0.5-w; d->t = 0.5+w;
   d = &dt[2][B][0]; d->x = -1; d->y =  0; d->z =  0; d->s = 0.5-w; d->t = 1.0-w;
   d = &dt[3][0][0]; d->x =  0; d->y = -1; d->z =  0; d->s =     w; d->t =     w;
   d = &dt[3][0][B]; d->x =  1; d->y =  0; d->z =  0; d->s = 0.5-w; d->t =     w;
   d = &dt[3][B][B]; d->x =  0; d->y =  1; d->z =  0; d->s = 0.5-w; d->t = 0.5-w;
   d = &dt[3][B][0]; d->x =  0; d->y =  0; d->z =  1; d->s =     w; d->t = 0.5-w;

   octa[0].u = &dt[0][B][B] ; octa[0].v = &dt[0][0][B] ; octa[0].w = &dt[0][B][0] ;
   octa[4].u = &dt[0][0][0] ; octa[4].v = &dt[0][B][0] ; octa[4].w = &dt[0][0][B] ;
   octa[1].u = &dt[1][B][B] ; octa[1].v = &dt[1][0][B] ; octa[1].w = &dt[1][B][0] ;
   octa[5].u = &dt[1][0][0] ; octa[5].v = &dt[1][B][0] ; octa[5].w = &dt[1][0][B] ;
   octa[2].u = &dt[2][B][B] ; octa[2].v = &dt[2][0][B] ; octa[2].w = &dt[2][B][0] ;
   octa[6].u = &dt[2][0][0] ; octa[6].v = &dt[2][B][0] ; octa[6].w = &dt[2][0][B] ;
   octa[3].u = &dt[3][B][B] ; octa[3].v = &dt[3][0][B] ; octa[3].w = &dt[3][B][0] ;
   octa[7].u = &dt[3][0][0] ; octa[7].v = &dt[3][B][0] ; octa[7].w = &dt[3][0][B] ;

   for (o=octa,i=0; i<8; o++,i++)  {
     x = o->u->x + o->v->x + o->w->x ; 
     y = o->u->y + o->v->y + o->w->y ; 
     z = o->u->z + o->v->z + o->w->z ; 
     w = sqrt(x*x + y*y + z*z) ;

     o->a = x / w ;
     o->b = y / w ;
     o->c = z / w ;
     o->d = -w / 3.0 ;

     o->J = 1./((o->u->y-o->w->y)*(o->v->x-o->w->x)-(o->u->x-o->w->x)*(o->v->y-o->w->y));
     o->K = 1./((o->v->y-o->u->y)*(o->w->x-o->u->x)-(o->v->x-o->u->x)*(o->w->y-o->u->y));
     o->L = 1./((o->w->y-o->v->y)*(o->u->x-o->v->x)-(o->w->x-o->v->x)*(o->u->y-o->v->y));
     }

   for (loop=0; loop<4; loop++)
     for (step=A,dstep=B,stage=1; stage<=level; dstep=step,step>>=1,stage++)
       for (i=dstep; i<=B; i+=dstep)
          for (j=step; j<=B; j+=dstep)  {
             world_mid(j-step,i      ,j+step,i      ) ;   /* upper */
             world_mid(j-step,i-dstep,j+step,i-dstep) ;   /* lower */
             world_mid(j-step,i      ,j-step,i-dstep) ;   /* left */
             world_mid(j+step,i      ,j+step,i-dstep) ;   /* right */
             world_mid(j-step,i      ,j+step,i-dstep) ;   /* diag */
             }

   stage = glGenLists(1);
   glNewList(stage,GL_COMPILE) ;

   switch (flag)  {
     case FLAT_SPHERE :
               for (loop=2; loop<4; loop++)
                 for (j=0; j<B; j++)  {
                   glBegin(GL_TRIANGLE_STRIP) ;
                   for (i=0; i<=B; i++)  {
                     d = &dt[loop][i][j] ;
                     glVertex3fv(&d->x);
                     glVertex3fv(&(d+1)->x);
                     }
                   glEnd() ;
                   }
               break ;

     case LIGT_SPHERE :
               for (loop=((Counter.flags & NDPNT_FLAG)?0:2); loop<4; loop++)
                 for (j=0; j<B; j++)  {
                   glBegin(GL_TRIANGLE_STRIP) ;
                   for (i=0; i<=B; i++)  {
                     d = &dt[loop][i][j];
                     glNormal3fv(&d->x);
                     glVertex3fv(&d->x);
                     glNormal3fv(&(d+1)->x);
                     glVertex3fv(&(d+1)->x);
                     }
                   glEnd() ;
                   }
               break ;

     case TEXX_SPHERE :
               for (loop=0; loop<4; loop++)
                 for (j=0; j<B; j++)  {
                   glBegin(GL_TRIANGLE_STRIP) ;
                   for (i=0; i<=B; i++)  {
                     d = &dt[loop][i][j];
                     glTexCoord2fv(&d->s);
                     glNormal3fv(&d->x);
                     glVertex3fv(&d->x);
                     glTexCoord2fv(&(d+1)->s);
                     glNormal3fv(&(d+1)->x);
                     glVertex3fv(&(d+1)->x);
                     }
                   glEnd() ;
                   }
               break ;

     default : printf("ERROR: generate_sphere()\n") ;
               break ;
     }

   glEndList();

   already[already_count++][1] = stage ; 
   return(stage) ;
}

