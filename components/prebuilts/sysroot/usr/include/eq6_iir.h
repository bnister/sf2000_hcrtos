#pragma once
#ifndef AUDIO_IIR_H
#define AUDIO_IIR_H

#ifdef __cplusplus
#include <cstdint>
using std::int32_t;
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IIR_COEF_FRAC_NBIT 24
typedef int32_t iir_coef_t;
#define IIR_COEF_Q_FROM_FLOAT(x) (iir_coef_t)((x) * (1 << IIR_COEF_FRAC_NBIT))

struct biquad_t {
  iir_coef_t b0;
  iir_coef_t b1;
  iir_coef_t b2;
  iir_coef_t a1;
  iir_coef_t a2;
};

struct iir_ctx_t {
  const struct biquad_t *coef;
  int32_t x1;
  int32_t x2;
  int32_t y1;
  int32_t y2;
};

static inline
void iir_reset(struct iir_ctx_t *ctx, const struct biquad_t *coef) {
  ctx->coef = coef;
  ctx->x1 = 0;
  ctx->x2 = 0;
  ctx->y1 = 0;
  ctx->y2 = 0;
}

// IIR direct form I implementation
static inline int32_t iir_filter(struct iir_ctx_t *ctx, int32_t x0) {
  const struct biquad_t *c = ctx->coef;
  int64_t y = (int64_t)x0 * c->b0;
  y += (int64_t)ctx->x1 * c->b1;
  y += (int64_t)ctx->x2 * c->b2;
  y -= (int64_t)ctx->y1 * c->a1;
  y -= (int64_t)ctx->y2 * c->a2;
  y += 1 << (IIR_COEF_FRAC_NBIT - 1);
  y >>= IIR_COEF_FRAC_NBIT;
  ctx->x2 = ctx->x1;
  ctx->x1 = x0;
  ctx->y2 = ctx->y1;
  ctx->y1 = y;
  return y;
}

#ifdef __cplusplus
}
#endif

#endif // AUDIO_IIR_H