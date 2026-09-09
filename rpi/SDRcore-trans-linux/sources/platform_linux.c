/*
 * Linux/WSL helpers for sdrcore-trans.
 */
#if defined(__linux__) || defined(__APPLE__)

#include "platform.h"
#include <stdio.h>
#include <errno.h>
#include <time.h>

void Sleep(unsigned long ms)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000UL);
    ts.tv_nsec = (long)((ms % 1000UL) * 1000000UL);
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
    }
}

int MessageBoxA(void *hwnd, const char *text, const char *caption, unsigned int type)
{
    (void)hwnd;
    (void)type;
    fprintf(stderr, "[MessageBox] %s: %s\n",
        caption ? caption : "", text ? text : "");
    return 1;
}

#endif /* __linux__ */
