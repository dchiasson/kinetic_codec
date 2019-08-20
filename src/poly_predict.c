#include "fixmatrix.h"
#include "fix16.h"
#include <stdio.h>
#include <stdint.h>

mf16 coef_computer;
int history_len;
int poly_order;

// this power function only works for small positive integers
int power(uint16_t a, uint16_t b) {
  int ret_val = 1;
  for (int i = 0; i < b; i++) 
    ret_val *= a;
  return ret_val;
}

void print_poly(int32_t *poly) {
  for (int n=0; n < 4; n++) {
    for (int m=0; m < 4; m++) {
      printf("%f ", fix16_to_float(poly[n*4 + m]));
    }
    printf("\n");
  }
  printf("\n");
  printf("\n");

}

/**
 * Compute the polynomial regression matrix used to find the polynomial coefficients
 *
 * This 
 */
int initialize_poly_predictor(int history_len, int poly_order) {
  poly_order = poly_order;
  history_len = history_len;
  mf16 x;
  x.rows = history_len;
  x.columns = poly_order;
  mf16_fill(&x, 0);
  for (int n=0; n < history_len; n++) {
    for (int m=0; m < poly_order; m++) {
      x.data[n][m] = fix16_from_int(power(n, m));
    }
  }
  mf16_mul_at(&coef_computer, &x, &x);
  mf16_cholesky(&coef_computer, &coef_computer);
  mf16_invert_lt(&coef_computer, &coef_computer);
  mf16_mul_bt(&coef_computer, &coef_computer, &x);
  return 0;
}


//int16_t fixed_polynomial_prediction(int16_t real_state, int history_len, int poly_order) {
  //int16_t coefs[poly_order];
  //compute_coefs(history_len, poly_order, coefs);
  //int16_t predicted_state = predict_state(coefs, history_len, poly_order);
  //push_value(real_state);
  //return (real_state - predicted_state);
//}
//update with new measurement
//compute new measurement prediction from past
//return prediction residual



