#include <stdio.h>
#include "jenc_rdo.h"

static inline
void jenc_rdo_q_init(struct jenc_rdo_q_t *q,
                     unsigned idx, unsigned size, unsigned thr) {
    q->idx = (uint8_t)idx;
    q->max = (uint8_t)(size - 1);
    q->cnt = 0;
    q->thr = (int8_t)thr;
}

enum jenc_rdo_q_op_result_t {
    JENC_RDO_Q_OP_RESULT_FAILED,
    JENC_RDO_Q_OP_RESULT_OK,
    JENC_RDO_Q_OP_RESULT_THR_REACHED,
};

static inline
enum jenc_rdo_q_op_result_t jenc_rdo_q_step_down(struct jenc_rdo_q_t *q) {
    if (q->idx > 0) {
        q->idx--;
        return --q->cnt > -q->thr ? JENC_RDO_Q_OP_RESULT_OK
                                  : JENC_RDO_Q_OP_RESULT_THR_REACHED;
    } else
        return JENC_RDO_Q_OP_RESULT_FAILED;
}

static inline
enum jenc_rdo_q_op_result_t jenc_rdo_q_step_up(struct jenc_rdo_q_t *q) {
    if (q->idx < q->max) {
        q->idx++;
        return ++q->cnt < q->thr ? JENC_RDO_Q_OP_RESULT_OK
                                 : JENC_RDO_Q_OP_RESULT_THR_REACHED;
    } else
        return 0;
}

static inline void jenc_rdo_q_reset_cnt(struct jenc_rdo_q_t *q) {
    q->cnt = 0;
}

void jenc_rdo_init(struct jenc_rdo_t *rdo,
        unsigned y_q_tbl_size, unsigned y_q_tbl_idx, unsigned y_op_cntr_thr,
        unsigned c_q_tbl_size, unsigned c_q_tbl_idx, unsigned c_op_cntr_thr) {
    rdo->last_q = NULL;
    jenc_rdo_q_init(&rdo->y_q, y_q_tbl_idx, y_q_tbl_size, y_op_cntr_thr);
    jenc_rdo_q_init(&rdo->c_q, c_q_tbl_idx, c_q_tbl_size, c_op_cntr_thr);
    rdo->c_y_q_diff = (int)c_q_tbl_idx - (int)y_q_tbl_idx;
}

static inline
struct jenc_rdo_q_t *jenc_rdo_switch_component(struct jenc_rdo_t *rdo) {
    struct jenc_rdo_q_t *q = rdo->last_q == &rdo->y_q ? &rdo->c_q
                                                      : &rdo->y_q;
    jenc_rdo_q_reset_cnt(rdo->last_q);
    rdo->last_q = q;
    return q;
}
typedef enum jenc_rdo_q_op_result_t (*jenc_op_func)(struct jenc_rdo_q_t *q);

void jenc_rdo_rate_control(struct jenc_rdo_t *rdo, enum jenc_rdo_op_t op) {
    /*enum jenc_rdo_q_op_result_t (*op_func)() = NULL;*/
	jenc_op_func op_func = NULL;
    if (op == JENC_RDO_OP_INCREASE_BITRATE) {
        if (rdo->last_q == NULL || // first operation
			rdo->y_q.idx >= rdo->c_q.idx - rdo->c_y_q_diff)
            rdo->last_q = &rdo->y_q;
        op_func = jenc_rdo_q_step_down;
    } else if (op == JENC_RDO_OP_REDUCE_BITRATE) {
    	if (rdo->last_q == NULL || // first operation
			rdo->y_q.idx >= rdo->c_q.idx - rdo->c_y_q_diff)
            rdo->last_q = &rdo->c_q;
        op_func = jenc_rdo_q_step_up;
    }
    if (op_func) {
        int op_res = op_func(rdo->last_q);
	if (op_res == JENC_RDO_Q_OP_RESULT_FAILED) {
		struct jenc_rdo_q_t *q = jenc_rdo_switch_component(rdo);
		op_res = op_func(q);
	} else if (op_res == JENC_RDO_Q_OP_RESULT_THR_REACHED) {
		// op succeeded, switch to the other component for net op
		jenc_rdo_switch_component(rdo);
	}
    }
}
