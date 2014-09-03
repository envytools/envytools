#ifndef MMT_BIN2DEDMA_H_
#define MMT_BIN2DEDMA_H_

#define PFX "--0000-- "

#include "mmt_bin_decode.h"
#include "mmt_bin_decode_nvidia.h"

void txt_memread(struct mmt_read *w, void *state);
void txt_memwrite(struct mmt_write *w, void *state);
void txt_mmap(struct mmt_mmap *mm, void *state);
void txt_munmap(struct mmt_unmap *mm, void *state);
void txt_mremap(struct mmt_mremap *mm, void *state);
void txt_open(struct mmt_open *o, void *state);
void txt_msg(uint8_t *data, int len, void *state);

struct mmt_txt_nvidia_state
{
} mmt_txt_nv_state;

extern const struct mmt_nvidia_decode_funcs txt_nvidia_funcs;
extern const struct mmt_nvidia_decode_funcs txt_nvidia_funcs_empty;

#endif /* MMT_BIN2TEXT_H_ */
