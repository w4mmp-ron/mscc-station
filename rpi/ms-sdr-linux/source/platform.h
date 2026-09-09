/*
 * Platform abstraction for ms-sdr (Windows vs Linux/WSL).
 * Include this before (or instead of) Windows-only headers.
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

/* Match Windows-ish types used throughout ms-sdr */
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

/* Winsock stubs */
typedef struct { int dummy; } WSADATA;
#define MAKEWORD(a,b) 0
static inline int WSAStartup(int v, WSADATA *w) { (void)v; (void)w; return 0; }
static inline int WSACleanup(void) { return 0; }
static inline int WSAGetLastError(void) { return errno; }
#define closesocket close

/* Sleep(ms) — Windows Sleep is milliseconds */
void Sleep(unsigned long ms);

/* Serial / Win32 API mapped in platform_linux.c (real fd + PTY for Kenwood CAT) */
typedef int HANDLE;
#define INVALID_HANDLE_VALUE (-1)
#define GENERIC_READ  0x80000000
#define GENERIC_WRITE 0x40000000
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_NORMAL 0x80
#define ERROR_FILE_NOT_FOUND 2

typedef struct {
    unsigned long DCBlength;
    unsigned long BaudRate;
    unsigned char ByteSize;
    unsigned char StopBits;
    unsigned char Parity;
} DCB;

typedef struct {
    unsigned long ReadIntervalTimeout;
    unsigned long ReadTotalTimeoutMultiplier;
    unsigned long ReadTotalTimeoutConstant;
    unsigned long WriteTotalTimeoutMultiplier;
    unsigned long WriteTotalTimeoutConstant;
} COMMTIMEOUTS;

typedef unsigned long DWORD;
typedef char *LPTSTR;
typedef unsigned long *LPDWORD;

/* Win32 serial parity / stop (values unused on Linux stubs) */
#define PARITY_NONE 0
#define PARITY_ODD  1
#define PARITY_EVEN 2
#define ONESTOPBIT  0
#define TWOSTOPBITS 2
#define MS_CTS_ON   0x0010
#define MS_DSR_ON   0x0020
#define MS_RING_ON  0x0040
#define MS_RLSD_ON  0x0080

HANDLE CreateFileA(const char *name, unsigned long access, unsigned long share,
                   void *sec, unsigned long creat, unsigned long flags, HANDLE tmp);
int CloseHandle(HANDLE h);
int ReadFile(HANDLE h, void *buf, DWORD n, DWORD *got, void *ov);
int WriteFile(HANDLE h, const void *buf, DWORD n, DWORD *wrote, void *ov);
int GetCommState(HANDLE h, DCB *dcb);
int SetCommState(HANDLE h, DCB *dcb);
int SetCommTimeouts(HANDLE h, COMMTIMEOUTS *t);
int GetCommModemStatus(HANDLE h, void *stat);
unsigned long GetLastError(void);
unsigned long FormatMessageA(unsigned long flags, const void *src, unsigned long msg,
                             unsigned long lang, char *buf, unsigned long size, void *args);
int MessageBoxA(void *hwnd, const char *text, const char *caption, unsigned int type);
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
#define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200
#define MAKELANGID(p,s) 0
#define LANG_NEUTRAL 0
#define SUBLANG_DEFAULT 0
#define MB_OK 0
#define MB_TASKMODAL 0
#define MB_ICONEXCLAMATION 0

#else /* Windows */

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#endif /* __linux__ */
