/*
 * Optional log helper for future use — NOT wired into the build or call sites yet.
 *
 * Replaces the two-call pattern:
 *   print_time();
 *   fprintf(G_fp_logfile, "[%d] message...\n", line_number++, ...);
 *
 * with:
 *   log_msg("message...\n", ...);
 *
 * Same on-disk format:
 *   [dd:hh:mm:ss][line] message...
 *
 * When integrating later:
 *   1. Add sources/log_msg.c to the Makefile SRCS list
 *   2. #include "log_msg.h" where needed
 *   3. Replace print_time()+fprintf pairs gradually
 *   4. Drop [%d] and line_number++ from the format — helper owns them
 */
#ifndef MSCC_LOG_MSG_H
#define MSCC_LOG_MSG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Printf-style log to G_fp_logfile.
 * Writes [dd:hh:mm:ss] then [line_number] then the formatted message.
 * Increments global line_number. No-op if G_fp_logfile is NULL.
 *
 * Example:
 *   log_msg("UDP Thread. PAN RESOLUTION → %d bins\n", G_Panadapter_Pixels);
 */
void log_msg(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* MSCC_LOG_MSG_H */
