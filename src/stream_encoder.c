#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "fir_predictive_filter.h"
#include "stream_encoder.h"
#include "data_format.h"
#include "encode.h"

#define BLOCK_SIZE_MAX (-1)
#define PREDICTION_DIFF_ENCODING 1
Parameters params;

int file_exists(const char *file_name) {
  if (access(file_name, F_OK) != -1) return 1;
  else return 0;
}

int build_stream_fir(char *filter_loc, int technique, ObjectState *object_state) {

  // 1) Connect Sensor Data Streams into our FIR object

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
  if (!file_exists(filter_loc)) {
    fprintf(stderr, "Filter coefficient file not found!\n");
    return 1;
  }

  // 2) Load filter coeficients from file

  int fd;
  fd = open(filter_loc, O_RDONLY, 0);
  FILE* fp = fdopen(fd, "r");
  fseek(fp, 0L, SEEK_END);
  int coeff_count = ftell(fp) / sizeof(int32_t);
  fseek(fp, 0L, SEEK_SET);
  int32_t *filter_coeffs = (int32_t*)mmap(NULL, sizeof(int32_t)*coeff_count, PROT_READ, MAP_SHARED, fd, 0);

  int history;
  int stream_count = 6;
  switch (technique) {
    case TECH__AUTOHOMO:
    {
      history = coeff_count;
      stream_fir->filter_type = FILTER_TYPE__AUTO_FIR;
      for (int i=0; i<stream_count; i++) {
        init_fir_filter(filter_coeffs, history, &stream_fir->fir_state[i]);
      }
      break;
    }
    case TECH__AUTOHETERO:
    {
      history = coeff_count / stream_count;
      if (history * stream_count != coeff_count) {
        fprintf(stderr, "Filter coefficients do not match technique requirement\n");
        return 1;
      }
      stream_fir->filter_type = FILTER_TYPE__AUTO_FIR;
      for (int i=0; i<stream_count; i++) { // input data stream
        init_fir_filter(&filter_coeffs[history*i], history, &stream_fir->fir_state[i]);
      }
      break;
    }
    case TECH__CROSSHOMO:
    {
      history = coeff_count / stream_count;
      if (history * stream_count != coeff_count) {
        fprintf(stderr, "Filter coefficients do not match technique requirement\n");
        return 1;
      }
      for (int i=0; i<stream_count; i++) { // input data stream
        for (int j=0; j<stream_count; j++) { // FIR stream index
          init_fir_filter(&filter_coeffs[history*j], history, &stream_fir->cross_fir_state[i].stream_state[j]);
        }
      }
      stream_fir->filter_type = FILTER_TYPE__CROSS_FIR;
      break;
    }
    case TECH__CROSSHETERO:
    {
      history = coeff_count / (stream_count * stream_count);
      if (history * stream_count * stream_count != coeff_count) {
        fprintf(stderr, "Filter coefficients do not match technique requirement\n");
        return 1;
      }
      for (int i=0; i<stream_count; i++) { // input data stream
        for (int j=0; j<stream_count; j++) { // FIR stream index
          init_fir_filter(&filter_coeffs[history*j+history*stream_count*i], history, &stream_fir->cross_fir_state[i].stream_state[j]);
        }
      }
      stream_fir->filter_type = FILTER_TYPE__CROSS_FIR;
      break;
    }
    default:
      fprintf(stderr, "Unexpected technique: %d\n", technique);
      return 1;
      break;
  }
  close(fd);

  stream_fir->history = history;

  // 3) Warmup Filters with initial values 


  return 0;
}


/// @TODO(David): support block sizes
int encode_data(int max_output_size, ObjectState *input_state, CompressedDataWriter *data_writer) {

  // Store inital values raw
  for (int stream=0; stream < input_state->stream_fir.pointer_count; stream++) {
    for (int i=0; i < input_state->stream_fir.history; i++) {
      int16_t warmup = input_state->stream_fir.pointers[stream][i];
      // We can ignore the resudual since we are writing the raw value.
      // We just want to warmup the filter
      get_fir_residual(warmup, &input_state->stream_fir.fir_state[stream]);
      put_word(warmup, data_writer);
    }
  }

  uint32_t sample_count = input_state->position.accel->data_len;
  for (uint32_t sample = input_state->stream_fir.history; sample < sample_count; sample++) {
    if ((data_writer->bit_pointer >> 3) > 0.8 * max_output_size) {
       printf("Output buffer running out of space\n");
       return 1;
    }
    if (input_state->stream_fir.filter_type == FILTER_TYPE__AUTO_FIR) {
      for (int stream=0; stream < input_state->stream_fir.pointer_count; stream++) {
        int32_t res = get_fir_residual(
            input_state->stream_fir.pointers[stream][sample],
            &input_state->stream_fir.fir_state[stream]);
        if (stream == 0) {
          //printf("samp:%d st:%d val:%d res:%d\n", sample, stream, input_state->stream_fir.pointers[stream][sample],  res);
        }
        rice_encode(res, data_writer, (stream % 6 < 3) ? params.rice_k_accel : params.rice_k_gyro); // Hack to allow different K values
      }
    } else if (input_state->stream_fir.filter_type == FILTER_TYPE__CROSS_FIR) {
      for (int stream=0; stream < input_state->stream_fir.pointer_count; stream++) {
        int32_t res = get_cross_fir_residual(
            input_state->stream_fir.pointers[stream][sample],
            &input_state->stream_fir.cross_fir_state[stream]);
        rice_encode(res, data_writer, (stream % 6 < 3) ? params.rice_k_accel : params.rice_k_gyro); // Hack to allow different K values
      }
      for (int stream=0; stream < input_state->stream_fir.pointer_count; stream++) {
        update_cross_fir_filters(
            input_state->stream_fir.pointers[stream][sample],
            &input_state->stream_fir.cross_fir_state[stream],
            stream);
      }
    } else {
      printf("ERROR unknown filter type: %d", input_state->stream_fir.filter_type);
    } // filter type
  } // for time sample
  return 0;
}
/// @TODO(David): if we are decoding blocks, we need to make sure to reset FIR history to recover
// from dropped packets
int decode_data(CompressedDataReader *data_reader, ObjectState *output_state, uint32_t sample_count) {

  // Read inital values raw
  for (int stream=0; stream < output_state->stream_fir.pointer_count; stream++) {
    for (int i=0; i < output_state->stream_fir.history; i++) {
      int16_t warmup = get_word(data_reader);
      output_state->stream_fir.pointers[stream][i] = warmup;
      // We can ignore the resudual since we are writing the raw value.
      // We just want to warmup the filter
      get_fir_residual(warmup, &output_state->stream_fir.fir_state[stream]);
    }
  }

  for (uint32_t sample = output_state->stream_fir.history; sample < sample_count; sample++) {
    if (output_state->stream_fir.filter_type == FILTER_TYPE__AUTO_FIR) {
      for (int stream=0; stream < output_state->stream_fir.pointer_count; stream++) {
        uint8_t rice_k = (stream % 6 < 3) ? params.rice_k_accel : params.rice_k_gyro; // Hack to allow different K values
        output_state->stream_fir.pointers[stream][sample] = 
          decode_fir_residual(rice_decode(data_reader, rice_k), &output_state->stream_fir.fir_state[stream]);

      }
    } else if (output_state->stream_fir.filter_type == FILTER_TYPE__CROSS_FIR) {
      for (int stream=0; stream < output_state->stream_fir.pointer_count; stream++) {
        uint8_t rice_k = (stream % 6 < 3) ? params.rice_k_accel : params.rice_k_gyro; // Hack to allow different K values
        output_state->stream_fir.pointers[stream][sample] =
          decode_cross_fir_residual(rice_decode(data_reader, rice_k), &output_state->stream_fir.cross_fir_state[stream]);
      }
      for (int stream=0; stream < output_state->stream_fir.pointer_count; stream++) {
        update_cross_fir_filters(
         output_state->stream_fir.pointers[stream][sample],
         &output_state->stream_fir.cross_fir_state[stream],
         stream);
      }
    } else {
      printf("ERROR unknown filter type: %d", output_state->stream_fir.filter_type);
    }// filter type
  } // for time sample
  return 0;
}

int check_errors(ObjectState *input_state, ObjectState *output_state) {
  uint32_t sample_count = input_state->position.accel->data_len;

  for (int p = 0; p < output_state->stream_fir.pointer_count; p++) {
    for (int s=0; s < (int)sample_count; s++) {
      if (output_state->stream_fir.pointers[p][s] != input_state->stream_fir.pointers[p][s]) {
        printf("ERROR p: %d s: %d\n",p,s);
        return 1;
      }
    }
  }
  return 0;
}

int load_sensor_data(const char *data_dir, ObjectState *input_state, int *sample_count) {
  /// @TODO(David): write a function to free an munmap sensor data
  int fd;
  char file_name[500];

  // Read in input data
  input_state->position.accel = malloc(sizeof(Vect));
  Vect *accel = input_state->position.accel;
  strcpy(file_name, data_dir);
  strcat(file_name, "acc_x");
  if (!file_exists(file_name)) {
    fprintf(stderr, "Sensor data files not found!\n");
    return 1;
  }
  fd = open(file_name, O_RDONLY, 0);
  FILE* fp = fdopen(fd, "r");
  fseek(fp, 0L, SEEK_END); // assume all streams are same length
  *sample_count = ftell(fp) / sizeof(int16_t);
  //printf("size %d\n", *sample_count);
  //fflush(stdout);
  fseek(fp, 0L, SEEK_SET);

  accel->x = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  strcpy(file_name, data_dir);
  strcat(file_name, "acc_y");
  fd = open(file_name, O_RDONLY, 0);
  accel->y = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  strcpy(file_name, data_dir);
  strcat(file_name, "acc_z");
  fd = open(file_name, O_RDONLY, 0);
  accel->z = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  accel->data_len = *sample_count;

  input_state->orientation.rot_vel = malloc(sizeof(Vect));
  Vect *gyro = input_state->orientation.rot_vel;
  strcpy(file_name, data_dir);
  strcat(file_name, "gyro_x");
  fd = open(file_name, O_RDONLY, 0);
  gyro->x = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  strcpy(file_name, data_dir);
  strcat(file_name, "gyro_y");
  fd = open(file_name, O_RDONLY, 0);
  gyro->y = (int16_t*)mmap(NULL, sizeof(int16_t)*(*sample_count), PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  strcpy(file_name, data_dir);
  strcat(file_name, "gyro_z");
  fd = open(file_name, O_RDONLY, 0);
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

int compress_main(char *data_folder, char *filter_loc, int technique, int rice_order) {
  ObjectState input_state;
  int sample_count;
  int ret=0;

  ret |= load_sensor_data(data_folder, &input_state, &sample_count);
  if (ret) return ret;

  params.rice_k_accel = rice_order;
  params.rice_k_gyro = rice_order-1;
  params.block_size = BLOCK_SIZE_MAX;
  params.prediction_strategy = technique;

  ret |= build_stream_fir(filter_loc, technique, &input_state);
  if (ret) return ret;

  ObjectState output_state = {};
  init_empty_object_state(&output_state, sample_count);
  ret |= build_stream_fir(filter_loc, technique, &output_state);
  if (ret) return ret;

  int max_output_size = sample_count * 6 * 2 * 200;
  uint8_t *data_out = calloc(1, max_output_size); // worse than .01 compression ratio will seg fault
  if (!data_out) {
    printf("calloc failed!\n");
    return 1;
  }
  CompressedDataWriter data_writer = {};
  data_writer.data_out = data_out;


  ret |= encode_data(max_output_size, &input_state, &data_writer);
  if (ret) return ret;
  uint32_t block_len = data_writer.bit_pointer >> 3;


  // Decode the data we just compressed to check for errors
  CompressedDataReader data_reader = {};
  data_reader.data_in = data_out;

  decode_data(&data_reader, &output_state, sample_count);
  if (check_errors(&input_state, &output_state)) {
    return 1;
  }

  double cr =  ((double)(sample_count*2*6)/block_len);
  printf("tech %d, k: %d, fin_size: %d, CR: %f, data: %s, filter: %s\n", technique, rice_order, block_len, cr, data_folder, filter_loc);

  if (data_out) {
    free(data_out);
    free(output_state.position.accel->x);
    free(output_state.position.accel->y);
    free(output_state.position.accel->z);
    free(output_state.orientation.rot_vel->x);
    free(output_state.orientation.rot_vel->y);
    free(output_state.orientation.rot_vel->z);
  }
  return 0;
}
