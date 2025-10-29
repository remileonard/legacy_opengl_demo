// Créer un fichier irix_network.h

#ifndef IRIX_NETWORK_H
#define IRIX_NETWORK_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    
    // Initialisation Winsock pour Windows
    static inline int network_init(void) {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    
    static inline void network_cleanup(void) {
        WSACleanup();
    }
    
    #define close_socket(s) closesocket(s)
    
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    
    static inline int network_init(void) { return 0; }
    static inline void network_cleanup(void) { }
    
    #define close_socket(s) close(s)
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

// Fonctions réseau portables
static inline SOCKET create_tcp_socket(void) {
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

static inline SOCKET create_udp_socket(void) {
    return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

#endif // IRIX_NETWORK_H