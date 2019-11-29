#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "fir_predictive_filter.h"
#include "data_format.h"
#include "encode.h"

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
    if(orientation->orient) {
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->orient->x;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->orient->y;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->orient->z;
    }
    if(orientation->rot_vel) {
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_vel->x;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_vel->y;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_vel->z;
    }
    if(orientation->rot_accel) {
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_accel->x;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_accel->y;
      stream_fir->pointers[stream_fir->pointer_count++] = orientation->rot_accel->z;
    }
  }
  int fd;
  char buffer[50];
  ///@TODO(David): Check that this file exists first!!
  //sprintf(buffer, "../data/fixed_poly_pred/%d_deg_poly_reg/%d", order, history);
  sprintf(buffer, "../data/natural_spline_pred/condition%d/%d", order, history);
  fd = open(buffer, O_RDONLY, 0);
  int32_t *poly_reg = (int32_t*)mmap(NULL, sizeof(int32_t)*history, PROT_READ, MAP_SHARED, fd, 0);
  for (int i=0; i<18; i++) {
    init_fir_filter(poly_reg, history, &stream_fir->fir_state[i]);
  }
  close(fd);
}


/// @TODO(David): support block sizes
int encode_data(ObjectState *input_state, CompressedDataWriter *data_writer) {
  uint32_t sample_count = input_state->position.accel->data_len;
  for (uint32_t sample=0; sample < sample_count; sample++) {
    for (int stream=0; stream < input_state->stream_fir.pointer_count; stream++) {
      int32_t res = get_fir_residual(input_state->stream_fir.pointers[stream][sample], &input_state->stream_fir.fir_state[stream]);
      rice_encode(res, data_writer, params);
    }
  }
  return 0;
}
/// @TODO(David): if we are decoding blocks, we need to make sure to reset FIR history to recover
// from dropped packets
int decode_data(CompressedDataReader *data_reader, ObjectState *output_state, uint32_t sample_count) {
  for (uint32_t sample=0; sample < sample_count; sample++) {
    for (int stream=0; stream < output_state->stream_fir.pointer_count; stream++) {
      output_state->stream_fir.pointers[stream][sample] = decode_fir_residual(rice_decode(data_reader, params), &output_state->stream_fir.fir_state[stream]);
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
  //*sample_count = 6167;
  *sample_count = 2471;

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

int init_empty_object_state(ObjectState *object_state, int sample_count) {
  // Initialize output data
  object_state->position = (Position){NULL, NULL, malloc(sizeof(Vect))};
  object_state->orientation = (Orientation){NULL, malloc(sizeof(Vect)), NULL};

  Vect *accel_out = object_state->position.accel;
  Vect *gyro_out = object_state->orientation.rot_vel;

  accel_out->x = calloc(1, sizeof(uint16_t) * sample_count);
  accel_out->y = calloc(1, sizeof(uint16_t) * sample_count);
  accel_out->z = calloc(1, sizeof(uint16_t) * sample_count);
  gyro_out->x = calloc(1, sizeof(uint16_t) * sample_count);
  gyro_out->y = calloc(1, sizeof(uint16_t) * sample_count);
  gyro_out->z = calloc(1, sizeof(uint16_t) * sample_count);
  return 0;
}

int main() {
  ObjectState input_state;
  int sample_count;

  load_sensor_data(&input_state, &sample_count);

  for (int order=0; order<=4; order++) {
    for (int history=2; history<10; history++) {
      int best_k = 0;
      double best_cr = 0;
      for (int k=4; k<14; k++) {
        params.rice_k = k;
        params.block_size = BLOCK_SIZE_MAX;
        params.prediction_strategy = PREDICTION_DIFF_ENCODING;

        build_stream_fir(order, history, &input_state);

        ObjectState output_state = {};
        init_empty_object_state(&output_state, sample_count);
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

        double cr =  ((double)(sample_count*2*6)/block_len);
        if (cr > best_cr) {
          best_k = k;
          best_cr = cr;
        }
        if (data_out)
          free(data_out);
      } // k
      printf("order: %d, history: %d, k: %d, CR: %f\n", order, history, best_k, best_cr);
      //if (order == 0) history += 100;
    } // history
  } // order

  return 0;
} // main
