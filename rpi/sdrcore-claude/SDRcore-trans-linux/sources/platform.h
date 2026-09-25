/*
 * Platform abstraction for sdrcore-trans (Linux/WSL tree).
 * Windows MSVC build remains in SDRcore-trans (untouched).
 */
#pragma once

#if defined(__linux__) || defined(__APPLE__)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef int BOOL;
typedef unsigned char byte;
typedef unsigned char UCHAR;
typedef unsigned int UINT32;
typedef unsigned long ULONG32;
typedef unsigned long long ULONG64;
typedef signed char INT8;
typedef short INT16;
typedef int INT32;
typedef char TCHAR;
typedef int SOCKET;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define MAX_PATH PATH_MAX
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)
#define NO_ERROR 0

typedef struct { int dummy; } WSADATA;
#define MAKEWORD(a,b) 0
static inline int WSAStartup(int v, WSADATA *w) { (void)v; (void)w; return 0; }
static inline int WSACleanup(void) { return 0; }
static inline int WSAGetLastError(void) { return errno; }
#define closesocket close

void Sleep(unsigned long ms);
int MessageBoxA(void *hwnd, const char *text, const char *caption, unsigned int type);
/* Wide MessageBox not used on Linux — map to MessageBoxA with empty conversion sites */
#define MessageBox(hwnd, text, caption, type) MessageBoxA((hwnd), "MessageBox", (caption), (type))
#define MB_OK 0
#define MB_ICONASTERISK 0
#define MB_ICONEXCLAMATION 0
#define MB_ICONSTOP 0
#define MB_TASKMODAL 0

#include <portaudio.h>

#else /* Windows — only if someone builds this tree on Win */

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <conio.h>
#include <ShlObj.h>
#include <KnownFolders.h>
#include "portaudio.h"

#endif /* __linux__ */
