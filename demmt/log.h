#ifndef DEMMT_LOG_H
#define DEMMT_LOG_H

#define MMT_DEBUG 0

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

extern int indent_logs;
extern int mmt_sync_fd;

#define fflush_stdout(fmt)         do { if (mmt_sync_fd != -1 && fmt[strlen(fmt) - 1] == '\n') fflush(stdout); } while (0)
#define mmt_debug(fmt, ...)        do { if (MMT_DEBUG) { fprintf(stdout, "DBG: " fmt, __VA_ARGS__); fflush_stdout(fmt); } } while (0)
#define mmt_debug_cont(fmt, ...)   do { if (MMT_DEBUG) { fprintf(stdout, fmt, __VA_ARGS__); fflush_stdout(fmt); } } while (0)
#define mmt_printf(fmt, ...)       do { fprintf(stdout, fmt, __VA_ARGS__); fflush_stdout(fmt); } while (0)
#define mmt_log(fmt, ...)          do { if (indent_logs) fprintf(stdout, "%64s" fmt, " ", __VA_ARGS__); else fprintf(stdout, "LOG: " fmt, __VA_ARGS__); fflush_stdout(fmt); } while (0)
#define mmt_log_cont(fmt, ...)     do { fprintf(stdout, fmt, __VA_ARGS__); fflush_stdout(fmt); } while (0)
#define mmt_log_cont_nl()          do { fprintf(stdout, "\n"); fflush_stdout("\n"); } while (0)
#define mmt_error(fmt, ...)        do { fprintf(stdout, "ERROR: " fmt, __VA_ARGS__); fflush_stdout(fmt); } while (0)

#define _print_x64(pfx, strct, field)	mmt_log_cont("%s" #field ": 0x%016" PRIx64, pfx, (strct)->field)
#define _print_x32(pfx, strct, field)	mmt_log_cont("%s" #field ": 0x%08"  PRIx32, pfx, (strct)->field)
#define _print_x16(pfx, strct, field)	mmt_log_cont("%s" #field ": 0x%04"  PRIx16, pfx, (strct)->field)
#define _print_x8( pfx, strct, field)	mmt_log_cont("%s" #field ": 0x%02"  PRIx8,  pfx, (strct)->field)

#define _print_d64_align(pfx, strct, field, algn)	mmt_log_cont("%s" #field ": %" algn PRId64, pfx, (strct)->field)
#define _print_d32_align(pfx, strct, field, algn)	mmt_log_cont("%s" #field ": %" algn PRId32, pfx, (strct)->field)
#define _print_d16_align(pfx, strct, field, algn)	mmt_log_cont("%s" #field ": %" algn PRId16, pfx, (strct)->field)
#define _print_d8_align( pfx, strct, field, algn)	mmt_log_cont("%s" #field ": %" algn PRId8,  pfx, (strct)->field)

#define _print_d64(pfx, strct, field)	_print_d64_align(pfx, strct, field, "")
#define _print_d32(pfx, strct, field)	_print_d32_align(pfx, strct, field, "")
#define _print_d16(pfx, strct, field)	_print_d16_align(pfx, strct, field, "")
#define _print_d8( pfx, strct, field)	_print_d8_align (pfx, strct, field, "")

#define _print_str(pfx, strct, field)	mmt_log_cont("%s" #field ": \"%s\"", pfx, (strct)->field)



#define print_x64(strct, field)		_print_x64("", strct, field)
#define print_x32(strct, field)		_print_x32("", strct, field)
#define print_x16(strct, field)		_print_x16("", strct, field)
#define print_x8( strct, field)		_print_x8 ("", strct, field)

#define print_d64(strct, field)		_print_d64_align("", strct, field, "")
#define print_d32(strct, field)		_print_d32_align("", strct, field, "")
#define print_d16(strct, field)		_print_d16_align("", strct, field, "")
#define print_d8( strct, field)		_print_d8_align ("", strct, field, "")

#define print_str(strct, field)		_print_str("", strct, field)

#endif
