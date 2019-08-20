#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "fir_predictive_filter.h"

typedef struct{
  int16_t *x;
  int16_t *y;
  int16_t *z;
  /// @TODO(David): Should these two be associated with each diminsion or with each object? My pointer arrays aren't being useful...
  uint32_t data_len;
  uint32_t data_index;
}Vect;

typedef struct{
  Vect *pos;
  Vect *vel;
  Vect *accel;
}Position;

typedef struct{
  Vect *orient;
  Vect *rot_vel;
  Vect *rot_accel;
}Orientation;

typedef struct{
  Position *position;
  Orientation *orientation;
  int16_t *pointers[18];
  FIRFilterState fir_state[18]; // @TODO(David): we can make this a void pointer to make it more flexible
  int pointer_count;
}ObjectState;

typedef struct{
  uint8_t *data_out;
  uint64_t bit_pointer;
}CompressedDataWriter;

typedef struct{
  uint8_t *data_in;
  uint64_t bit_pointer;
}CompressedDataReader;

typedef struct{
  uint8_t rice_k;
  int block_size;
  int prediction_strategy;
}Parameters;
#define BLOCK_SIZE_MAX (-1)
#define PREDICTION_DIFF_ENCODING 1
Parameters params;

void build_object_state(Position *position, Orientation *orientation, int order, int hist, ObjectState *object_state) {
  object_state->position = position;
  object_state->orientation = orientation;
  object_state->pointer_count = 0;
  if (position) {
    if(position->pos) {
      object_state->pointers[object_state->pointer_count++] = position->pos->x;
      object_state->pointers[object_state->pointer_count++] = position->pos->y;
      object_state->pointers[object_state->pointer_count++] = position->pos->z;
    }
    if(position->vel) {
      object_state->pointers[object_state->pointer_count++] = position->vel->x;
      object_state->pointers[object_state->pointer_count++] = position->vel->y;
      object_state->pointers[object_state->pointer_count++] = position->vel->z;
    }
    if(position->accel) {
      object_state->pointers[object_state->pointer_count++] = position->accel->x;
      object_state->pointers[object_state->pointer_count++] = position->accel->y;
      object_state->pointers[object_state->pointer_count++] = position->accel->z;
    }
  }
  if (orientation) {
    /// @TODO(David): Change this order
    if(orientation->rot_vel) {
      object_state->pointers[object_state->pointer_count++] = orientation->rot_vel->x;
      object_state->pointers[object_state->pointer_count++] = orientation->rot_vel->y;
      object_state->pointers[object_state->pointer_count++] = orientation->rot_vel->z;
    }
    if(orientation->orient) {
      object_state->pointers[object_state->pointer_count++] = orientation->orient->x;
      object_state->pointers[object_state->pointer_count++] = orientation->orient->y;
      object_state->pointers[object_state->pointer_count++] = orientation->orient->z;
    }
    if(orientation->rot_accel) {
      object_state->pointers[object_state->pointer_count++] = orientation->rot_accel->x;
      object_state->pointers[object_state->pointer_count++] = orientation->rot_accel->y;
      object_state->pointers[object_state->pointer_count++] = orientation->rot_accel->z;
    }
  }
  int fd;
  char buffer[50];
  sprintf(buffer, "../data/fixed_poly_pred/%d_deg_poly_reg/%d", order, hist);
  fd = open(buffer, O_RDONLY, 0);
  int32_t *poly_reg = (int32_t*)mmap(NULL, sizeof(int32_t)*hist, PROT_READ, MAP_SHARED, fd, 0);
  for (int i=0; i<18; i++) {
    init_fir_filter(poly_reg, hist, &object_state->fir_state[i]);
  }
  close(fd);
}

void put_bit(uint8_t bit, CompressedDataWriter *data_writer)
{
  data_writer->data_out[data_writer->bit_pointer >> 3] |= (bit & 1) << (data_writer->bit_pointer & 7);
  (data_writer->bit_pointer)++;
}

uint8_t get_bit(CompressedDataReader *data_reader) {
  uint8_t bit;
  bit = (data_reader->data_in[data_reader->bit_pointer >> 3] >> (data_reader->bit_pointer & 7)) & 1;
  data_reader->bit_pointer++;
  return bit;
}

void put_word(uint16_t word, CompressedDataWriter *data_writer) {
  // Big endian
  data_writer->bit_pointer += 8 - (data_writer->bit_pointer & 0b111); // pad with zeros till byte boundary
  data_writer->data_out[data_writer->bit_pointer >> 3] = word >> 8 & 0xff;
  data_writer->bit_pointer += 8;
  data_writer->data_out[data_writer->bit_pointer >> 3] = word & 0xff;
  data_writer->bit_pointer += 8;
}

uint16_t get_word(CompressedDataReader *data_reader) {
  // Big endian
  data_reader->bit_pointer += 8 - (data_reader->bit_pointer & 0b111); // padded with zeros till byte boundary
  uint16_t val = 0;
  val |= data_reader->data_in[data_reader->bit_pointer >> 3] << 8;
  data_reader->bit_pointer += 8;
  val |= data_reader->data_in[data_reader->bit_pointer >> 3];
  data_reader->bit_pointer += 8;
  return val;
}


void put_vector_0(Vect *vect, CompressedDataWriter *data_writer) {
    put_word(vect->x[0], data_writer);
    put_word(vect->y[0], data_writer);
    put_word(vect->z[0], data_writer);
}

/// @TODO(David): Use the FLAC functions, they are way more efficient
void rice_encode(int32_t res, CompressedDataWriter *data_writer) {
  uint16_t residual;
  if (res < 0) // negative is odd
    residual = -res*2-1;
  else // positive is even
    residual = res*2;
  //printf("%lu : %d \n", data_writer->bit_pointer, residual);
  // exponent
  data_writer->bit_pointer += residual >> params.rice_k; // faster than put_bit
  // one termination
  put_bit(1, data_writer);
  // remainder
  for (int i=0; i < params.rice_k; i++) {
    put_bit((residual >> i) & 1, data_writer);
  }
}

int32_t rice_decode(CompressedDataReader *data_reader) {
  uint16_t res = 0;
  while (!get_bit(data_reader)) {
    res++;
  }
  res <<= params.rice_k;
  for (int i=0; i < params.rice_k; i++) {
    res += (get_bit(data_reader) << i);
  }
  if (res & 1) { // odd is negative
    return -(res + 1) / 2;
  } else { // even is positive
    return res / 2;
  }
}

int encode_block(ObjectState *input_state, CompressedDataWriter *data_writer) {
  if (params.prediction_strategy == PREDICTION_DIFF_ENCODING) {
    uint32_t sample_count = input_state->position->accel->data_len;
    for (uint32_t sample=0; sample < sample_count; sample++) {
      if ((params.block_size == BLOCK_SIZE_MAX && sample == 0) ||
          (sample % params.block_size == 0)) { // block header
        for (int stream=0; stream < input_state->pointer_count; stream++) {
          put_word(input_state->pointers[stream][sample], data_writer);
        }
      } else {
        long res;
        for (int stream=0; stream < input_state->pointer_count; stream++) {
          res = input_state->pointers[stream][sample] - input_state->pointers[stream][sample-1];
          //printf("%d\n", res);
          rice_encode(res, data_writer);
        }
      }
    }
  }
  return 0;
}

int encode_data(ObjectState *input_state, CompressedDataWriter *data_writer) {
  uint32_t sample_count = input_state->position->accel->data_len;
  for (uint32_t sample=0; sample < sample_count; sample++) {
    for (int stream=0; stream < input_state->pointer_count; stream++) {
      int32_t res = get_fir_residual(input_state->pointers[stream][sample], &input_state->fir_state[stream]);
      //printf("%d\n", res);
      //rice_encode(get_fir_residual(input_state->pointers[stream][sample], &input_state->fir_state[stream]), data_writer);
      rice_encode(res, data_writer);
    }
  }
  return 0;
}
int decode_data(CompressedDataReader *data_reader, ObjectState *output_state, uint32_t sample_count) {
  for (uint32_t sample=0; sample < sample_count; sample++) {
    for (int stream=0; stream < output_state->pointer_count; stream++) {
      output_state->pointers[stream][sample] = decode_fir_residual(rice_decode(data_reader), &output_state->fir_state[stream]);
    }
  }
  return 0;
}

int decode_block(CompressedDataReader *data_reader, ObjectState *output_state, uint32_t sample_count) {
  if (params.prediction_strategy == PREDICTION_DIFF_ENCODING) {
    for (uint32_t sample=0; sample < sample_count; sample++) {
      if ((params.block_size == BLOCK_SIZE_MAX && sample == 0) ||
          (sample % params.block_size == 0)) { // block header
        for (int stream=0; stream < output_state->pointer_count; stream++) {
          output_state->pointers[stream][sample] = get_word(data_reader);
        }
      } else {
        for (int stream=0; stream < output_state->pointer_count; stream++) {
          output_state->pointers[stream][sample] = output_state->pointers[stream][sample-1] + rice_decode(data_reader);
        }
      }
    }
  }
  return 0;
}

int check_errors(ObjectState *input_state, ObjectState *output_state) {
  uint32_t sample_count = input_state->position->accel->data_len;

  for (int p = 0; p < output_state->pointer_count; p++) {
    for (int s=0; s < (int)sample_count; s++) {
      if (output_state->pointers[p][s] != input_state->pointers[p][s]) {
        printf("ERROR p: %d s: %d\n",p,s);
        exit(1);
      }
    }
  }
  return 0;
}

int main() {
  int fd;
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

  for (int order=0; order<=6; order++) {
    for (int hist=order; hist<7; hist++) {
      int best_k = 0;
      double best_cr = 0;
      for (int k=6; k<14; k++) {
  build_object_state(&input_pos, &input_orient, order, hist, &input_state);

  for (int i=0; i < sample_count; i++) {
    //printf("%d ", accel.x[i]);
  }

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
  //build_object_state(&output_pos, &output_orient, order, hist, &output_state);


  // @TODO(David): how do I know this is big enough?!
  uint8_t *data_out = calloc(1, sample_count * 9 * 2 * 1000); // worse than .1 compression ratio will seg fault
  CompressedDataWriter data_writer = {};
  data_writer.data_out = data_out;

  params.rice_k = k;
  params.block_size = BLOCK_SIZE_MAX;
  params.prediction_strategy = PREDICTION_DIFF_ENCODING;


  //encode_block(&input_state, &data_writer);
  encode_data(&input_state, &data_writer);
  uint32_t block_len = data_writer.bit_pointer >> 3;


  // Decode the data we just compressed to check for errors
  CompressedDataReader data_reader = {};
  data_reader.data_in = data_out;

  //decode_block(&data_reader, &output_state, sample_count);
  //decode_data(&data_reader, &output_state, sample_count);
  //check_errors(&input_state, &output_state);

  //printf("RICE K: %d\n", params.rice_k);
  //printf("Block size: %d\n", params.block_size);
  //printf("Output byte len: %d\n", (int)block_len);
  //printf("Compression ratio: %f\n", ((double)(sample_count*2*9)/block_len));
  double cr =  ((double)(sample_count*2*9)/block_len);
  if (cr > best_cr) {
    best_k = k;
    best_cr = cr;
  }
  if (data_out)
    free(data_out);


      }
  printf("order: %d, hist: %d, k: %d, CR: %f\n", order, hist, best_k, best_cr);
      if (order == 0) hist += 100;
    }
  }

  return 0;
}
