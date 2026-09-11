/*
 * Linux/WSL implementations of Windows-ish helpers used by ms-sdr.
 * Serial: real fd open + optional PTY for Kenwood CAT (digital apps).
 */
#if defined(__linux__) || defined(__APPLE__)

#define _GNU_SOURCE
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pty.h>

/* Read timeout for "serial" fd (ms), set via SetCommTimeouts */
static unsigned long g_read_timeout_ms = 20;

/*
 * When we create a PTY for Kenwood CAT we must keep the slave fd open in
 * this process. If the slave is fully closed, master read() returns EIO
 * and the CAT thread dies before any digital app attaches.
 */
static int g_pty_slave_fd = -1;
static int g_pty_master_fd = -1;

void Sleep(unsigned long ms)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000UL);
    ts.tv_nsec = (long)((ms % 1000UL) * 1000000UL);
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
        /* retry remaining */
    }
}

/*
 * name:
 *   "PTY" or "COM0"  → create PTY; symlink $HOME/ms-sdr-cat → slave
 *   "/dev/..." or absolute path → open that device
 *   "\\\\.\\COMx" or "COMx" → try /dev/ttyUSBx (best-effort)
 */
HANDLE CreateFileA(const char *name, unsigned long access, unsigned long share,
                   void *sec, unsigned long creat, unsigned long flags, HANDLE tmp)
{
    int fd = -1;
    char path[512];
    (void)access; (void)share; (void)sec; (void)creat; (void)flags; (void)tmp;

    if (name == NULL) {
        errno = EINVAL;
        return INVALID_HANDLE_VALUE;
    }

    /* Strip Windows \\.\ prefix if present */
    if (strncmp(name, "\\\\.\\", 4) == 0)
        name += 4;

    if (strcmp(name, "PTY") == 0 || strcmp(name, "COM0") == 0 ||
        strcmp(name, "pty") == 0 || strcmp(name, "cat") == 0) {
        int master = -1, slave = -1;
        char slave_name[128];
        const char *home;
        char linkpath[512];
        struct termios tio;

        /* Replace previous PTY if reopening */
        if (g_pty_slave_fd >= 0) {
            close(g_pty_slave_fd);
            g_pty_slave_fd = -1;
        }
        if (g_pty_master_fd >= 0) {
            close(g_pty_master_fd);
            g_pty_master_fd = -1;
        }

        if (openpty(&master, &slave, slave_name, NULL, NULL) < 0) {
            return INVALID_HANDLE_VALUE;
        }

        /*
         * Keep slave open in-process so master does not get EIO while no
         * digital app is attached. Apps open the same slave via symlink
         * (multiple opens are fine on the pts slave).
         */
        g_pty_slave_fd = slave;
        g_pty_master_fd = master;

        /* Raw 8N1-ish on both ends for Kenwood text CAT */
        if (tcgetattr(master, &tio) == 0) {
            cfmakeraw(&tio);
            tio.c_cflag |= (CLOCAL | CREAD);
            tcsetattr(master, TCSANOW, &tio);
        }
        if (tcgetattr(slave, &tio) == 0) {
            cfmakeraw(&tio);
            tio.c_cflag |= (CLOCAL | CREAD);
            tcsetattr(slave, TCSANOW, &tio);
        }

        home = getenv("MS_SDR_HOME");
        if (home == NULL || home[0] == '\0')
            home = getenv("HOME");
        if (home == NULL)
            home = "/tmp";
        snprintf(linkpath, sizeof(linkpath), "%s/ms-sdr-cat", home);
        unlink(linkpath);
        if (symlink(slave_name, linkpath) != 0) {
            fprintf(stderr, "[ms-sdr] PTY slave %s (symlink %s failed: %s)\n",
                slave_name, linkpath, strerror(errno));
        } else {
            fprintf(stderr, "[ms-sdr] Kenwood CAT PTY ready: %s -> %s\n",
                linkpath, slave_name);
            fprintf(stderr, "[ms-sdr] Point digital apps at: %s\n", linkpath);
        }
        /* Master: non-blocking + select in ReadFile */
        fcntl(master, F_SETFL, fcntl(master, F_GETFL) | O_NONBLOCK);
        return (HANDLE)master;
    }

    if (name[0] == '/') {
        strncpy(path, name, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    } else if (strncmp(name, "COM", 3) == 0 || strncmp(name, "com", 3) == 0) {
        /* Best-effort: COM5 → /dev/ttyUSB0 is wrong; prefer explicit /dev path in ini */
        snprintf(path, sizeof(path), "/dev/%s", name);
    } else {
        strncpy(path, name, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    }

    fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
        return INVALID_HANDLE_VALUE;
    return (HANDLE)fd;
}

int CloseHandle(HANDLE h)
{
    if (h < 0)
        return 1;
    if (h == g_pty_master_fd) {
        close(h);
        g_pty_master_fd = -1;
        if (g_pty_slave_fd >= 0) {
            close(g_pty_slave_fd);
            g_pty_slave_fd = -1;
        }
        return 1;
    }
    close(h);
    return 1;
}

int ReadFile(HANDLE h, void *buf, DWORD n, DWORD *got, void *ov)
{
    fd_set rfds;
    struct timeval tv;
    ssize_t r;
    (void)ov;

    if (got) *got = 0;
    if (h < 0 || buf == NULL)
        return 0;

    FD_ZERO(&rfds);
    FD_SET(h, &rfds);
    tv.tv_sec = (time_t)(g_read_timeout_ms / 1000UL);
    tv.tv_usec = (suseconds_t)((g_read_timeout_ms % 1000UL) * 1000UL);
    if (select(h + 1, &rfds, NULL, NULL, &tv) <= 0) {
        /* timeout = success with 0 bytes (Windows serial style) */
        return 1;
    }
    r = read(h, buf, (size_t)n);
    if (r < 0) {
        /* Transient / no peer — look like Windows serial timeout (0 bytes) */
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ||
            errno == EIO) {
            if (got) *got = 0;
            return 1;
        }
        return 0;
    }
    if (r == 0) {
        /* EOF / hangup — stay open so app can reconnect */
        if (got) *got = 0;
        return 1;
    }
    if (got) *got = (DWORD)r;
    return 1;
}

int WriteFile(HANDLE h, const void *buf, DWORD n, DWORD *wrote, void *ov)
{
    ssize_t r;
    (void)ov;
    if (wrote) *wrote = 0;
    if (h < 0 || buf == NULL)
        return 0;
    r = write(h, buf, (size_t)n);
    if (r < 0)
        return 0;
    if (wrote) *wrote = (DWORD)r;
    return 1;
}

int GetCommState(HANDLE h, DCB *dcb)
{
    struct termios tio;
    if (h < 0 || dcb == NULL)
        return 0;
    if (tcgetattr(h, &tio) != 0)
        return 0;
    dcb->DCBlength = sizeof(*dcb);
    dcb->BaudRate = 9600;
    dcb->ByteSize = 8;
    dcb->StopBits = ONESTOPBIT;
    dcb->Parity = PARITY_NONE;
    return 1;
}

int SetCommState(HANDLE h, DCB *dcb)
{
    struct termios tio;
    speed_t speed = B9600;
    if (h < 0 || dcb == NULL)
        return 0;
    if (tcgetattr(h, &tio) != 0) {
        /* PTY may still accept cfset */
        memset(&tio, 0, sizeof(tio));
        cfmakeraw(&tio);
    }
    switch (dcb->BaudRate) {
    case 1200: speed = B1200; break;
    case 2400: speed = B2400; break;
    case 4800: speed = B4800; break;
    case 9600: speed = B9600; break;
    case 19200: speed = B19200; break;
    case 38400: speed = B38400; break;
    case 57600: speed = B57600; break;
    case 115200: speed = B115200; break;
    default: speed = B9600; break;
    }
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);
    cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);
    if (tcsetattr(h, TCSANOW, &tio) != 0)
        return 0;
    return 1;
}

int SetCommTimeouts(HANDLE h, COMMTIMEOUTS *t)
{
    (void)h;
    if (t != NULL) {
        g_read_timeout_ms = t->ReadIntervalTimeout;
        if (g_read_timeout_ms == 0)
            g_read_timeout_ms = 20;
        if (g_read_timeout_ms > 5000)
            g_read_timeout_ms = 5000;
    }
    return 1;
}

/*int GetCommModemStatus(HANDLE h, void *stat)
{
    (void)h;
    if (stat) *(DWORD *)stat = 0;
    return 1;
}
*/

int GetCommModemStatus(HANDLE h, void *stat)
{
    DWORD *lpModemStat = (DWORD *)stat;
    int mstat = 0;

    if (h < 0 || lpModemStat == NULL) {
        if (lpModemStat) *lpModemStat = 0;
        return 0;
    }

    /* PTY case: no real modem lines */
    if (h == g_pty_master_fd) {
        *lpModemStat = 0;
        return 1;
    }

    if (ioctl(h, TIOCMGET, &mstat) < 0) {
        perror("ioctl(TIOCMGET) failed");   // helpful for debugging
        *lpModemStat = 0;
        return 0;
    }

    *lpModemStat = 0;
    if (mstat & TIOCM_CTS) *lpModemStat |= MS_CTS_ON;
    if (mstat & TIOCM_DSR) *lpModemStat |= MS_DSR_ON;
    if (mstat & TIOCM_RI)  *lpModemStat |= MS_RING_ON;
    if (mstat & TIOCM_CAR) *lpModemStat |= MS_RLSD_ON;

    return 1;
}

unsigned long GetLastError(void)
{
    return (unsigned long)errno;
}

unsigned long FormatMessageA(unsigned long flags, const void *src, unsigned long msg,
                             unsigned long lang, char *buf, unsigned long size, void *args)
{
    (void)flags; (void)src; (void)lang; (void)args;
    if (!buf || size == 0)
        return 0;
    snprintf(buf, size, "%s", strerror((int)msg));
    return (unsigned long)strlen(buf);
}

int MessageBoxA(void *hwnd, const char *text, const char *caption, unsigned int type)
{
    (void)hwnd; (void)type;
    fprintf(stderr, "[MessageBox] %s: %s\n", caption ? caption : "", text ? text : "");
    return 1;
}

#endif /* __linux__ */
