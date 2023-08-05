#pragma once

// Windows
#ifdef _WIN32

#define NOMINMAX

# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif /* WIN32_LEAN_AND_MEAN */
# ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#  define _WINSOCK_DEPRECATED_NO_WARNINGS
# endif /* _WINSOCK_DEPRECATED_NO_WARNINGS */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>

#endif 

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

#include "parser.hpp"
#include "Synchronization.hpp"

#include "units.hpp"
#include "stats.hpp"

using namespace Orpy;

const std::string VERSION = "pre-release 0.0.1";

#ifdef _WIN32
typedef SOCKET socket_t;
/* POSIX ssize_t is not a thing on Windows */
typedef signed long long int ssize_t;
#else
typedef int socket_t;
// winsock has INVALID_SOCKET which is returned by socket(),
// this is the POSIX replacement
# define INVALID_SOCKET -1
#endif 

#define RECVBUF  8192

#define MAX_THREAD_RATE_S   10000000
#define SOCKET_TIMEOUT_MS   2000
#define RECORD_INTERVAL_MS  100

enum phases
{
    CONNECT,
    WRITE,
    READ    
};

struct config
{
    uint64_t connections = 10;
    uint64_t duration = 10;
    uint64_t threads = 1;
    uint64_t timeout = SOCKET_TIMEOUT_MS;
    uint64_t pipeline = 0;
    bool     delay = false;
    bool     dynamic = false;
    bool     latency = false;

    ParsedURL url;

    //char* script;
    //SSL_CTX* ctx;
};

struct statistics
{
    std::unique_ptr<stats> latency = std::make_unique<stats>();
    std::unique_ptr<stats> requests = std::make_unique<stats>();
};

struct buffer
{
    std::vector<char> data;
    size_t length;
    int cursor;
};

struct connection
{
    ~connection()
    {
        if(fd)
        {
            #ifdef _WIN32		
                shutdown(fd, SD_BOTH);
                closesocket(fd);
                WSACleanup();
            #else
                close(fd);
            #endif
        }
    }

    enum state
    {
        FIELD, VALUE
    };
    int fd = -1;
    //SSL* ssl;
    phases phase = CONNECT;
    bool delayed = false;
    std::chrono::high_resolution_clock::time_point start;
    std::string request = "";
    size_t length = 0;
    size_t written = 0;
    uint64_t pending = 0;
    buffer headers;
    buffer body;
    std::vector<char> data;
};

struct threadData
{
    config cfg;
    std::unique_ptr<ThreadPool<connection>> queue;
    struct addrinfo* addr;
    uint64_t connections;
    uint64_t complete;
    uint64_t requests;
    uint64_t bytes;
    uint64_t sent;
    std::chrono::high_resolution_clock::time_point start;
    errorsData errors;
};