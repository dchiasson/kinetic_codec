#ifndef SRC__FIR_PREDICTIVE_FILTER_H
#define SRC__FIR_PREDICTIVE_FILTER_H

#define MAX_FIR_LENGTH 256 // must be power of two
#define CROSS_STREAM_CT 6

#define FILTER_TYPE__AUTO_FIR 1
#define FILTER_TYPE__CROSS_FIR 2

typedef struct{
  int16_t past[MAX_FIR_LENGTH];
  uint32_t pointer;
}FIRRingBuffer;

typedef struct{
  FIRRingBuffer ring_buffer;
  int32_t filter_coefs[MAX_FIR_LENGTH];
  int filter_length;
}FIRFilterState;

typedef struct{
  FIRFilterState stream_state[CROSS_STREAM_CT];
}CrossFIRFilterState;

/* single stream (auto-correlation) */
int init_fir_filter(int32_t *coeffs, int length, FIRFilterState *state);
int16_t get_fir_residual(int16_t measurement, FIRFilterState *state);
int16_t decode_fir_residual(int16_t residual, FIRFilterState *state);

/* multple streams (cross-correlation) */
int16_t get_cross_fir_residual(int16_t measurement, CrossFIRFilterState *state);
int16_t decode_cross_fir_residual(int16_t residual, CrossFIRFilterState *state);
void update_cross_fir_filters(int16_t measurement, CrossFIRFilterState *filters, int stream);

#endif // SRC__FIR_PREDICTIVE_FILTER_H
