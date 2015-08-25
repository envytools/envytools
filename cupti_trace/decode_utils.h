#ifndef TRACE_DECODE_H
#define TRACE_DECODE_H

void init_rnnctx(const char *chipset, int use_colors);
void destroy_rnnctx();

int lookup_trace(const char *chipset, const char *event);

#endif
