/*
 * Copyright 1991, 1992, 1993, 1994, Silicon Graphics, Inc.
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
/*
 * sbinteg.c
 *		Spaceball Integration Module
 *
 * Written By Gianni Mariani 10-Jul-1989
 *
 *	
 * Copyright (C) 1989 Spatial Systems Inc. All Rights Reserved.
 * Spaceball is a registered trademark of Spatial Systems Inc.
 *
 * RESTRICTIONS
 *
 * This module may be used and distributed freely provided the
 * routines are used exclusively in conjunction with a Spaceball (TM). No
 * portion may be used for other purposes without prior written permission
 * from Spatial Systems Inc. This notice must accompany any full or partial
 * copies.
 *
 * PURPOSE
 *
 * This module contains handling routines for Spaceball events.  This
 * is intended to be modified by each application developer to suit
 * their own needs.  It is suggested however that the keypad functionality
 * be left somewhat the same as it is here so that users become accustomed
 * to the functionality and hence reduce the learning time between systems
 * that they use.
 *
 * HOW
 *
 * Programming Spaceball on an Iris workstation
 * 
 *     Iris workstations allow access to the Spaceball through the 
 * Silicon Graphics GL event queue system.  The Spaceball interface 
 * has been implemented as 7 new 'delta' devices and 9 new button
 * devices.  In addition there are a number of new calls to 
 * allow you to control Spaceball.
 *
 *     This module provides all the nessasary code to manipulate the
 * Spaceball GL interface.  This module will present the information
 * to the application you wish to integrate in an easy to use form.
 * You can use the Spaceball GL interface directly to perform other
 * operations not provided in this module.
 * 
 * This module's interface consists of 5 routines that can be called from
 * well defined places in the source.  These are :
 * 
 * 	sbInit()		- Initialize module sbinteg
 *				  Returns true if a Spaceball is
 *				  found, false if not.
 * 	sbEvent(dev, val) 	- Call after every call to qread
 * 				  Returns 1 if it is a Spaceball event.
 * 				  Note: it needs device INPUTCHANGE
 * 				  to correctly run the Spaceball I/F.
 * 	sbMapply( ... )		- Apply changes required.
 * 
 * 	sbStatus()		- Return a status string to be 
 * 				  displayed
 * 	sbPrmpt()		- Prompt Spaceball for a new reading.
 * 
 *     The user must supply two routines, they are spcbal_do and spchome.
 * These functions are specific to each system and can be handled differently
 * for each system.
 *
 *	The user provided routines can be coded as follows.
 * ******************************************************************
 * / * ==================== spcbal_do ========================== * /
 * / * This routine is called by sbinteg.c when a Spaceball event has
 *   * occurred.  This routine will use this information to manipulate
 *   * a viewing matrix.
 *   * /
 * spcbal_do( rotvec, rotscale, transvec, transcale )
 * Coord 	rotvec[ 3 ];
 * float	rotscale;
 * Coord	transvec[ 3 ];
 * float	transcale;
 * {
 *
 *	/ * Apply the incremental rotation and translation to the
 *	  * viewing matrix
 *	  * /
 * 	sbMapply( 
 *		view_matrix,	/ * The viewing matrix 		* /
 * 		fov,		/ * The field of view		* / 
 *		aspect, 	/ * The aspect ratio		* /
 *		objrad,		/ * The object radius		* / 
 * 		rotvec, 	/ * Passed Rotation vector	* /
 * 		rotscale, 	/ * Passed Rotation scale	* /
 *		transvec,	/ * Passed Translation vector	* /
 *		transcale	/ * Passed Translation scale	* /
 *	);
 *
 *	/ * Screen update required flag needs to be set * /
 *	screen_update_required = TRUE;
 *	
 * }
 *
 * / * ====================== spchome ====================== * /
 * / * This routine is called when the user requests that the image
 *   * is reset to the initial position.  This usually means setting
 *   * the viewing matrix to the identity.
 *   * /
 * spchome()
 * {
 *	int	i, j;
 *	/ * Set the view matrix to the identity * /
 *	for ( i = 0; i < 4; i ++ ) {
 *		for ( j = 0; j < 4; j ++ ) {
 *			view_matrix[ i ][ j ] = ( i == j );
 *		}
 *	}
 * }
 *
 * ******************************************************************
 *
 * This module provides default functionality for the Spaceball buttons.
 * This functionality can be changed by the user, however it is
 * recommended that these are not changed to allow users to switch
 * between applications without requiring to go through a new
 * learning curve.
 *
 *
 * The buttons are set to provide the following functionality.
 *
 *	SBBUT1 - Spacball keypad 1
 *	SELECT TRANSLATION MODE/SET TRANSLATION SCALE FACTOR
 *	press one	- Turn translations off
 *			  One beep.
 *	press two	- Select dominant translation ( This
 *			  allows you to select the largest absolute
 *			  element of the translation vector - it will
 *			  provide pure x y or z translations.
 *			  Two beeps.
 *	press three	- Turn translations back on.
 *			  Three beeps.
 *	pressed directly after pressing buttons 5 or 6 sets the
 *	scale factor for translations.
 *
 *	SBBUT2 - Spaceball keypad 2
 *	SELECT ROTATION MODE/SET ROTATION SCALE FACTOR
 *	press one	- Turn rotations off
 *			  One beep.
 *	press two	- Select dominant rotation ( This
 *			  allows you to select the largest absolute
 *			  element of the rotation vector - it will
 *			  provide pure rotations about the x y or z 
 *			  axes.
 *			  Two beeps.
 *	press three	- Turn rotations back on.
 *			  Three beeps.
 *	pressed directly after pressing buttons 5 or 6 sets the
 *	scale factor for rotations.
 *
 *	SBBUT3 - Spaceball keypad 3
 *	SELECT MOUSE/SPACEBALL MODE
 *	Toggle between mouse and Spaceball mode.
 *	When entering mouse mode there will be three short beeps
 *	when entering spaceball mode there will be one short beep.
 *
 *	SBBUT4 - Spaceball keypad 4
 *	SELECT OBJECT/EYEPOINT CONTROL MODE
 *	Toggle between object control and eyepoint control modes.
 *	Object control allows the user to manipulate the view as if
 *	the object is at the Spaceball, i.e. exerting a force at the
 *	Spaceball is like exerting a force on the object.
 *	Eyepoint control allows the user to manipulate the eyepoint
 *	so that the forces exerted at the Spaceball are placed on
 * 	the eyepoint of the system.
 *
 *	SBBUT5 - Spaceball keypad 5
 *	SET LARGER SCALE FACTOR
 *	This allows the changing of rotation and translation scale 
 *	factors.  Every hit to this key sets the scale factor up
 *	by 20%.  After striking this key a number of times you may
 *	choose it to apply to translations or rotations by 
 *	preesing keypad 1 or 2.	
 *
 *	SBBUT6 - Spaceball keypad 6
 *	SET SMALLER SCALE FACTOR
 *	This allows the changing of rotation and translation scale 
 *	factors.  Every hit to this key sets the scale factor down
 *	by 20%.  After striking this key a number of times you may
 *	choose it to apply to translations or rotations by 
 *	preesing keypad 1 or 2.	
 *
 *	SBBUT7 - Spaceball keypad 7
 *	--- UNUSED ---
 *	You may find an interesting function for this key !
 *
 *	SBBUT8 - Spaceball keypad 8
 *	REZERO SPACEBALL
 *	This will ask Spaceball to rezero itself. This can be
 *	used to eliminate drift.
 *	
 *	SBPICK - Spaceball pick button
 *	Reset the image to original position.
 *
 * 
 * ****************************************************************  
 *
 *	To display a Spaceball status message you may call the
 * function sbStatus() to return a pointer to a string containing the
 * current Spaceball status.  This message should be displayed on every
 * screen update.
 *
 * By using this module you can make the integration of Spaceball into
 * your code very quickly and easily.  Feel free to modify this module
 * to change the functionality to suit your application.
 *
 */

#ifdef spaceball

/* ===================== INCLUDE FILES ======================= */

#include <math.h>
#include <gl.h>
#include <gl/device.h>
#include <gl/spaceball.h>

/* ===================== MACROS ============================== */

#define veclength( v )  sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] )

#define lengthof( array )       (sizeof( array )/sizeof( array[0] ))

#define SBM_ON		0
#define SBM_OFF		1
#define SBM_DOMINANT	2

/* ===================== SBXEVENT STRUCT ===================== */
/* This structure is used to make a table of devices to be used
 * by this module
 */

typedef struct {
    short	  dev;		/* Device number */
    char	* str;		/* Textual rep of dev - for debug if reqd*/
    short	  ret_val;	/* Set to true if it is a spaceball event */
    int	      ( * func )();	/* Function to call when event occurs */
    int		  param;	/* Parameter to the function */
} sbx_event;

/* ===================== SBX_TABLE =========================== */
/* Every device that needs to be captured is represented in this 
 * table
 */

/* The following is a set of function declarations that will be
 * used in the table to perform actions from events processed. */
int	sbx_period();
int	sbx_t();
int	sbx_r();
int	sbx_k1();
int	sbx_k2();
int	sbx_k3();
int	sbx_k4();
int	sbx_k5();
int	sbx_k6();
int	sbx_k7();
int	sbx_k8();
int	sbx_pick();
int	sbx_inputchange();

sbx_event	sbx_table[] = {
{
    SBPERIOD,	/* short  dev;	Device number */
    "SBPERIOD",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_period,	/* int ( * func )();Function to call when event occurs */
    0		/* int  param;Parameter to the function */
},
{
    SBTX,	/* short  dev;	Device number */
    "SBTX",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_t,	/* int ( * func )();Function to call when event occurs */
    0		/* int  param;Parameter to the function */
},
{
    SBTY,	/* short  dev;	Device number */
    "SBTY",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_t,	/* int ( * func )();Function to call when event occurs */
    1		/* int  param;Parameter to the function */
},
{
    SBTZ,	/* short  dev;	Device number */
    "SBTZ",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_t,	/* int ( * func )();Function to call when event occurs */
    2		/* int  param;Parameter to the function */
},
{
    SBRX,	/* short  dev;	Device number */
    "SBRX",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_r,	/* int ( * func )();Function to call when event occurs */
    0		/* int  param;Parameter to the function */
},
{
    SBRY,	/* short  dev;	Device number */
    "SBRY",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_r,	/* int ( * func )();Function to call when event occurs */
    1		/* int  param;Parameter to the function */
},
{
    SBRZ,	/* short  dev;	Device number */
    "SBRZ",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_r,	/* int ( * func )();Function to call when event occurs */
    2		/* int  param;Parameter to the function */
},
{
    SBBUT1,	/* short  dev;	Device number */
    "SBBUT1",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_k1,	/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
{
    SBBUT2,	/* short  dev;	Device number */
    "SBBUT2",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_k2,	/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
{
    SBBUT3,	/* short  dev;	Device number */
    "SBBUT3",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_k3,	/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
{
    SBBUT4,	/* short  dev;	Device number */
    "SBBUT4",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_k4,	/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
{
    SBBUT5,	/* short  dev;	Device number */
    "SBBUT5",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_k5,	/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
{
    SBBUT6,	/* short  dev;	Device number */
    "SBBUT6",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_k6,	/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
{
    SBBUT7,	/* short  dev;	Device number */
    "SBBUT7",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_k7,	/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
{
    SBBUT8,	/* short  dev;	Device number */
    "SBBUT8",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_k8,	/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
{
    SBPICK,	/* short  dev;	Device number */
    "SBPICK",	/* char	* str;	Textual rep of dev - for debug if reqd*/
    1,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_pick,	/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
{
    INPUTCHANGE,/* short  dev;	Device number */
    "INPUTCHANGE",/* char* str;	Textual rep of dev - for debug if reqd*/
    0,		/* short  ret_val;Set to true if it is a spaceball event */
    sbx_inputchange,/* int ( * func )();Function to call when event occurs */
    0  		/* int  param;Parameter to the function */
},
};	/* sbx_table */

/* Set to true if spaceball is open */
int	sbv_open = 0;

/* ======================= sbInit ========================== */
/* Check to see that Spaceball exists and queue all the 
 * Spaceball devices.
 */

sbInit()
{
    int         i;

    /* If it is there then assume it exists */
    if ( sbv_open ) {
	return 1;
    }

    /* Make sure that a spaceball exists */
    if ( !sbexists() ) {
	return 0;
    }

    sbv_open = 1;
    /* queue all the devices in the table */
    for ( i = 0; i < lengthof( sbx_table ); i ++ ) {
        qdevice( sbx_table[ i ].dev );
    }

    return 1;
}

/* ======================= sbEvent ======================== */
/* sbEvent must be called for every event that needs to be trapped
 * by this module.  It can be called for every event recieved
 * and will return true if the event is a special Spaceball
 * event and will return false if is not.  Note however that
 * it will use the events from the INPUTCHANGE device but will
 * return false.
 */
sbEvent( dev, val )
short	dev;
short	val;
{
    int		i;

    /* Make sure that the Spaceball is open */
    if ( ! sbv_open ) {
	return 0;
    }

    /* Scan through the table of devices to check for devices
     * to be trapped.
     */
    for ( i = 0; i < lengthof( sbx_table ); i ++ ) {
	if ( dev == sbx_table[ i ].dev ) {
	    (* (sbx_table[ i ].func))( dev, val, sbx_table[ i ].param );
	    return sbx_table[ i ].ret_val;
	}
    }

    /* Was not a Saceball event */
    return 0;
}



/* ================== Values ========================= */

#define IDENTITY_MATRIX         { \
        { 1, 0, 0, 0 }, \
        { 0, 1, 0, 0 }, \
        { 0, 0, 1, 0 }, \
        { 0, 0, 0, 1 }  \
  }

int	sbv_prompt = 1;	/* True if a prompt is required */

int	sbv_mask = 0;	/* Used to determine which events are still */
			/* requred to complete a block of data from */
			/* Spaceball.				    */


typedef struct {	/* used for type casting */
    Matrix	mat;
} sbt_mat;

/* =============== Values from Spaceball ================== */
int	sbv_period;	/* Period between readings */

Coord	sbv_t[ 3 ];	/* Translation vector */

Coord	sbv_r[ 3 ];	/* Rotation axis vector */

Coord	sbv_tvec[ 3 ];	/* Scaled trsnalation vector */

int	sbv_roff;	/* Set to true if rots are off */
int	sbv_toff;	/* Set to true if trans are off */

float	sbr_translation_rate = .006;
float  	sbr_rotation_rate = .015;

int	sbv_tranmode = 0;/* Translation mode */
int	sbv_rotmode = 0; /* Rotation mode */

int	sbv_eyemode = 0;	/* Mode of operation */

int	sbv_status_update = 1;	/* Does the status line need update ? */

int	sbv_stscale = 0;	/* True when change scale seleceted */

float	sbv_adjust_scale = 1.0;	/* The factor to change a scale by */

int	sbv_inmouse = 0;	/* Assume we are in Spaceball mode */

/* ================== Functions ======================= */

/* Called when a SBPERIOD event occurs */
int	sbx_period( dev, val, arg )
short	dev;
short	val;
int	arg;
{
    sbv_period = val;
    sbv_check( 1, 0 );
}


/* Called for events from devices SBTX, SBTY, SBTZ */
int	sbx_t( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    /* Check to see if translations have been set to off */
    if ( ! sbv_toff ) {
	sbv_t[ arg ] = ldexp( (float) val, -14 );
    } else {
	sbv_t[ arg ] = 0.0;
    }

    sbv_check( 2 << arg, 0 );
}


/* Called for events from devices SBRX, SBRY, SBRZ */
int	sbx_r( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    /* Check to see if rotations have been set to off */
    if ( ! sbv_roff ) {
	sbv_r[ arg ] = ldexp( (float) val, -14 );
    } else {
	sbv_r[ arg ] = 0.0;
    }

    sbv_check( (0x10 << arg), arg == 2 );
}

/* This function detects when all expected information from
 * Spaceball has arrived.  When this happens it will call
 * spcbal_do(...)
 */
sbv_check( val, complete )
int	val;
int	complete;
{
    float	rscale;
    float	tscale;

    /* Did we get all the values ? */
    if ((sbv_mask |= val) == 0x7f) { 

	/* Was the last value rot z ? */
	if ( !complete ) {
	    return;
	}

	sbv_prompt = 1;		/* Send a prompt next time */
	
	/* If both rotations and translations are off then */
	/* don't bother doing any work */
	if ( (sbv_rotmode == SBM_OFF) && (sbv_tranmode == SBM_OFF) ) {
	    return;
	}

	/* Calculate scale factors */
	tscale = sbr_translation_rate * sbv_period;	
	rscale = sbr_rotation_rate * sbv_period;	

	/* Check to see if we have dominant mode */
	if (sbv_rotmode == SBM_DOMINANT) {
	    sbx_dominant( sbv_r );
	}
	if (sbv_tranmode == SBM_DOMINANT) {
	    sbx_dominant( sbv_t );
	}


	spcbal_do( sbv_r, rscale, sbv_t, tscale );  /* Call the application */

    }
}


/* Apply the Spaceball stuff to a matrix */
sbMapply( mat, fov, aspect, objrad, sbv_r, rscale, sbv_t, tscale )
Matrix		mat;
float		fov;		/* Field of view */
float		aspect;		/* Aspect ratio  */
float		objrad;		/* Object radius */
Coord	      * sbv_r;
float		rscale;
Coord	      * sbv_t;
float		tscale;
{
    Coord	vec[ 3 ];

    vec[ 0 ] = mat[ 3 ][ 0 ];
    vec[ 1 ] = mat[ 3 ][ 1 ];
    vec[ 2 ] = mat[ 3 ][ 2 ];

    mat[ 3 ][ 0 ] = 0.0;
    mat[ 3 ][ 1 ] = 0.0;
    mat[ 3 ][ 2 ] = 0.0;

    sbMvapply( mat, vec, fov, aspect, objrad, sbv_r, rscale, sbv_t, tscale );

    mat[ 3 ][ 0 ] = vec[ 0 ];
    mat[ 3 ][ 1 ] = vec[ 1 ];
    mat[ 3 ][ 2 ] = vec[ 2 ];
    
}

/* utility routine to scale a vector
 */
sbx_scalevec( result, source, scale )
Coord	    * result;
Coord	    * source;
float	      scale;
{
    int		i;

    /* do it for all the elements */
    for ( i = 0; i < 3; i ++ ) { 
	* (result ++) = * (source ++) * scale;
    }
}
	
/* Choose dominant component
 */
sbx_dominant( vec )
float vec[3];
{
    if (fabs( vec[0] ) > fabs( vec[1] )) {
	vec[1] = 0.0;
	if (fabs( vec[0] ) > fabs( vec[2] ))
	    vec[2] = 0.0;
	else
	    vec[0] = 0.0;
    } else {
	vec[0] = 0.0;
	if (fabs( vec[1] ) > fabs( vec[2] ))
	    vec[2] = 0.0;
	else
	    vec[1] = 0.0;
    }
}

/* utility routines to handle wierd matrix multiplies
 */
typedef struct {
    Matrix	m;
} mattype;

static
void
m3x3mult( m1, m2, prod )
mattype		*m1, *m2, *prod;
{
        register int 	row, col;
        mattype 	temp[1];

	* temp = * prod;

        for(row=0 ; row<3 ; row++) {
                for(col=0 ; col<3 ; col++) {
                        temp->m[row][col] = 
				m1->m[row][0] * m2->m[0][col]
                              + m1->m[row][1] * m2->m[1][col]
                              + m1->m[row][2] * m2->m[2][col]
                              + m1->m[row][3] * m2->m[3][col];
		}
	}

	* prod = * temp;
}

static
void
m4x3mult( m1, m2, prod )
mattype		*m1, *m2, *prod;
{
        register int 	row, col;
        mattype 	temp[1];

	* temp = * prod;

        for(row=0 ; row<4 ; row++) {
                for(col=0 ; col<3 ; col++) {
                        temp->m[row][col] = 
				m1->m[row][0] * m2->m[0][col]
                              + m1->m[row][1] * m2->m[1][col]
                              + m1->m[row][2] * m2->m[2][col]
                              + m1->m[row][3] * m2->m[3][col];
		}
	}

	* prod = * temp;
}


/* Same as sbMapply except works on a rotation matrix and a translation
 * vector
 */
sbMvapply( mat, vec, fov, aspect, objrad, rotvec, rscale, tranvec, tscale )
Matrix		mat;		/* Orientation of object */
Coord	      * vec;		/* Position of object */
float           fov;            /* Field of view */
float           aspect;         /* Aspect ratio  */
float           objrad;         /* Object radius */
Coord         * rotvec;
float           rscale;
Coord         * tranvec;
float           tscale;
{

    float	v[ 3 ];
    float	objdist;
    float	bentfactor;
    Matrix 	delta_matrix;
    Coord	sbv_t[ 3 ];
    Coord	sbv_r[ 3 ];
    Coord	bentvec[ 3 ];

    /* Scale the vectors */
    sbx_scalevec( sbv_r, rotvec, rscale );
    sbx_scalevec( sbv_t, tranvec, tscale );

    if ( sbv_eyemode ) {
	vec[ 0 ] -= sbv_t[ 0 ] * objrad;
	vec[ 1 ] -= sbv_t[ 1 ] * objrad;
	vec[ 2 ] += sbv_t[ 2 ] * objrad;
    } else {

	v[2] = vec[ 2 ] - 1.1;

	if ( v[2] > -objrad ) {
	    vec[ 0 ] += sbv_t[ 0 ] * objrad;
	    vec[ 1 ] += sbv_t[ 1 ] * objrad;
	    vec[ 2 ] -= sbv_t[ 2 ] * objrad;
	} else {
	    v[ 0 ] = vec[ 0 ];
	    v[ 1 ] = vec[ 1 ];
	    objdist = veclength( v );
	    bentfactor = sbv_t[2] / objdist;

	    bentvec[ 0 ] = v[ 0 ] * bentfactor;
	    bentvec[ 1 ] = v[ 1 ] * bentfactor;
	    bentvec[ 2 ] = v[ 2 ] * bentfactor;

	    vec[ 0 ] += objdist * (sbv_t[0] + bentvec[0]);
	    vec[ 1 ] += objdist * (sbv_t[1] + bentvec[1]);
	    vec[ 2 ] += objdist * (           bentvec[2]);
	}
    }

    /* Make sure that rotations are on */
    if (sbv_roff) {
    	return;
    }

    if ( sbv_eyemode ) {
	
	rotarbaxis( -1.0, -sbv_r[0], -sbv_r[1], sbv_r[2], delta_matrix );

	mat[3][0] = vec[0];
	mat[3][1] = vec[1];
	mat[3][2] = vec[2] - 0.1;

	m4x3mult( mat, delta_matrix, mat );

	vec[0] = mat[3][0];
	vec[1] = mat[3][1];
	vec[2] = mat[3][2] + 0.1;

	mat[3][0] = 0.0;
	mat[3][1] = 0.0;
	mat[3][2] = 0.0;
    } else {
	/* Object mode */

	rotarbaxis( 1.0, -sbv_r[0], -sbv_r[1], sbv_r[2], delta_matrix );

	m3x3mult( mat, delta_matrix, mat );
    }

}


#ifdef EULERS
/* Same as sbMvapply except works on Euler rotation parameters and a 
 * translation vector
 */
sbEvapply( e, vec, fov, aspect, objrad, rotvec, rscale, tranvec, tscale )
Coord	      * e;		/* Orientation of object */
Coord	      * vec;		/* Position of object */
float           fov;            /* Field of view */
float           aspect;         /* Aspect ratio  */
float           objrad;         /* Object radius */
Coord         * rotvec;
float           rscale;
Coord         * tranvec;
float           tscale;
{

    float	v[ 3 ];
    float	objdist;
    float	bentfactor;
    Matrix 	delta_matrix;
    Coord	sbv_t[ 3 ];
    Coord	sbv_r[ 3 ];
    Coord	bentvec[ 3 ];
    Coord	ex[ 4 ];
    Matrix	mat;

    /* Scale the vectors */
    sbx_scalevec( sbv_r, rotvec, rscale );
    sbx_scalevec( sbv_t, tranvec, tscale );

    if ( sbv_eyemode ) {
	vec[ 0 ] -= sbv_t[ 0 ] * objrad;
	vec[ 1 ] -= sbv_t[ 1 ] * objrad;
	vec[ 2 ] += sbv_t[ 2 ] * objrad;
    } else {
	v[2] = vec[ 2 ] - 1.1;

	if ( v[2] > -objrad ) {
	    vec[ 0 ] += sbv_t[ 0 ] * objrad;
	    vec[ 1 ] += sbv_t[ 1 ] * objrad;
	    vec[ 2 ] -= sbv_t[ 2 ] * objrad;
	} else {
	    v[ 0 ] = vec[ 0 ];
	    v[ 1 ] = vec[ 1 ];
	    objdist = veclength( v );
	    bentfactor = sbv_t[2] / objdist;

	    bentvec[ 0 ] = v[ 0 ] * bentfactor;
	    bentvec[ 1 ] = v[ 1 ] * bentfactor;
	    bentvec[ 2 ] = v[ 2 ] * bentfactor;

	    vec[ 0 ] += objdist * (sbv_t[0] + bentvec[0]);
	    vec[ 1 ] += objdist * (sbv_t[1] + bentvec[1]);
	    vec[ 2 ] += objdist * (           bentvec[2]);
	}
    }

    /* Make sure that rotations are on */
    if (sbv_roff) {
    	return;
    }

    if ( sbv_eyemode ) {
	
	axistoeuler( -1.0, -sbv_r[0], -sbv_r[1], sbv_r[2], ex );

	add_eulers( ex, e, e );

	rotarbaxis( -1.0, -sbv_r[0], -sbv_r[1], sbv_r[2], delta_matrix );

	*((mattype *) mat) = *((mattype *) delta_matrix);

	mat[3][0] = vec[0];
	mat[3][1] = vec[1];
	mat[3][2] = vec[2] - 1.1;

	m4x3mult( mat, delta_matrix, mat );

	vec[0] = mat[3][0];
	vec[1] = mat[3][1];
	vec[2] = mat[3][2] + 1.1;

    } else {
	/* Object mode */

	axistoeuler( 1.0, -sbv_r[0], -sbv_r[1], sbv_r[2], ex );

	add_eulers( ex, e, e );
    }

}

/* axistoeuler will take an axis rotation vector and create an equivalent
 * Euler rotation vector.
 */
axistoeuler( scale, x_rotvec, y_rotvec, z_rotvec, e )
float	  scale;
Coord	  x_rotvec;
Coord	  y_rotvec;
Coord	  z_rotvec;
Coord	* e;
{
    float
      radians, radians2, invlength, myx_rotvec, myy_rotvec, myz_rotvec,
      sin02, cos02;

    /* Find the length of the vector */
    radians = sqrt( x_rotvec*x_rotvec + y_rotvec*y_rotvec + z_rotvec*z_rotvec );

    /* If the vector has zero length - return the identity matrix */
    if (radians == 0.0) {
        int	i;
	int	j;

	e[0] = 0.0;
	e[1] = 0.0;
	e[2] = 0.0;
	e[3] = 1.0;

        return;
    }

    /* normalize the rotation vector */
    invlength = 1 / radians;
    myx_rotvec = x_rotvec * invlength;
    myy_rotvec = y_rotvec * invlength;
    myz_rotvec = z_rotvec * invlength;
    radians2  = radians * scale/2;   /* calculate the number of radians */

    /* Do all the smart stuff */
    sin02 = sin( radians2 );
    cos02 = cos( radians2 );

    e[0] = myx_rotvec * sin02;
    e[1] = myy_rotvec * sin02;
    e[2] = myz_rotvec * sin02;
    e[3] = cos02;

}

#endif


/* Error handler */
sbx_error( str )
{
    printf( "%s error : %s", __FILE__, str );
    exit( 2 );
}

/* ===================== sbx_k? ========================== */
/* These routines handle keypad events.
 */

int	sbx_k2( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    if ( val ) {

	if ( sbv_stscale ) {		/* if we are setting the scale */
	    sbr_rotation_rate *= sbv_adjust_scale;
	    sbv_adjust_scale = 1;	/* Initialize it to 0 */
	    sbv_stscale = 0;		/* Scale factor has been used */
	    sbv_status_update = 1;	/* Update the status line */
	    sbx_beep( 1 );
	    return;
	}

	/* Rotation modes */
	switch ( sbv_rotmode ) {
	    case SBM_ON : {
		/* Rotations are on - turn them off */
		sbv_rotmode = SBM_OFF;
		sbv_roff = 1;
		sbv_status_update = 1;		/* Update the status line */
		sbx_beep( 1 );			/* do a type 1 beep */
		break;
	    }
	    case SBM_OFF : {
		/* Rotations are off - go to dominant mode */
		sbv_rotmode = SBM_DOMINANT;
		sbv_roff = 0;
		sbv_status_update = 1;
		sbx_beep( 2 );
		break;
	    }
            case SBM_DOMINANT : {
                /* Rotations are off - go to dominant mode */
                sbv_rotmode = SBM_ON;
                sbv_roff = 0;
                sbv_status_update = 1;
                sbx_beep( 3 );
                break;
            }
	    default : {
		sbx_error( "Bug 1" );	/* Should never get here */
		break;
	    }
	}
   } else {
	/* k2 has been released */
   }
}



int	sbx_k1( dev, val, arg )
short   dev;
short   val;
int     arg;
{

    if ( val ) {

	if ( sbv_stscale ) {		/* if we are setting the scale */
	    sbr_translation_rate *= sbv_adjust_scale;
	    sbv_adjust_scale = 1;	/* Initialize it to 0 */
	    sbv_stscale = 0;		/* Scale factor has been used */
	    sbv_status_update = 1;	/* Update the status line */
	    sbx_beep( 1 );
	    return;
	}

	/* Rotation modes */
	switch ( sbv_tranmode ) {
	    case SBM_ON : {
		/* Rotations are on - turn them off */
		sbv_tranmode = SBM_OFF;
		sbv_toff = 1;
		sbv_status_update = 1;		/* Update the status line */
		sbx_beep( 1 );			/* do a type 1 beep */
		break;
	    }
	    case SBM_OFF : {
		/* Rotations are off - go to dominant mode */
		sbv_tranmode = SBM_DOMINANT;
		sbv_toff = 0;
		sbv_status_update = 1;
		sbx_beep( 2 );
		break;
	    }
            case SBM_DOMINANT : {
                /* Rotations are off - go to dominant mode */
                sbv_tranmode = SBM_ON;
                sbv_toff = 0;
                sbv_status_update = 1;
                sbx_beep( 3 );
                break;
            }
	    default : {
		sbx_error( "Bug 1" );	/* Should never get here */
		break;
	    }
	}
   } else {
	/* k1 has been released */
   }
}



int	sbx_k3( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    if ( val ) {
	/* go into mouse mode / spaceball mode */
	if ( ! sbv_inmouse ) {
	    sbv_inmouse = 1;
	    sbmouse();
	    sbx_beep( 4 );
	    sbv_status_update = 1;
	} else {
	    sbv_inmouse = 0;
	    sbspaceball();
	    sbx_beep( 5 );
	    sbv_status_update = 1;
	}
    } else {
    }
}



int	sbx_k4( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    if ( val ) {
	/* go into mouse mode / spaceball mode */
	if ( sbv_eyemode ) {
	    sbv_eyemode = 0;
	    sbx_beep( 1 );
	    sbv_status_update = 1;
	} else {
	    sbv_eyemode = 1;
	    sbx_beep( 2 );
	    sbv_status_update = 1;
	}
    } else {
    }
}



int	sbx_k5( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    if ( val ) {
        sbv_adjust_scale *= 1.2;    /* Initialize it to 0 */
        sbv_stscale = 1;            /* Scale factor has been used */
	sbv_status_update = 1;
	sbx_beep( 5 );		    /* One short beep */
    } else {
    }
 
}



int	sbx_k6( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    if ( val ) {
        sbv_adjust_scale /= 1.2;    /* Initialize it to 0 */
        sbv_stscale = 1;            /* Scale factor has been used */
	sbv_status_update = 1;
	sbx_beep( 5 );		    /* One short beep */
    } else {
    }
}



int	sbx_k7( dev, val, arg )
short   dev;
short   val;
int     arg;
{
}



int	sbx_k8( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    if ( val ) {
	sbrezero();
	sbx_beep( 1 );
    } else {
    }
}



int	sbx_pick( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    if ( val ) {
	sbx_beep( 1 );
	spchome();	/* User supplied routine to reset view*/
    } else {
    }
}



int	sbx_inputchange( dev, val, arg )
short   dev;
short   val;
int     arg;
{
    if ( val ) {
	/* Whenever the window comes into view - prompt spaceball */
	/* This avoids a 1.5 sec delay				  */
	sbprompt();
    } else {
    }
}


char * sbx_other()
{
    static	char buffer[ 30 ];

    if ( sbv_stscale ) {
	sprintf( buffer, " Sensitivity[%7.3f]", sbv_adjust_scale);
	return buffer;
    } else {
	return "";
    }
}

/* Set up a table of control modes */
char	* sbx_mode[] = {
    "On",
    "Off",
    "Dominant"
};

/* ======================== sbSchng ============================== */
/* Return true if there has been a status line change
 * Call sbStatus to reset the flag.
 */

sbSchng()
{
    return sbv_status_update;
}

/* ======================== sbStatus ============================= */
/* Return a pointer to a string with a meaningfull status line 
 */
char	* sbStatus()
{
    static char		buffer[ 100 ];

    if ( ! sbv_open ) {
	sbv_status_update = 0;
	return "";
    }

    /* Check to see if we need to update the status */
    if ( sbv_status_update ) {
	sprintf( buffer,
	    "Spaceball Status : Translations[%s] Rotations[%s] Mode[%s]%s",
	    sbx_mode[ sbv_tranmode ],
	    sbx_mode[ sbv_rotmode ],
	    ( sbv_eyemode ? "Eyepoint" : "Object" ),
	    sbx_other()
	);
    }

    return buffer;
}


/* Set up a beeping table */

char	* sbv_beep[] = {
    "Cc",
    "Cc",
    "CcCc",
    "CcCcCc",
    "AaAaAa",
    "Aa"
};

sbx_beep( n )
int	n;
{
    sbbeep( sbv_beep[ n ] );
}

/* ======================== sbPrmpt ============================= */
/* Prompt spaceball if required.
 * This should be called after ( or before ) every screen update.
 */
sbPrmpt()
{
    if ( sbv_open && sbv_prompt ) {
	sbv_prompt = 0;
	sbprompt();
    }
}

#endif spaceball
