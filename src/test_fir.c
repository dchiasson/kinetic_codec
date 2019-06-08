#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "fir_predictive_filter.h"


int main() {
  int fd;
  fd = open("../data/fixed_poly_reg/4_deg_poly_reg/4", O_RDONLY, 0);
  int32_t *poly_reg = (int32_t*)mmap(NULL, sizeof(int32_t)*4*4, PROT_READ, MAP_SHARED, fd, 0);
  print_poly(poly_reg);
  close(fd);
  return 0;
  initialize_poly_predictor(8, 4);
  return 0;
  //int sample_count = 965286;
  int sample_count = 6167;
  Vect accel = {};
  Vect gyro = {};
  Vect mag = {};
  Position input_pos = {NULL, NULL, &accel};
  Orientation input_orient = {&mag, &gyro, NULL};
  ObjectState input_state = {};

  // Read in input data
  //int fd;
  fd = open("../data/raw_data/0_AccelX", O_RDONLY, 0);
  accel.x = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_AccelY", O_RDONLY, 0);
  accel.y = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_AccelZ", O_RDONLY, 0);
  accel.z = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  accel.data_len = sample_count;

  fd = open("../data/raw_data/0_GyroX", O_RDONLY, 0);
  gyro.x = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_GyroY", O_RDONLY, 0);
  gyro.y = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_GyroZ", O_RDONLY, 0);
  gyro.z = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  gyro.data_len = sample_count;

  fd = open("../data/raw_data/0_MagX", O_RDONLY, 0);
  mag.x = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_MagY", O_RDONLY, 0);
  mag.y = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_MagZ", O_RDONLY, 0);
  mag.z = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  mag.data_len = sample_count;
  build_object_state(&input_pos, &input_orient, &input_state);

  for (int i=0; i < sample_count; i++) {
    //printf("%d ", accel.x[i]);
  }
  printf("\n");

  // Output data
  Vect accel_out = {};
  Vect gyro_out = {};
  Vect mag_out = {};
  accel_out.x = calloc(1, sizeof(uint16_t) * sample_count);
  accel_out.y = calloc(1, sizeof(uint16_t) * sample_count);
  accel_out.z = calloc(1, sizeof(uint16_t) * sample_count);
  gyro_out.x = calloc(1, sizeof(uint16_t) * sample_count);
  gyro_out.y = calloc(1, sizeof(uint16_t) * sample_count);
  gyro_out.z = calloc(1, sizeof(uint16_t) * sample_count);
  mag_out.x = calloc(1, sizeof(uint16_t) * sample_count);
  mag_out.y = calloc(1, sizeof(uint16_t) * sample_count);
  mag_out.z = calloc(1, sizeof(uint16_t) * sample_count);
  Position output_pos = {NULL, NULL, &accel_out};
  Orientation output_orient = {&mag_out, &gyro_out, NULL};
  ObjectState output_state = {};
  build_object_state(&output_pos, &output_orient, &output_state);


  // @TODO(David): how do I know this is big enough?!
  uint8_t *data_out = calloc(1, sample_count * 9 * 2 * 10); // worse than .1 compression ratio will seg fault
  CompressedDataWriter data_writer = {};
  data_writer.data_out = data_out;

  params.rice_k = 3;
  params.block_size = 2000;
  params.prediction_strategy = PREDICTION_DIFF_ENCODING;


  encode_block(&input_state, &data_writer);
  uint32_t block_len = data_writer.bit_pointer >> 3;


  // Decode the data we just compressed to check for errors
  CompressedDataReader data_reader = {};
  data_reader.data_in = data_out;

  decode_block(&data_reader, &output_state, sample_count);
  check_errors(&input_state, &output_state);

  printf("RICE K: %d\n", params.rice_k);
  printf("Block size: %d\n", params.block_size);
  printf("Output byte len: %d\n", (int)block_len);
  printf("Compression ratio: %f\n", ((double)(sample_count*2*9)/block_len));

  if (data_out)
    free(data_out);
  return 0;
}
