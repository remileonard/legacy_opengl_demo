/*
 * irix_network.h - Network compatibility layer for IRIX to Windows/Unix portability
 * Provides portable socket API abstraction for UDP broadcast/multicast networking
 */

#ifndef IRIX_NETWORK_H
#define IRIX_NETWORK_H

#ifdef _WIN32
    /* Windows networking */
    #include <winsock2.h>
    #include <ws2tcpip.h>
    
    /* Windows uses int for socket handles but defines SOCKET */
    #ifndef socklen_t
    typedef int socklen_t;
    #endif
    
    /* Windows uses closesocket instead of close */
    #define close_socket(s) closesocket(s)
    
    /* Windows uses ioctlsocket instead of ioctl */
    #define ioctl_socket(s, cmd, arg) ioctlsocket(s, cmd, arg)
    
    /* Windows error codes are different */
    #define SOCKET_ERRNO WSAGetLastError()
    #define EWOULDBLOCK_SOCKET WSAEWOULDBLOCK
    
    /* Windows doesn't define caddr_t */
    #ifndef caddr_t
    typedef char* caddr_t;
    #endif
    
    /* Initialize/cleanup Windows sockets */
    int init_network_layer(void);
    void cleanup_network(void);
    
#else
    /* Unix/Linux networking */
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    
    /* Unix uses close for sockets */
    #define close_socket(s) close(s)
    
    /* Unix uses ioctl directly */
    #define ioctl_socket(s, cmd, arg) ioctl(s, cmd, arg)
    
    /* Unix uses errno directly */
    #define SOCKET_ERRNO errno
    #define EWOULDBLOCK_SOCKET EWOULDBLOCK
    
    /* No need for init/cleanup on Unix */
    static inline int init_network(void) { return 0; }
    static inline void cleanup_network(void) { }
    
#endif

/* Common network utilities */
int gethostaddr(struct sockaddr_in *addr);

#endif /* IRIX_NETWORK_H */