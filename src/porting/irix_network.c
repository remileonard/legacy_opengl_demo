/*
 * irix_network.c - Network compatibility layer implementation
 */

#include "irix_network.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32

/* Initialize Windows Sockets */
int iris2ogl_init_network_layer(void)
{
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", result);
        return -1;
    }
    return 0;
}

/* Cleanup Windows Sockets */
void iris2ogl_cleanup_network(void)
{
    WSACleanup();
}

#endif

/* Get host internet address - portable version */
int gethostaddr(struct sockaddr_in *addr)
{
    char hostname[256];
    struct hostent *hp;

    if (gethostname(hostname, sizeof(hostname)) < 0)
        return -1;
    
    hp = gethostbyname(hostname);
    if (!hp)
        return -1;
    
    addr->sin_family = AF_INET;
    addr->sin_port = 0;
    
#ifdef _WIN32
    memcpy(&addr->sin_addr, hp->h_addr, hp->h_length);
#else
    memcpy(&addr->sin_addr, hp->h_addr, hp->h_length);
#endif
    
    return 0;
}
