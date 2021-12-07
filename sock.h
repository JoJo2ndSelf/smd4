/*
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>

#ifndef SMD_API
#define SMD_API
#endif

typedef size_t sock_handle_t;
// Represents an internet address usable by sockets
typedef struct sock_address_t {
  union {
    unsigned int host;
    unsigned char hostbytes[4];
  };
  unsigned short port;
} sock_address_t;

SMD_API int sock_initialize(size_t max_sockets);

SMD_API void sock_shutdown();

SMD_API int sock_get_address(sock_address_t *address, const char *host, unsigned short port);
SMD_API const char *sock_host_to_str(unsigned int host);
SMD_API int sock_get_host_name(char* name, int name_len);

 // Closes a previously opened socket
SMD_API void sock_close(sock_handle_t socket);

SMD_API int sock_open(sock_handle_t* socket, int non_blocking);
// Opens a socket and binds it to a specified port
// (use 0 to select a random open port)
//
// Socket will not block if 'non-blocking' is non-zero
//
// Returns 0 on success
// Returns -1 on failure (call 'zed_net_get_error' for more info)
SMD_API int sock_listen(sock_handle_t* socket, unsigned int port, int non_blocking, int listen);

// Closes a previously opened socket 
SMD_API void sock_close(sock_handle_t socket);

// Connect to a remote endpoint
// Returns 0 on success.
//  if the socket is non-blocking, then this can return 1 if the socket isn't ready
//  returns -1 otherwise. (call 'zed_net_get_error' for more info)
SMD_API int sock_connect(sock_handle_t socket, sock_address_t remote_addr);

// Accept connection
// New remote_socket inherits non-blocking from listening_socket
// Returns 0 on success.
//  if the socket is non-blocking, then this can return 1 if the socket isn't ready
//  if the socket is non_blocking and there was no connection to accept, returns 2
//  returns -1 otherwise. (call 'zed_net_get_error' for more info)
SMD_API int sock_accept(sock_handle_t skt, sock_handle_t *remote_socket, sock_address_t *remote_addr);

// Sends a specific amount of data to 'destination'
//
// Returns 0 on success, -1 otherwise (call 'zed_net_get_error' for more info)
SMD_API int sock_send(sock_handle_t socket, const void *data, int size);

// Receives a specific amount of data from 'sender'
//
// Returns the number of bytes received, -1 otherwise (call 'zed_net_get_error' for more info)
SMD_API int sock_receive(sock_handle_t socket, void *data, int size);

#ifdef SMD_SOCK_IMPL

#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "wsock32.lib")
#else
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

const int INVALID_SOCKET = -1; // or -1
const int SOCKET_ERROR = -1;
#endif

typedef struct sock_t{
  int handle; // -1 when free
  int ready;
  int blocking;
} sock_t;

static sock_t* sockets;
static uint32_t totalSockets;

static sock_handle_t alloc_socket() {
  for (uint32_t i = 1; i < totalSockets; ++i) {
    if (sockets[i].handle == -1) {
      return i;
    }
  }
  return 0;
}

static void free_socket(sock_handle_t hdl) {
  sockets[hdl].handle = -1;
  sockets[hdl].ready = 0;
  sockets[hdl].blocking = 0;
}

// Returns 1 if it would block, <0 if there's an error.
static int would_block(sock_t *socket) {
  struct timeval timer;
  fd_set writefd;
  int retval;

  if (!socket->blocking && !socket->ready) {
    FD_SET(socket->handle, &writefd);
    timer.tv_sec = 0;
    timer.tv_usec = 0;
    retval = select(0, NULL, &writefd, NULL, &timer);
    if (retval == 0)
      return 1;
    else if (retval == SOCKET_ERROR) {
      return -1;//zed_net__error("Got socket error from select()");
    }
    socket->ready = 1;
  }

  return 0;
}

int sock_initialize(size_t max_sockets) {
  sockets = malloc(sizeof(sock_t)*(max_sockets +1));
  totalSockets = max_sockets + 1;
  for (uint32_t i = 0; i < totalSockets; ++i) {
    sockets[i].handle = -1;
  }
#ifdef _WIN32
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
  {
    return -1;
  }

  return 0;
#else
  return 0;
#endif
}

void sock_shutdown() {
  // TODO: actually shutdown sockets;
  free(sockets);
}

void sock_close(sock_handle_t skt) {
  if (!skt) {
    return;
  }
  sock_t* sock = sockets + skt;

  if (sock->handle) {
#ifdef _WIN32
    closesocket(sock->handle);
#else
    close(sock->handle);
#endif
  }
  free_socket(skt);
}

int sock_open(sock_handle_t* skt, int non_blocking) {
  *skt = alloc_socket();
  sock_t* sock = sockets + (*skt);

  // Create the socket
  sock->handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock->handle <= 0) {
    sock_close(*skt);
    //return zed_net__error("Failed to create socket");
    return -1;
  }

  sock->blocking = !non_blocking;

  return 0;
}

int sock_listen(sock_handle_t* skt, unsigned int port, int non_blocking, int listening) {
  *skt = alloc_socket();
  sock_t* sock = sockets+(*skt);

  // Create the socket
  sock->handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock->handle <= 0) {
    sock_close(*skt);
    //return zed_net__error("Failed to create socket");
    return -1;
  }

  // Bind the socket to the port
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(sock->handle, (const struct sockaddr *) &address, sizeof(struct sockaddr_in)) != 0) {
    sock_close(*skt);
    //return zed_net__error("Failed to bind socket");
    return -1;
  }

  // Set the socket to non-blocking if neccessary
  if (non_blocking) {
#ifdef _WIN32
    u_long nblock = non_blocking;
    if (ioctlsocket(sock->handle, FIONBIO, &nblock) != 0) {
      sock_close(*skt);
      //return zed_net__error("Failed to set socket to non-blocking");
      return -1;
    }
#else
    if (fcntl(sock->handle, F_SETFL, O_NONBLOCK, non_blocking) != 0) {
      sock_close(*skt);
      //return zed_net__error("Failed to set socket to non-blocking");
      return -1;
    }
#endif
    sock->ready = 0;
  }

  if (listening) {
#ifndef SOMAXCONN
#define SOMAXCONN 10
#endif
    if (listen(sock->handle, SOMAXCONN) != 0) {
      sock_close(*skt);
      //return zed_net__error("Failed make socket listen");
      return -1;
    }
  }
  sock->blocking = !non_blocking;

  return 0;
}

int sock_connect(sock_handle_t skt, sock_address_t remote_addr) {
  struct sockaddr_in address;
  int retval;

  if (!skt)
    //return zed_net__error("Socket is NULL");
    return -1;

  sock_t* sock = sockets+skt;

  retval = would_block(sock);
  if (retval == 1)
    return 1;
  else if (retval) {
    sock_close(skt);
    return -1;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = remote_addr.host;
  address.sin_port = htons(remote_addr.port);

  retval = connect(sock->handle, (const struct sockaddr *) &address, sizeof(address));
  if (retval == SOCKET_ERROR) {
    sock_close(skt);
    //return zed_net__error("Failed to connect socket");
    return -1;
  }

  return 0;
}

int sock_accept(sock_handle_t skt, sock_handle_t *remote_socket, sock_address_t *remote_addr) {
  struct sockaddr_in address;
  int retval, handle;
  sock_t* listening_socket = sockets+skt;

  if (!skt)
    //return zed_net__error("Listening socket is NULL");
    return -1;
  if (!remote_socket)
    //return zed_net__error("Remote socket is NULL");
    return -1;
  if (!remote_addr)
    //return zed_net__error("Address pointer is NULL");
    return -1;

  retval = would_block(listening_socket);
  if (retval == 1)
    return 1;
  else if (retval) {
    sock_close(skt);
    return -1;
  }
#ifdef _WIN32
  typedef int socklen_t;
#endif
  socklen_t addrlen = sizeof(address);
  handle = accept(listening_socket->handle, (struct sockaddr *)&address, &addrlen);

  if (handle == INVALID_SOCKET)
    return 2;

  remote_addr->host = address.sin_addr.s_addr;
  remote_addr->port = ntohs(address.sin_port);
  *remote_socket = alloc_socket();
  sock_t* rsock = sockets+*remote_socket;
  rsock->blocking = listening_socket->blocking;
  rsock->ready = 0;
  rsock->handle = handle;

  return 0;
}

int sock_send(sock_handle_t skt, const void *data, int size) {
  if (!skt) return -1;
  
  int retval;
  sock_t* sock = sockets+skt;

  retval = would_block(sock);
  if (retval == 1)
    return 1;
  else if (retval) {
    sock_close(skt);
    return -1;
  }

  int sent_bytes = send(sock->handle, (const char *)data, size, 0);
  if (sent_bytes != size) {
    //return zed_net__error("Failed to send data");
    return -1;
  }

  return 0;
}

int sock_receive(sock_handle_t skt, void *data, int size) {
  if (!skt) return -1;

  int retval;
  sock_t* sock = sockets + skt;

  retval = would_block(sock);
  if (retval == 1)
    return 1;
  else if (retval) {
    sock_close(skt);
    return -1;
  }

#ifdef _WIN32
  typedef int socklen_t;
#endif

  int received_bytes = recv(sock->handle, (char *)data, size, 0);
  if (received_bytes <= 0) {
    return 0;
  }
  return received_bytes;
}

int sock_get_address(sock_address_t *address, const char *host, unsigned short port) {
  if (host == NULL) {
    address->host = INADDR_ANY;
  }
  else {
    address->host = inet_addr(host);
    if (address->host == INADDR_NONE) {
      struct hostent *hostent = gethostbyname(host);
      if (hostent) {
        memcpy(&address->host, hostent->h_addr, hostent->h_length);
      }
      else {
        return -1;///zed_net__error("Invalid host name");
      }
    }
  }

  address->port = port;

  return 0;
}

const char *sock_host_to_str(unsigned int host) {
  struct in_addr in;
  in.s_addr = host;

  return inet_ntoa(in);
}

int sock_get_host_name(char* name, int name_len) {
  return gethostname(name, name_len) == 0 ? 0 : -1;
}

#endif

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus