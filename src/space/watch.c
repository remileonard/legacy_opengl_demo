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
#include <sys/time.h>
#include <GL/gl.h>
#include "space.h" 

typedef struct  {
        sint32 year ;
        sint32 month ;
        sint32 day ;
        sint32 hour ;
        sint32 min ;
        flot64 sec ;
        flot64 jd ;
} t_time ;

       int            timer_flag = 0 ;
static struct timeval bsdtime,lastime ;
static flot64         epochdate = 2447891.5 ;
static flot64         savet = 0 ;
extern t_stopwatch Counter ;

static void   julian_date(t_time *) ;

/****************************************************************************
*  check_timer()  - 
****************************************************************************/
flot64 check_timer(void)

{  struct timezone tzp;
   struct timeval  tp;
   int             sec,usec;
   t_time          tt ;
   flot64          q ;
   
   if (!timer_flag)  {
     timer_flag = 1 ;

     gettimeofday(&bsdtime, &tzp);
     lastime = bsdtime ;

     tt.year  = 1970 ;
     tt.month = 1 ;
     tt.day   = 1 ;
     tt.hour  = 0 ;
     tt.min   = 0 ;
     tt.sec   = 1.0e-6 * bsdtime.tv_usec;
     tt.sec   += bsdtime.tv_sec ;
     julian_date(&tt) ;

     Counter.D = tt.jd ;
     printf("Julian Date: %f\n",Counter.D + epochdate) ;
     }

   gettimeofday(&tp, &tzp);

   sec  = tp.tv_sec  - lastime.tv_sec;
   usec = tp.tv_usec - lastime.tv_usec;
   if (usec < 0) {
      sec--;
      usec += 1000000;
      }
   q  = 1.0e-6*usec ;
   q += sec ;
   
   if (Counter.flags & TMREV_FLAG)
     q = -q ;

   Counter.D += (Counter.timacc*q) / (24.0*3600.0) ;

   sec  = tp.tv_sec  - bsdtime.tv_sec;
   usec = tp.tv_usec - bsdtime.tv_usec;
   if (usec < 0) {
      sec--;
      usec += 1000000;
      }
   q  = 1.0e-6*usec ;
   q += sec ;

   lastime = tp ;
   return(q) ;
}

/****************************************************************************
*  delta_timer()  - 
****************************************************************************/
flot64 delta_timer(void)

{  struct timezone tzp;
   struct timeval  tp;
   int             sec,usec;
   flot64          q ;
   
   gettimeofday(&tp, &tzp);

   sec  = tp.tv_sec  - lastime.tv_sec;
   usec = tp.tv_usec - lastime.tv_usec;
   if (usec < 0) {
      sec--;
      usec += 1000000;
      }
   q  = 1.0e-6*usec ;
   q += sec ;
   
   if (Counter.flags & TMREV_FLAG)
     q = -q ;

   return((Counter.timacc*q) / (24.0*3600.0)) ;
}

/****************************************************************************
*  julian_date()  - 
****************************************************************************/
static void julian_date(t_time *tt)

{  register sint32 B,C,D,julian,year,month ;

   year  = tt->year ;
   month = tt->month ;

   if (year < 1582)
     julian = 0 ;
   else if (year > 1582)
     julian = 1 ;
   else if (month < 10)
     julian = 0 ;
   else if (month > 10)
     julian = 1 ;
   else if (tt->day < 15)
     julian = 0 ;
   else julian = 1 ;

   if (month == 1 || month == 2)  {
     year-- ;
     month+=12 ;
     }

   if (julian)
     B = 2 - (year/100) + (year/400) ;
   else B = 0 ;

   if (year < 0)
     C = (flot64) 365.25 * year - 0.75 ;
   else C = (flot64) 365.25 * year ;

   D = (flot64) 30.6001*(month+1) ;

   tt->jd  = (flot64) B + C + D + tt->day + (1720994.5 - epochdate) +
                        (tt->hour + tt->min/60.0 + tt->sec/3600.0)/24.0 ;
}

/****************************************************************************
*  reverse_julian_date()  - 
****************************************************************************/
void reverse_julian_date(flot64 jd,char *date)

{  register flot64 q,f,dd ;
   register sint32 i,b,a,c,d,e,g,mm,yy,day,hour,min ;

   q = jd + 0.5 + epochdate ;
 
   i = (int) q ;
   f = q - i ;

   if (i > 2299160)  {
     a = ((flot64)i - 1867216.25) / 36524.25 ;
     b = i + 1 + a - (a>>2) ;
     }
   else b = i ;

   c = b + 1524 ;

   d = ((flot64)c - 122.1) / 365.25 ;

   e = 365.25 * (flot64)d ;

   g = ((flot64)c - (flot64)e) / 30.6001 ;

   dd = c - e + f - (sint32)(30.6001*(flot64)g) ;
   dd -= 8.0/24.0 ;

   mm = ((g < 13.5) ? g-1 : g-13) ;

   yy = ((mm > 2.5) ? d-4716 : d-4715) ;

   day  = (sint32)dd ;
   q    = 24.0*(dd-day) ;
   hour = (sint32)q ;
   q    = 60.0 * (q-hour) ;
   min  = (sint32)q ;
   
   sprintf(date,"Date : %02d:%02d %02d/%02d %04d",hour,min,day,mm,yy) ;
}

