/*
 * Copyright 1984-1991, 1992, 1993, 1994, Silicon Graphics, Inc.
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

#ifndef UDPBRDCSTDEF
#define UDPBRDCSTDEF

#include "porting/irix_network.h"

#define IGNOREOWNMSG 1 /* Flag for recvbroadcast() to ignore messages */
                       /* which are received on originating host */
#define ACCEPTOWNMSG 0 /* Flag for recvbroadcast() to accept messages */
                       /* which are received on originating host */

extern int getbroadcast(/* char *service, struct sockaddr_in *addr */);

extern int sendbroadcast(/* int socketfiledes, char *message, 
			    int messagelength, struct sockaddr_in *addr */);

extern int recvbroadcast(/* int socketfiledes, char *messagebuf,
			    int messagebuflen, int ignoreownflag */);

extern char multicast;     /* if TRUE use multicast instead of broadcast */
extern char mcast_ttl;     /* packet time-to-live (must be type "char") */
extern char *mcast_ifaddr; /* name of interface to send to/recv from */

#endif /* UDPBRDCSTDEF */
