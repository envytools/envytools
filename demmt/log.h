#ifndef DEMMT_LOG_H
#define DEMMT_LOG_H

#define MMT_DEBUG 0

extern int indent_logs;

#include <stdio.h>
#define mmt_debug(fmt, ...)        do { if (MMT_DEBUG)        fprintf(stdout, "DBG: " fmt, __VA_ARGS__); } while (0)
#define mmt_debug_cont(fmt, ...)   do { if (MMT_DEBUG)        fprintf(stdout, fmt, __VA_ARGS__); } while (0)
#define mmt_printf(fmt, ...)       do { fprintf(stdout, fmt, __VA_ARGS__); } while (0)
#define mmt_log(fmt, ...)          do { if (indent_logs) fprintf(stdout, "%64s" fmt, " ", __VA_ARGS__); else fprintf(stdout, "LOG: " fmt, __VA_ARGS__); } while (0)
#define mmt_log_cont(fmt, ...)     do { fprintf(stdout, fmt, __VA_ARGS__); } while (0)
#define mmt_log_cont_nl()          do { fprintf(stdout, "\n"); } while (0)
#define mmt_error(fmt, ...)        do { fprintf(stdout, "ERROR: " fmt, __VA_ARGS__); } while (0)

#endif
