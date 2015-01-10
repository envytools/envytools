#ifndef DEMMT_DECODE_UTILS_H
#define DEMMT_DECODE_UTILS_H

#include "mmt_bin_decode.h"

void dump_mmt_buf_as_text(struct mmt_buf *buf, const char *pfx);
void dump_mmt_buf_as_words(struct mmt_buf *buf);
void dump_mmt_buf_as_words_horiz(struct mmt_buf *buf, const char *pfx);
void dump_mmt_buf_as_word_pairs(struct mmt_buf *buf, const char *(*fun)(uint32_t));
void dump_mmt_buf_as_words_desc(struct mmt_buf *buf, const char *(*fun)(uint32_t));
void dump_args(struct mmt_memory_dump *args, int argc, uint64_t except_ptr);

#endif
