/**
 * Fixed FIR predictive filtering
 *
 * A fixed point FIR filter is used to predict future measurements.
 * If the prediction is good, residual will be much more compressible
 * than the original data.
 *
 * This allows the efficient implementation of polynomial regression
 * based prediction and high order differential encoding.
 *
 * Filter coefficients must be provided in fixed point 16.16 format
 * to guarantee identical computation across diverse hardware.
 */
#include <stdint.h>
#include "fix16.h"
#include "fir_predictive_filter.h"

#define FIR_BUFFER_MASK (MAX_FIR_LENGTH - 1)

/**
 * Load pre-computed FIR predictive filter coefficients
 *
 * Filter coefficients must be in fixed point fix16_t format
 */
///@TODO(David): clear history, set initialization values
int init_fir_filter(int32_t *coeffs, int length, FIRFilterState *state) {
  if (length > MAX_FIR_LENGTH)
    return -1;
  for (int i = 0; i < length; i++)
    state->filter_coefs[i] = coeffs[i];
  state->filter_length = length;
  for (int sample=0; sample < MAX_FIR_LENGTH; sample++) {
    state->ring_buffer.past[sample] = fix16_from_int(0);
  }
  return 0;
}

/**
 * Return current filter output
 */
static fix16_t apply_filter(FIRFilterState *state) {
  fix16_t sum = 0;
  for (int i=0; i < state->filter_length; i++) {
    uint32_t buffer_place = (state->ring_buffer.pointer - i) & FIR_BUFFER_MASK;
    fix16_t product = fix16_mul(
        state->filter_coefs[state->filter_length-i-1],
        fix16_from_int(state->ring_buffer.past[buffer_place]));
    sum = fix16_add(sum, product);
    if (sum == fix16_overflow || product == fix16_overflow) {
      //overflow
    }
  }
  return sum;
}

/**
 * Update input ring buffer with new value
 */
static int update_filter(int16_t update, FIRFilterState *state) {
  state->ring_buffer.pointer = (state->ring_buffer.pointer + 1) & FIR_BUFFER_MASK;
  state->ring_buffer.past[state->ring_buffer.pointer] = update;
  return 0;
}

/**
 * Apply FIR predictive filter and return the residual
 * For auto-correlation encoding
 */
int16_t get_fir_residual(int16_t measurement, FIRFilterState *state) {
  int16_t residual = measurement - fix16_to_int(apply_filter(state));
  update_filter(measurement, state);
  return residual;
}

/**
 * Convert residual into original measurement
 * For auto-correlation decoding
 */
int16_t decode_fir_residual(int16_t residual, FIRFilterState *state) {
  int16_t measurement = fix16_to_int(apply_filter(state)) + residual;
  update_filter(measurement, state);
  return measurement;
}

/**
 * Apply FIR predictive filter and return the residual
 * For cross-correlation encoding
 */
int16_t get_cross_fir_residual(int16_t measurement, CrossFIRFilterState *state) {
  fix16_t prediction = 0;
  for (int i=0; i < CROSS_STREAM_CT; i++) {
    prediction += apply_filter(&state->stream_state[i]);
  }
  return measurement - fix16_to_int(prediction);
}

/**
 * Convert residual into original measurement
 * For cross-correlation decoding
 */
int16_t decode_cross_fir_residual(int16_t residual, CrossFIRFilterState *state) {
  fix16_t prediction = 0;
  for (int i=0; i < CROSS_STREAM_CT; i++) {
    prediction += apply_filter(&state->stream_state[i]);
  }
  return fix16_to_int(prediction) + residual;
}

/**
 * Update filter states for cross-correlation encoding/decoding
 */
void update_cross_fir_filters(int16_t measurement, CrossFIRFilterState *filters, int stream) {
  for (int i=0; i < CROSS_STREAM_CT; i++) {
    update_filter(measurement, &filters[i].stream_state[stream]);
  }
}
