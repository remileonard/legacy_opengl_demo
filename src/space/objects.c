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
#include <math.h>
#include <GL/gl.h>
#include "space.h"

extern t_stopwatch Counter ;

/**********************************************************************
*  object_space_station()  -
**********************************************************************/
sint32 object_space_station(sint32 resol)

{  register sint32 obj ;
   register flot32 siz = 1.0 ;

   obj = glGenLists(1);
   glNewList(obj,GL_COMPILE) ;

   glTranslatef(0.0,-60.0*siz,0.0) ;
   make_toroid(100.0*siz,10.0*siz,resol) ;

   glRotatef( 90.0,1.0,0.0,0.0) ;
   make_cylind(200.0*siz,5.0*siz,0,resol) ;
   glRotatef(-90.0,1.0,0.0,0.0) ;
   glRotatef( 90.0,0.0,0.0,1.0) ;
   make_cylind(200.0*siz,5.0*siz,0,resol) ;
   glRotatef(-90.0,0.0,0.0,1.0) ;

   glTranslatef(0.0, 60.0*siz,0.0) ;
   make_cylind(250.0*siz,10.0*siz,1,resol) ;

   glTranslatef(0.0, 60.0*siz,0.0) ;
   make_toroid(100.0*siz,10.0*siz,resol) ;

   glRotatef( 90.0,1.0,0.0,0.0) ;
   make_cylind(200.0*siz,5.0*siz,0,resol) ;
   glRotatef(-90.0,1.0,0.0,0.0) ;
   glRotatef( 90.0,0.0,0.0,1.0) ;
   make_cylind(200.0*siz,5.0*siz,0,resol) ;
   glRotatef(-90.0,0.0,0.0,1.0) ;

   glEndList() ;

   return(obj) ;
}

/**********************************************************************
*  make_toroid()  -
**********************************************************************/
static void make_toroid(flot32 R,flot32 r,sint32 resol)

{  register sint32 i,j,i2 ;
   register flot32 theta,phi,x,y,z,xc,yc,zc,cp,sp ;
   register P8     arr[16][32] ;

   for (theta=0.0,i=0; i<resol; theta+=2.0*M_PI/resol,i++)  {
     x = R + r*fcos(theta);
     y = r*fsin(theta);
     z = 0.0 ;

     xc = R ;
     yc = 0.0 ;
     zc = 0.0 ;

     for (phi=0.0,j=0; j<2*resol; phi+=M_PI/resol,j++)  {
       cp = fcos(phi) ;
       sp = fsin(phi) ;

       switch (((i&1)<<1) + (j&1))  {
         case 0: arr[i][j].t.s = 0.0 ; arr[i][j].t.t = 0.0 ; break ;
         case 1: arr[i][j].t.s = 1.0 ; arr[i][j].t.t = 0.0 ; break ;
         case 2: arr[i][j].t.s = 0.0 ; arr[i][j].t.t = 1.0 ; break ;
         case 3: arr[i][j].t.s = 1.0 ; arr[i][j].t.t = 1.0 ; break ;
         }

       arr[i][j].n.x = ( (x-xc)*cp + (z-zc)*sp)/r ;
       arr[i][j].n.y = ( (y-yc)               )/r ;
       arr[i][j].n.z = (-(x-xc)*sp + (z-zc)*cp)/r ; 

       arr[i][j].p.x =  x*cp + z*sp ;
       arr[i][j].p.y =  y ;
       arr[i][j].p.z = -x*sp + z*cp ; 
       }
     }

   for (i=0; i<resol; i++)  {
     i2 = ((i == resol-1) ? 0 : i+1) ;
     glBegin(GL_TRIANGLE_STRIP) ;
     for (j=0; j<2*resol; j++)  {
       glTexCoord2fv(&arr[i2][j].t.s) ;
       glNormal3fv(&arr[i2][j].n.x) ;
       glVertex3fv(&arr[i2][j].p.x) ;

       glTexCoord2fv(&arr[i ][j].t.s) ;
       glNormal3fv(&arr[i ][j].n.x) ;
       glVertex3fv(&arr[i ][j].p.x) ;
       }
     glTexCoord2fv(&arr[i2][0].t.s) ;
     glNormal3fv(&arr[i2][0].n.x) ;
     glVertex3fv(&arr[i2][0].p.x) ;

     glTexCoord2fv(&arr[i ][0].t.s) ;
     glNormal3fv(&arr[i ][0].n.x) ;
     glVertex3fv(&arr[i ][0].p.x) ;
     glEnd() ;
     }
}

/**********************************************************************
*  make_cylind()  -
**********************************************************************/
static void make_cylind(flot32 L,flot32 r,sint32 flag,sint32 resol)

{  register sint32 j ;
   register flot32 phi,cp,sp ;
   register P8     arr[2][16] ;

   for (phi=0.0,j=0; j<resol; phi+=2.0*M_PI/resol,j++)  {
     cp = fcos(phi) ;
     sp = fsin(phi) ;

     if (flag)  {
       if (j&1)  {
         arr[0][j].t.s =  0.0 ;
         arr[0][j].t.t =  0.0 ;
         arr[1][j].t.s =  0.0 ;
         arr[1][j].t.t = 32.0 ;
         }
       else  {
         arr[0][j].t.s =  0.5 ;
         arr[0][j].t.t =  0.0 ;
         arr[1][j].t.s =  0.5 ;
         arr[1][j].t.t = 32.0 ;
         }
       }
     else  {
       arr[0][j].t.s = 0.0 ;
       arr[0][j].t.t = 0.0 ;
       arr[1][j].t.s = 0.0 ;
       arr[1][j].t.t = 0.0 ;
       }

     arr[0][j].n.x = arr[1][j].n.x =  cp ;
     arr[0][j].n.y = arr[1][j].n.y = 0.0 ;
     arr[0][j].n.z = arr[1][j].n.z = -sp ; 

     arr[0][j].p.x = arr[1][j].p.x =  r*cp ;
     arr[0][j].p.y =  -L/2.0 ;
     arr[1][j].p.y =   L/2.0 ;
     arr[0][j].p.z = arr[1][j].p.z = -r*sp ; 
     }

   glBegin(GL_TRIANGLE_STRIP) ;
   for (j=0; j<resol; j++)  {
     glTexCoord2fv(&arr[1][j].t.s) ;
     glNormal3fv(&arr[1][j].n.x) ;
     glVertex3fv(&arr[1][j].p.x) ;

     glTexCoord2fv(&arr[0][j].t.s) ;
     glNormal3fv(&arr[0][j].n.x) ;
     glVertex3fv(&arr[0][j].p.x) ;
     }
   glTexCoord2fv(&arr[1][0].t.s) ;
   glNormal3fv(&arr[1][0].n.x) ;
   glVertex3fv(&arr[1][0].p.x) ;

   glTexCoord2fv(&arr[0][0].t.s) ;
   glNormal3fv(&arr[0][0].n.x) ;
   glVertex3fv(&arr[0][0].p.x) ;
   glEnd() ;
}

/**********************************************************************
*  object_planet_orbit()  - 
**********************************************************************/
void object_planet_orbit(register t_body *body,flot32 tilt)

{  register sint32 i ;
   register D3     f,center ;
   register flot64 t,save,delta ;
   register P3     *a,arr[256] ;

   center.x = 0.0;
   center.y = 0.0;
   center.z = 0.0;

   save = Counter.D ;
   delta = body->ptr->yer / 256.0 ;

   if (body->ptr->ecc < 0.8) 
     for (a=arr,i=0; i<256; a++,Counter.D+=delta,i++)  {
       calc_posit(&f,body->ptr,&center,tilt) ;
       a->x = f.x ;
       a->y = f.y ;
       a->z = f.z ;
       }
   else  {
     for (a=arr,i=0; i<256; a++,i++)  {
       t = i/256.0 ;
       if (t<=0.5)
         t = 2.0*t*t ;
       else t = (4.0 - 2.0*t)*t - 1.0 ;
       Counter.D = body->ptr->ee + t*body->ptr->yer ;

       calc_posit(&f,body->ptr,&center,tilt) ;
       a->x = f.x ;
       a->y = f.y ;
       a->z = f.z ;
       }
     }

   Counter.D = save ;

   body->orbtobj[0] = glGenLists(1);
   glNewList(body->orbtobj[0],GL_COMPILE) ;

   glBegin(GL_LINE_STRIP) ;
   for (a=arr,i=0; i<256; a++,i++)
     glVertex3fv(&a->x) ;
   glVertex3fv(&arr->x) ;
   glEnd() ;
   glEndList() ;

   body->orbtobj[1] = glGenLists(1);
   glNewList(body->orbtobj[1],GL_COMPILE) ;

   glBegin(GL_LINE_STRIP) ;
   for (a=arr,i=0; i<256; a+=4,i+=4)
     glVertex3fv(&a->x) ;
   glVertex3fv(&arr->x) ;
   glEnd() ;
   glEndList() ;

   body->orbtobj[2] = glGenLists(1);
   glNewList(body->orbtobj[2],GL_COMPILE) ;

   glBegin(GL_LINE_STRIP) ;
   for (a=arr,i=0; i<256; a+=16,i+=16)
     glVertex3fv(&a->x) ;
   glVertex3fv(&arr->x) ;
   glEnd() ;
   glEndList() ;
}

/**********************************************************************
*  object_planet_rings()  - 
***********************************************************************/
sint32 object_planet_rings(t_system *planet,sint32 reso) 

{  register sint32 i,obj ;
   register flot32 theta,delta,s,c,w ;
   register flot32 theta1,theta2,alpha ;
   register P3     p,q,r ;
   register T2     t1,t2 ;

   theta1 = fasin(planet->rad/planet->r1) ;
   theta2 = fasin(planet->rad/planet->r2) ;

   alpha = fasin(planet->rad/planet->r2) ; 
   delta = 2.0*M_PI/(flot32)reso ;  

   obj = glGenLists(1);
   glNewList(obj,GL_COMPILE) ;

   /* lit part */
   glColor4ub(0xff,0xff,0xff,0xa0) ;
   glBegin(GL_TRIANGLE_STRIP) ;

   t2.s = 1.0 ;
   t2.t = 1.0 ;

   for (theta=theta2; theta<2.0*M_PI-theta2+0.001; theta+=delta)  {
     s = fsin(theta) ;
     c = fcos(theta) ;

     q.x = planet->r2*s ;
     q.y = 0.0;
     q.z = planet->r2*c ;
     glTexCoord2fv(&t2.s) ;
     glVertex3fv(&q.x) ;

     if (theta < theta1)  {
       w = planet->rad / sin(M_PI-alpha-(theta-theta2)) ;
       t1.s = t1.t = (w-planet->r1)/(planet->r2-planet->r1) ;
       }
     else if (theta < 2.0*M_PI-theta1)  {
       w = planet->r1 ;
       t1.s = t1.t = 0.0 ;
       }
     else  {
       w = planet->rad / sin(M_PI-alpha-((theta1-theta2) - (theta-2.0*M_PI+theta1))) ;
       t1.s = t1.t = (w-planet->r1)/(planet->r2-planet->r1) ;
       }

     q.x = w*s ;
     q.y = 0.0;
     q.z = w*c ;
     glTexCoord2fv(&t1.s) ;
     glVertex3fv(&q.x) ;
     }

   q.x = planet->r2*fsin(-theta2) ;
   q.y = 0.0;
   q.z = planet->r2*fcos(-theta2) ;
   glTexCoord2fv(&t2.s) ;
   glVertex3fv(&q.x) ;

   glEnd() ;

   /* shadow part */
   glColor4ub(0xff,0xff,0xff,0x10) ;
   glBegin(GL_TRIANGLE_STRIP) ;

   t1.s = 0.0 ;
   t1.t = 0.0 ;

   for (theta= -theta1; theta<theta1+0.001; theta+=delta)  {
     s = fsin(theta) ;
     c = fcos(theta) ;

     q.x = planet->r1*s ;
     q.y = 0.0;
     q.z = planet->r1*c ;
     glTexCoord2fv(&t1.s) ;
     glVertex3fv(&q.x) ;

     if (theta < -theta2)  {
       w = planet->rad / sin(M_PI-alpha+theta2+theta) ;
       t2.s = t2.t = (w-planet->r1)/(planet->r2-planet->r1) ;
       }
     else if (theta < theta2)  {
       w = planet->r2 ;
       t2.s = t2.t = 1.0 ;
       }
     else  {
       w = planet->rad / sin(M_PI-alpha-(theta-theta2)) ;
       t2.s = t2.t = (w-planet->r1)/(planet->r2-planet->r1) ;
       }

     q.x = w*s ;
     q.y = 0.0;
     q.z = w*c ;
     glTexCoord2fv(&t2.s) ;
     glVertex3fv(&q.x) ;
     }

   q.x = planet->r1*fsin(theta1) ;
   q.y = 0.0;
   q.z = planet->r1*fcos(theta1) ;
   glTexCoord2fv(&t1.s) ;
   glVertex3fv(&q.x) ;

   glEnd() ;

   glEndList() ;
   return(obj) ;
}

/**********************************************************************
*  object_shadow_squares()  -
**********************************************************************/
sint32 object_shadow_squares(void)

{  register sint32 i,obj ;
   register flot32 v[3],h;

   h = M_PI/75.0;

   obj = glGenLists(1);
   glNewList(obj,GL_COMPILE) ;

   glBegin(GL_QUADS);
   for (i=0; i<360; i+=24)  {
     v[0] = fcos((i+ 0)*DTOR); v[1] = -h; v[2] = fsin((i+ 0)*DTOR); glVertex3fv(v);
     v[1] =  h; glVertex3fv(v);
     v[0] = fcos((i+12)*DTOR); v[1] =  h; v[2] = fsin((i+12)*DTOR); glVertex3fv(v);
     v[1] = -h; glVertex3fv(v);
     }
   glEnd();

   glEndList() ;
   return(obj) ;
}

