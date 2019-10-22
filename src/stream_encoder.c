#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "fir_predictive_filter.h"
#include "data_format.h"

#define BLOCK_SIZE_MAX (-1)
#define PREDICTION_DIFF_ENCODING 1
Parameters params;

void build_stream_fir(int order, int history, ObjectState *object_state) {
  Position *position = &object_state->position;
  Orientation *orientation = &object_state->orientation;
  StreamFir *stream_fir = &object_state->stream_fir;
  object_state->stream_fir.pointer_count = 0;
  if (position) {
    if(position->pos) {
      stream_fir->pointers[stream_fir->pointer_count++] = position->pos->x;
      stream_fir->pointers[stream_fir->pointer_count++] = position->pos->y;
      stream_fir->pointers[stream_fir->pointer_count++] = position->pos->z;
    }
    if(position->vel) {
      stream_fir->pointers[stream_fir->pointer_count++] = position->vel->x;
      stream_fir->pointers[stream_fir->pointer_count++] = position->vel->y;
      stream_fir->pointers[stream_fir->pointer_count++] = position->vel->z;
    }
    if(position->accel) {
      stream_fir->pointers[stream_fir->pointer_count++] = position->accel->x;
      stream_fir->pointers[stream_fir->pointer_count++] = position->accel->y;
      stream_fir->pointers[stream_fir->pointer_count++] = position->accel->z;
    }
  }
  if (orientation) {
    /// @TODO(David): Change this order
    if(orientation->rot_vel) {
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_vel->x;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_vel->y;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_vel->z;
    }
    if(orientation->orient) {
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->orient->x;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->orient->y;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->orient->z;
    }
    if(orientation->rot_accel) {
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_accel->x;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_accel->y;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_accel->z;
    }
  }
  int fd;
  char buffer[50];
  sprintf(buffer, "../data/fixed_poly_pred/%d_deg_poly_reg/%d", order, history);
  fd = open(buffer, O_RDONLY, 0);
  int32_t *poly_reg = (int32_t*)mmap(NULL, sizeof(int32_t)*history, PROT_READ, MAP_SHARED, fd, 0);
  for (int i=0; i<18; i++) {
    init_fir_filter(poly_reg, history, &stream_fir->fir_state[i]);
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

/// @TODO(David): support block sizes
int encode_data(ObjectState *input_state, CompressedDataWriter *data_writer) {
  uint32_t sample_count = input_state->position.accel->data_len;
  for (uint32_t sample=0; sample < sample_count; sample++) {
    for (int stream=0; stream < input_state->stream_fir.pointer_count; stream++) {
      int32_t res = get_fir_residual(input_state->stream_fir.pointers[stream][sample], &input_state->stream_fir.fir_state[stream]);
      rice_encode(res, data_writer);
    }
  }
  return 0;
}
/// @TODO(David): if we are decoding blocks, we need to make sure to reset FIR history to recover
// from dropped packets
int decode_data(CompressedDataReader *data_reader, ObjectState *output_state, uint32_t sample_count) {
  for (uint32_t sample=0; sample < sample_count; sample++) {
    for (int stream=0; stream < output_state->stream_fir.pointer_count; stream++) {
      output_state->stream_fir.pointers[stream][sample] = decode_fir_residual(rice_decode(data_reader), &output_state->stream_fir.fir_state[stream]);
    }
  }
  return 0;
}

int check_errors(ObjectState *input_state, ObjectState *output_state) {
  uint32_t sample_count = input_state->position.accel->data_len;

  for (int p = 0; p < output_state->stream_fir.pointer_count; p++) {
    for (int s=0; s < (int)sample_count; s++) {
      if (output_state->stream_fir.pointers[p][s] != input_state->stream_fir.pointers[p][s]) {
        printf("ERROR p: %d s: %d\n",p,s);
        exit(1);
        break;
      }
    }
  }
  return 0;
}

int load_sensor_data(ObjectState *input_state, int *sample_count) {
  int fd;
  /// @TODO(David): write a function to free an munmap sensor data
  //*sample_count = 965286;
  *sample_count = 6167;

  // Read in input data
  input_state->position.accel = malloc(sizeof(Vect));
  Vect *accel = input_state->position.accel;
  fd = open("../data/raw_data/0_AccelX", O_RDONLY, 0);
  accel->x = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_AccelY", O_RDONLY, 0);
  accel->y = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_AccelZ", O_RDONLY, 0);
  accel->z = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  accel->data_len = *sample_count;

  input_state->orientation.rot_vel = malloc(sizeof(Vect));
  Vect *gyro = input_state->orientation.rot_vel;
  fd = open("../data/raw_data/0_GyroX", O_RDONLY, 0);
  gyro->x = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_GyroY", O_RDONLY, 0);
  gyro->y = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_GyroZ", O_RDONLY, 0);
  gyro->z = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  gyro->data_len = *sample_count;

  //input_state->orientation.orient = malloc(sizeof(Vect));
  //Vect *mag = input_state->orientation.orient;
  //fd = open("../data/raw_data/0_MagX", O_RDONLY, 0);
  //mag->x = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  //close(fd);
  //fd = open("../data/raw_data/0_MagY", O_RDONLY, 0);
  //mag->y = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  //close(fd);
  //fd = open("../data/raw_data/0_MagZ", O_RDONLY, 0);
  //mag->z = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  //close(fd);
  //mag->data_len = *sample_count;
  return 0;
}

int main() {
  ObjectState input_state;
  int sample_count;

  load_sensor_data(&input_state, &sample_count);

  for (int order=0; order<=6; order++) {
    for (int history=order; history<7; history++) {
      int best_k = 0;
      double best_cr = 0;
      for (int k=6; k<14; k++) {
        params.rice_k = k;
        params.block_size = BLOCK_SIZE_MAX;
        params.prediction_strategy = PREDICTION_DIFF_ENCODING;

        build_stream_fir(order, history, &input_state);

        for (int i=0; i < sample_count; i++) {
          //printf("%d ", accel.x[i]);
        }

        // Initialize output data
        Vect accel_out = {};
        Vect gyro_out = {};
        //Vect mag_out = {};
        accel_out.x = calloc(1, sizeof(uint16_t) * sample_count);
        accel_out.y = calloc(1, sizeof(uint16_t) * sample_count);
        accel_out.z = calloc(1, sizeof(uint16_t) * sample_count);
        gyro_out.x = calloc(1, sizeof(uint16_t) * sample_count);
        gyro_out.y = calloc(1, sizeof(uint16_t) * sample_count);
        gyro_out.z = calloc(1, sizeof(uint16_t) * sample_count);
        //mag_out.x = calloc(1, sizeof(uint16_t) * sample_count);
        //mag_out.y = calloc(1, sizeof(uint16_t) * sample_count);
        //mag_out.z = calloc(1, sizeof(uint16_t) * sample_count);
        Position output_pos = {NULL, NULL, &accel_out};
        Orientation output_orient = {NULL, &gyro_out, NULL};
        ObjectState output_state = {};
        output_state.position = output_pos;
        output_state.orientation = output_orient;
        build_stream_fir(order, history, &output_state);


        // @TODO(David): how do I know this is big enough?!
        uint8_t *data_out = calloc(1, sample_count * 9 * 2 * 1000); // worse than .1 compression ratio will seg fault
        CompressedDataWriter data_writer = {};
        data_writer.data_out = data_out;


        encode_data(&input_state, &data_writer);
        uint32_t block_len = data_writer.bit_pointer >> 3;


        // Decode the data we just compressed to check for errors
        CompressedDataReader data_reader = {};
        data_reader.data_in = data_out;

        decode_data(&data_reader, &output_state, sample_count);
        check_errors(&input_state, &output_state);

        //printf("RICE K: %d\n", params.rice_k);
        //printf("Block size: %d\n", params.block_size);
        //printf("Output byte len: %d\n", (int)block_len);
        //printf("Compression ratio: %f\n", ((double)(sample_count*2*9)/block_len));
        double cr =  ((double)(sample_count*2*6)/block_len);
        if (cr > best_cr) {
          best_k = k;
          best_cr = cr;
        }
        if (data_out)
          free(data_out);
      } // k
      printf("order: %d, history: %d, k: %d, CR: %f\n", order, history, best_k, best_cr);
      if (order == 0) history += 100;
    } // history
  } // order

  return 0;
} // main
