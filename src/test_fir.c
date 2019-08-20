#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "fir_predictive_filter.h"

#include "fix16.h"

#include "unittests.h"


int main() {
  int status = 0;
  COMMENT("Test FIR Predictive Filter");
  int fd;
  fd = open("../data/fixed_poly_pred/3_deg_poly_reg/3", O_RDONLY, 0);
  int32_t *poly_reg = (int32_t*)mmap(NULL, sizeof(int32_t)*3, PROT_READ, MAP_SHARED, fd, 0);
  FIRFilterState state;
  init_fir_filter(poly_reg, 3, &state);
  int16_t res;
  res = get_fir_residual(1, &state);
  TEST(res == 1);
  res = get_fir_residual(1, &state);
  TEST(res == -2);
  res = get_fir_residual(1, &state);
  TEST(res == 1);
  res = get_fir_residual(1, &state);
  TEST(res == 0);
  res = get_fir_residual(1, &state);
  TEST(res == 0);
  close(fd);
  return 0;
}
