#pragma once

#include <stdint.h>

enum jenc_rdo_op_t {
    JENC_RDO_OP_KEEP,
    JENC_RDO_OP_REDUCE_BITRATE,
    JENC_RDO_OP_INCREASE_BITRATE,
};

struct jenc_rdo_q_t {
    // table select index
    uint8_t idx;
    // max index of quantization table (size - 1)
    uint8_t max;
    // successive operation conuter, initialized to 0
    int8_t cnt;
    // threshold of successive operations
    int8_t thr;
};

struct jenc_rdo_t {
    struct jenc_rdo_q_t *last_q;
    struct jenc_rdo_q_t y_q;
    struct jenc_rdo_q_t c_q;
    int c_y_q_diff;
};

/// @brief initialize JPEG encoder RDO
/// @param rdo the RDO object
/// @param y_q_tbl_size luma quantization table size
/// @param y_q_tbl_idx initial luma quantization index
/// @param y_op_cntr_thr successive luma quantization operation threshold
/// @param c_q_tbl_size chroma quantization table size
/// @param c_q_tbl_idx initial chroma quantization index
/// @param c_op_cntr_thr successive chroma quantization operation threshold
void jenc_rdo_init(struct jenc_rdo_t *rdo,
        unsigned y_q_tbl_size, unsigned y_q_tbl_idx, unsigned y_op_cntr_thr,
        unsigned c_q_tbl_size, unsigned c_q_tbl_idx, unsigned c_op_cntr_thr);
void jenc_rdo_rate_control(struct jenc_rdo_t *rdo, enum jenc_rdo_op_t op);

static inline
int jenc_rdo_y_q_index(const struct jenc_rdo_t *rdo) {
    return rdo->y_q.idx;
}

static inline
int jenc_rdo_c_q_index(const struct jenc_rdo_t *rdo) {
    return rdo->c_q.idx;
}
