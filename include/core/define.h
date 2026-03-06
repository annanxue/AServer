#ifndef ASERVER_DEFINE_H
#define ASERVER_DEFINE_H

#include "version.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define ASERVER_THREAD_CALLBACK DWORD WINAPI
    #define ASERVER_THREAD_RETURN DWORD
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <pthread.h>
    #define ASERVER_THREAD_CALLBACK void*
    #define ASERVER_THREAD_RETURN void*
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

#ifdef _WIN32
    using socket_t = SOCKET;
#else
    using socket_t = int;
#endif

namespace AServer {

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

}

#define ASERVER_NEW new(std::nothrow)

#ifdef _WIN32
    #define SAFE_DELETE(p) do { if (p) { delete (p); (p) = nullptr; } } while(0)
    #define SAFE_DELETE_ARRAY(p) do { if (p) { delete[] (p); (p) = nullptr; } } while(0)
#else
    #define SAFE_DELETE(p) do { if (p) { delete (p); (p) = nullptr; } } while(0)
    #define SAFE_DELETE_ARRAY(p) do { if (p) { delete[] (p); (p) = nullptr; } } while(0)
#endif

#define ASERVER_SAFE_FREE(p) do { if (p) { free(p); (p) = nullptr; } } while(0)

#endif
