/* Minimal platform bits for mscc-init (Linux). */
#pragma once

#if defined(__linux__) || defined(__APPLE__)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <portaudio.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_PATH PATH_MAX

void Sleep(unsigned long ms);
int MessageBoxA(void *hwnd, const char *text, const char *caption, unsigned int type);
#define MB_OK 0
#define MB_ICONEXCLAMATION 0

#else
#error "mscc-init-linux is for Linux builds only; use the Windows tree for MSVC"
#endif
