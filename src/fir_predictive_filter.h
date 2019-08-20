#ifndef SRC__FIR_PREDICTIVE_FILTER_H
#define SRC__FIR_PREDICTIVE_FILTER_H

#define MAX_FIR_LENGTH 16384 // must be power of two

typedef struct{
  int16_t past[MAX_FIR_LENGTH];
  uint32_t pointer;
}FIRRingBuffer;

typedef struct{
  FIRRingBuffer ring_buffer;
  int32_t filter_coefs[MAX_FIR_LENGTH];
  int filter_length;
}FIRFilterState;

int init_fir_filter(int32_t *coeffs, int length, FIRFilterState *state);
int16_t get_fir_residual(int16_t measurement, FIRFilterState *state);
int16_t decode_fir_residual(int16_t residual, FIRFilterState *state);

#endif // SRC__FIR_PREDICTIVE_FILTER_H
