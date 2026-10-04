/*
 * Platform layer for sdrcore-recv (Linux / Pi). Keeps the Windows-style names the
 * code still uses (Sleep, MessageBoxA, SOCKET, WSAStartup, ...) mapped to Linux.
 */
#pragma once

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
#define MB_OK 0
#define MB_ICONASTERISK 0
#define MB_ICONEXCLAMATION 0
#define MB_ICONSTOP 0
#define MB_TASKMODAL 0

/* System PortAudio (mscc-portaudio in /usr/local) */
#include <portaudio.h>
