#ifndef DEMMT_H
#define DEMMT_H

#define MMT_DEBUG 0

#define mmt_debug(fmt, ...) do { if (MMT_DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)
#define mmt_log(fmt, ...)   do { fprintf(stdout, "%64s" fmt, " ", __VA_ARGS__); } while (0)
#define mmt_error(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while (0)

extern struct rnndomain *domain;
extern struct rnndb *rnndb;
extern int chipset;
extern int guess_invalid_pushbuf;
extern int m2mf_hack_enabled;

#endif
