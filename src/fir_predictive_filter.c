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
#define MAX_FIR_LENGTH 16384 // must be power of two
#define FIR_BUFFER_MASK (MAX_FIR_LENGTH - 1)

//ring buffer for the FIR filter history (or IIR filter?)

typedef struct{
  int16_t past[MAX_FIR_LENGTH];
  uint32_t pointer;
}RingBuffer;

RingBuffer ring_buffer;
fix_16t filter_coefs[MAX_FIR_LENGTH];
int filter_length

/**
 * Load pre-computed FIR predictive filter coefficients
 *
 * Filter coefficients must be in fixed point fix_16t format
 */
int load_fir_filter(int32_t *coeffs, int length) {
  if (length > MAX_FIR_LENGTH)
    return -1;
  for (int i = 0; i < length; i++)
    fliter_coefs[i] = coeffs[i];
  filter_length = length;
}

/**
 * Apply FIR predictive filter and return the residual
 * Encoding operation
 */
int16_t get_fir_residual(int16_t measurement) {
  int16_t residual = measurement - apply_filter();
  update_filter(measurement);
  return residual;
}
/**
 * Convert residual into original measurement
 * Decoding operation
 */
int16_t decode_residual(int16_t residual) {
  int16_t measurement = apply_filter() + residual;
  update_filter(measurement);
  return measurement;
}

/**
 * Return current filter output
 */
int16_t apply_filter() {
  fix16_t sum = 0;
  for (int i=0; i < filter_length; i++) {
    uint32_t buffer_place = (ring_buffer.pointer - i) & FIR_BUFFER_MASK;
    fix16_t product = fix16_mul(filter_coefs[filter_length-i-1], ring_buffer.past[buffer_place]);
    sum = fix16_add(sum, product);
    if (sum == fix16_overflow || product == fix16_overflow) {
      //overflow
    }
  }
  return fix16_to_int(sum);
}

/**
 * Update input ring buffer with new value
 */
int update_filter(int16_t update) {
  past[pointer] = update;
  pointer = (pointer + 1) & FIR_BUFFER_MASK;
}
