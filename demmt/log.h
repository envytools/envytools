#ifndef DEMMT_LOG_H
#define DEMMT_LOG_H

#define MMT_DEBUG 0

#include <string.h>

extern int indent_logs;
extern int mmt_sync_fd;

#include <stdio.h>
#define fflush_stdout(fmt)         do { if (mmt_sync_fd != -1 && fmt[strlen(fmt) - 1] == '\n') fflush(stdout); } while (0)
#define mmt_debug(fmt, ...)        do { if (MMT_DEBUG) { fprintf(stdout, "DBG: " fmt, __VA_ARGS__); fflush_stdout(fmt); } } while (0)
#define mmt_debug_cont(fmt, ...)   do { if (MMT_DEBUG) { fprintf(stdout, fmt, __VA_ARGS__); fflush_stdout(fmt); } } while (0)
#define mmt_printf(fmt, ...)       do { fprintf(stdout, fmt, __VA_ARGS__); fflush_stdout(fmt); } while (0)
#define mmt_log(fmt, ...)          do { if (indent_logs) fprintf(stdout, "%64s" fmt, " ", __VA_ARGS__); else fprintf(stdout, "LOG: " fmt, __VA_ARGS__); fflush_stdout(fmt); } while (0)
#define mmt_log_cont(fmt, ...)     do { fprintf(stdout, fmt, __VA_ARGS__); fflush_stdout(fmt); } while (0)
#define mmt_log_cont_nl()          do { fprintf(stdout, "\n"); fflush_stdout("\n"); } while (0)
#define mmt_error(fmt, ...)        do { fprintf(stdout, "ERROR: " fmt, __VA_ARGS__); fflush_stdout(fmt); } while (0)

#endif
