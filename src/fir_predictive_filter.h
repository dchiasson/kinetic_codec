#ifndef SRC__FIR_PREDICTIVE_FILTER_H
#define SRC__FIR_PREDICTIVE_FILTER_H

int load_fir_filter(int32_t *coeffs, int length);
int16_t get_fir_residual(int16_t measurement);
int16_t decode_residual(int16_t residual);

#endif // SRC__FIR_PREDICTIVE_FILTER_H
