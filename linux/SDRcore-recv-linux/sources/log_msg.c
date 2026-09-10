/*
 * Optional log helper for future use — NOT wired into the build or call sites yet.
 * See log_msg.h for integration notes and format.
 */
#include "log_msg.h"

#include <stdarg.h>
#include <stdio.h>

#include "extern.h"

void log_msg(const char *fmt, ...)
{
    va_list ap;

    if (G_fp_logfile == NULL || fmt == NULL)
        return;

    print_time();
    fprintf(G_fp_logfile, "[%d] ", line_number++);

    va_start(ap, fmt);
    vfprintf(G_fp_logfile, fmt, ap);
    va_end(ap);

    fflush(G_fp_logfile);
}
