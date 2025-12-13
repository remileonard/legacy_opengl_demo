#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef _WIN32
	#include <sys/socket.h>
	#include <net/if.h>
	#include <sys/ioctl.h>
#endif
#include "porting/iris2ogl.h"
#include "porting/irix_network.h"
#include "cycles.h"

/* zip is way cool -> cycle-search == colours all over the place */

#define no_M_DEBUG

/* prototypes for local fns */
int find_others(void);
int setup_mcast_io(char *, u_short , u_short , char *);
int recv_mcast(int , void *, int );
void send_mcast(int , void *, int );
int recv_full(void);

/* 
 * Note: these values are chosen not to collide with known multicast 
 * protocols and ports, but they might be wrong...
 * The group address is chosen so that it won't be forwarded beyond
 * the local (sub)network.
 */

#define DEFAULT_PORT	8222
#define DEFAULT_GROUP	"225.13.13.250"

struct sockaddr_in addr;
int s;                  /* socket for comms */
int full[CYCLES];       /* true if received full info from this id */
clock_t last_time[CYCLES];  /* the time a packet was last recieved from this bike */
static char g_group[32], g_interface[32];	/* multicast options... */
static u_short g_ttl, g_port;

extern CYCLE *good, bike[CYCLES];
extern int used[CYCLES], robot[CYCLES];

void parse_args(int argc, char **argv) {
    int c, errflag = 0;
    extern char *optarg;
    extern int optind, opterr;

    /* set default communication parameters */
    sprintf(g_group, "%s", DEFAULT_GROUP);
    sprintf(g_interface, "%s", "");
    g_ttl = 1;
    g_port = DEFAULT_PORT;

    /* parse command line args */
    while ((c = getopt(argc, argv, "i:t:")) != -1)
	switch(c) {
	case 'i':
	    sprintf(g_interface, "%s", optarg);
	    break;
	case 't':
	    g_ttl = (u_short)atoi(optarg);
	    break;
        default:
            errflag++;
            break;
	}
    if (errflag) {
	printf("usage: %s [ -i interface ] [ -t ttl ]\n", argv[0]);
	exit(1);
    }
}

/*
 * open a multicast socket
 */
void init_comms(void) {
    s = setup_mcast_io(g_group, g_port, g_ttl, g_interface);
}


/*
 * loop waiting for big packets from all players
 */
int wait_for_replies(void) {
    int new_user;

    /* loop here waiting 'til we have "full" from all */
#ifdef M_DEBUG
printf("recving full's...\n");
#endif
    new_user = recv_full();

#ifdef M_DEBUG
printf("go play...\n");
#endif
    /* our work here is done - go play games */
    return(new_user);
}


/*
 * read the network for a second or so and build up a list of players
 * set our id to be a free slot if there is one, otherwise exit
 * initialise our position data
 */
void init_network(int *num_robots) {
    int id, i, cnt;

    if ((id = find_others()) == -1) {
	printf("sorry - game is full\n");
	exit(0);
    }
#ifdef M_DEBUG
printf("our id is %d\n", id);
printf("init_net: used: ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
#endif

    /* ok, we're in the game... */
    good = &bike[id];
    init_pos(good);
    good->id = id;
    good->type = PERSON;
    used[id] = 1;
    full[id] = 1;

    /* and so are our robots */
    cnt = 0;
    for (i = 0; i < CYCLES; i++) {
	if (robot[i])
	    if (cnt < *num_robots) {
		cnt++;
		init_pos(&bike[i]);
		bike[i].id = i;
		used[i] = 1;
		full[i] = 1;
	    }
	    else
		robot[i] = 0;
    }
}


/*
 * loop waiting for a "full" packet from all
 */
int recv_full() {
    int i;
    struct tms t;
    clock_t clicks;
    int new_user = 0;

#ifdef M_DEBUG
printf("TOP\n");
printf("recv full: used ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
printf("recv full: full ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", full[i]); }
printf("\n");
#endif

    /* loop again waiting for info from all other players */
    clicks = times(&t) + 1.5*HZ;	    /* look for 1.5 seconds */
    for (i = 0; times(&t) < clicks; i++)
	if (get_and_sort_mcasts()) new_user = 1;

#ifdef M_DEBUG
printf("AFTER GET AND SORT\n");
printf("recv full: new_user found? %d\n",  new_user);
printf("recv full: used ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
printf("recv full: full ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", full[i]); }
printf("\n");
#endif

    /*
     * if after this we haven't got "full"s from all "used" by this
     * time then flag the "used"s as dead 'cos they're too slow
     */
    for (i = 0; i < CYCLES; i++)
	if (i != good->id && used[i] && !full[i] && !robot[i])
	    used[i] = 0;

#ifdef M_DEBUG
printf("AFTER ELIMATED no_returns\n");
printf("recv full: used ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
printf("recv full: full ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", full[i]); }
printf("\n");
#endif

    return(new_user);
}


/*
 * see who else is out there. returns a free id #, or -1 if game full
 * return as many robot ids as possible,  but don't worry too much
 * if we can't do them all...
 */
int find_others(void) {
    int i, id;
    struct tms t;
    clock_t clicks;
    CYCLE tmp;

    /* zero the used, robot structs */
    for (i = 0; i < CYCLES; i++) {
	used[i] = 0;
	full[i] = 0;
	robot[i] = 0;
    }

    /* loop waiting for info from all other players */
    clicks = times(&t) + 1.5*HZ;	    /* look for 1.5 second */
    for (i = 0; times(&t) < clicks; i++)
	if (recv_mcast(s, &tmp, sizeof(tmp)) != -1) used[tmp.id] = 1;   	

#ifdef M_DEBUG
printf("find: used: ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
#endif

    id = -1;
    /* return unused slots, if any */
    for (i = 0; i < CYCLES; i++) {
	if (!used[i])
	    if (id == -1)   /* allocate our id first */
		id = i;
	    else
		robot[i] = 1;
    }

    return(id);
}


void send_mcast(int fd, void *message, int size) {
    int cnt;

    cnt = sendto(fd, message, size, 0, (const struct sockaddr*)&addr, sizeof(addr));
#ifdef M_DEBUG
if (cnt > 100)
 printf("sent mcast: %d id %d aliv? %d\n", cnt, ((CYCLE *)message)->id, ((CYCLE *)message)->alive);
#endif
    if (cnt < 0) {
	perror("sendto");
	exit(1);
    }
}


void send_full_mcast(CYCLE *C) {
#ifdef M_DEBUG
printf("send: full ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", full[i]); }
printf("\n");
printf("send: used ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
#endif
    send_mcast(s, C, FULL_PACKET);
}


void send_all_full_mcast() {
    int i;

#ifdef M_DEBUG
printf("send: full ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", full[i]); }
printf("\n");
printf("send: used ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
#endif
    /* send us */
    send_mcast(s, good, FULL_PACKET);

    /* and our robots */
    for (i = 0; i < CYCLES; i++)
	if (robot[i])
	    send_mcast(s, &bike[i], FULL_PACKET);
}


void send_update_mcast() {
    int i;

    /* send us */
    send_mcast(s, good, SHORT_PACKET);

    /* and our robots */
    for (i = 0; i < CYCLES; i++)
	if (robot[i])
	    send_mcast(s, &bike[i], SHORT_PACKET);
}


int recv_mcast(int fd, void *message, int size) {
    int cnt;
    struct sockaddr_in fromaddr;
    int fromaddrlen = sizeof(fromaddr);

    cnt = recvfrom(fd, message, size, 0, (struct sockaddr*)&fromaddr, &fromaddrlen);
#ifdef M_DEBUG
if (cnt > 100)
 printf("recv'd mcast: %d id %d alive? %d\n", cnt, ((CYCLE *)message)->id, ((CYCLE *)message)->alive);
#endif
    if (cnt < 0 && errno != EWOULDBLOCK) {
	perror("recvfrom");
	exit(1);
    }
    return(cnt);
}


/*
 * check if cycle hasn't been heard from for a second or so and
 * kill it off if it hasn't
 * NOTE: the timeout muct be greater than the startup sequence waits
 */
void kill_dead_cycle(void) {
    int i;
    struct tms t;

    for (i = 0; i < CYCLES; i++)
	if (used[i] && i != good->id && !robot[i] && last_time[i] + NET_TIMEOUT*HZ < times(&t)) {
	    bike[i].quit = 1;
	    used[i] = 0;
	    full[i] = 0;
	    printf("cycles: killed \"%s\" id %d: lost contact for %g seconds\n", bike[i].name, bike[i].id, (float)NET_TIMEOUT);
	}
}


/*
 * receive multicast packet handler
 *  - gets data from network
 *  - packs data into cycle structs
 *  - handles both long and short (update) packets
 *  - return 1 if new user has arrived, 0 otherwise
 */
int get_and_sort_mcasts(void) {
    CYCLE tmp;
    int i, len, id, new_user;
    struct tms t;

    new_user = 0;

    while((len = recv_mcast(s, &tmp, sizeof(tmp))) != -1) {
	id = tmp.id;

	/* filter out packets from ourself */
	if (id == good->id) {
	    printf("error: packet from ourself %d  ... committing suicide\n", len);
	    good->quit = 1;
	    new_user = 1;
	}

	/* and from our robots */
	for (i = 0; i < CYCLES; i++) {
	    if (robot[i] && i == id) {
		/* drop packets from ourself */
		printf("error: packet from our robots %d  ... downer\n", len);
		bike[i].alive = 0;
		new_user = 1;
		continue;
	    }
	}

	/* remember the time this packet arrived */
	last_time[id] = times(&t);

	/*
	 * if new, _and_ have got a full packet, then add to
	 * used list and flag new_user
	 */
	used[id] = 1;
	if (!full[id] && tmp.alive) new_user = 1;

	/* if someone is quiting remove player from list */
	if (tmp.quit) {
	    used[id] = 0;
	    full[id] = 0;
	    bzero(&bike[id], sizeof(CYCLE));
	}

	/*
	 * if short packet then check that have previously
	 * received full data and if so, update cycle struct
	 */
	if (len == SHORT_PACKET) {
	    if (full[id]) {
		int tp;

		/* update cycle data and trail */
		bcopy((void *)&tmp, (void *)&bike[id], (int)SHORT_PACKET);
		tp = bike[id].trail_ptr;
		bcopy((void *)&bike[id].trail_update, (void *)&bike[id].trail[tp], (int)sizeof(CPOINT));
	    }
	}
	/*
	 * else if long packet then copy over into cycle struct and
	 * flag full received from this id
	 */
	else if (len == FULL_PACKET) {
	    bcopy((void *)&tmp, (void *)&bike[id], (int)FULL_PACKET);
	    full[id] = 1;
#ifdef M_DEBUG
printf("get_sort BIG: full ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", full[i]); }
printf("\n");
printf("get_sort BIG: used ");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
#endif
	}
	else {
	    printf("cycles: get_and_sort_mcasts: weird packet length %d - wrong version bucko??\n", len);
	}

	/* if dead then set full to 0 */
	if (!tmp.alive) full[id] = 0;

    }

    return(new_user);
}


/*
 * setup normal mcast routing - default calling is
 *  - mostly taken from 4Dgifts mcast.c
 */
int setup_mcast_io(char *group, u_short port, u_short ttl, char *interface) {
    int			fd, i, on;
    struct ip_mreq	mreq;
    u_char		loop = 0;
    struct in_addr	ifaddr;
    struct in_addr	grpaddr;
    u_char		t_ttl;

    t_ttl = ttl;    /* dunno why need this... */

#if 0
printf("t %d p %d i %s g %s\n", ttl, port, interface, group);
#endif

    on = 1;
    grpaddr.s_addr = inet_addr(group);

    if (!IN_MULTICAST(grpaddr.s_addr)) {
	fprintf(stderr, "Invalid multicast group address: %s\n", group);
	exit(1);
    }

    if (port != DEFAULT_PORT)
	fprintf(stderr, "Warning: port %d may be in use, check /etc/services for defined ports\n", (int)port);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    ifaddr.s_addr = htonl(INADDR_ANY);

    if (strcmp(interface, "")) {
	/*
	 * Make sure the specified interface exists and is capable of doing
	 * multicasting.
	 */

#ifdef _WIN32
	/* Sur Windows, l'enum├®ration des interfaces est diff├®rente */
	/* Pour l'instant, on utilise simplement INADDR_ANY */
	fprintf(stderr, "Warning: interface selection not implemented on Windows, using default interface\n");
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&ifaddr, sizeof(ifaddr)) < 0) {
	    perror("setsockopt ifaddr");
	    exit(1);
	}
#else
	struct ifconf ifc;
	struct ifreq *ifr;
	char buf[BUFSIZ];

#ifdef M_DEBUG
printf("interface active %s\n", interface);
#endif

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(fd, (int)SIOCGIFCONF, (char *)&ifc) < 0) {
	    perror("ioctl SIOCGIFCONF");
	    exit(1);
	}

	ifr = ifc.ifc_req;
	for (i = ifc.ifc_len/sizeof(*ifr); --i >= 0; ifr++) {
	    if (ifr->ifr_addr.sa_family != AF_INET)
		continue;
	    if (strncmp(ifr->ifr_name, interface, strlen(ifr->ifr_name)) == 0) {

		/* Obtain the interface's assigned network address */
		ifaddr = ((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr;

		if (ioctl(fd, (int)SIOCGIFFLAGS, (char *) ifr) < 0) {
		    perror("ioctl SIOCGIFFLAGS");
		    exit(1);
		}
		if (!(ifr->ifr_flags & IFF_MULTICAST)) {
		    fprintf(stderr,
			"%s: interface doesn't support multicasting\n",
			interface);
		    exit(1);
		}

		/* Specify the interface to use when sending packets */
		if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr, sizeof(ifaddr)) < 0) {
		    perror("setsockopt ifaddr");
		    exit(1);
		}
		break;
	    }
	}
#endif /* _WIN32 */
	if (ifaddr.s_addr == htonl(INADDR_ANY)) {
	    fprintf(stderr, "%s: invalid or unknown interface\n", interface);
	    exit(1);
	}
    }

    /* disable loop in multicast mode */
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop))) {
	perror("setsockopt loop");
	exit(1);
    }

    /* turn on non-blocking i/o */
    if (ioctl(fd, (int)FIONBIO, &on) < 0) {
	perror("non blocking");
	exit(1);
    }

    /* set time to live */
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&t_ttl, sizeof(t_ttl))) {
	perror("setsockopt ttl");
	exit(1);
    }

#if 0
    /*
     * DEBUG:
     * Allow multiple instances of this program to listen on the same
     * port on the same host. By default, only 1 program can bind
     * to the port on a host.
     */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
	perror("setsockopt REUSEPORT");
	exit(1);
    }
#endif

    if (bind(fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0) {
	perror("bind");
	exit(1);
    }

    mreq.imr_multiaddr = grpaddr;
    mreq.imr_interface = ifaddr;
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq)) < 0) {
	perror("setsockopt mreq");
	exit(1);
    }

    /* can't put this line up above - bind fails. why??? */
    addr.sin_addr = grpaddr;

    return(fd);
}
