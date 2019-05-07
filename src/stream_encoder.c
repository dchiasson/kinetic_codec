#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

typedef struct{
  int16_t *x;
  int16_t *y;
  int16_t *z;
}Vect;

typedef struct {
  uint8_t *data_out;
  uint64_t bit_pointer;
}DataWriter;

typedef struct {
  uint8_t *data_in;
  uint64_t bit_pointer;
}DataReader;

typedef struct {
  uint8_t rice_k;
}Parameters;
Parameters params;

void put_bit(uint8_t bit, DataWriter *data_writer)
{
  data_writer->data_out[data_writer->bit_pointer >> 3] |= (bit & 1) << (data_writer->bit_pointer & 7);
  (data_writer->bit_pointer)++;
}

uint8_t get_bit(DataReader *data_reader) {
  uint8_t bit;
  bit = (data_reader->data_in[data_reader->bit_pointer >> 3] >> (data_reader->bit_pointer & 7)) & 1;
  data_reader->bit_pointer++;
  return bit;
}

void put_word(uint16_t word, DataWriter *data_writer) {
  // Big endian
  data_writer->data_out[data_writer->bit_pointer >> 3] = word >> 8 & 0xff;
  data_writer->bit_pointer += 8;
  data_writer->data_out[data_writer->bit_pointer >> 3] = word & 0xff;
  data_writer->bit_pointer += 8;
}

uint16_t get_word(DataReader *data_reader) {
  // Big endian
  /// @TODO(David): assumed to be at byte boundary
  uint16_t val = 0;
  val |= data_reader->data_in[data_reader->bit_pointer >> 3] << 8;
  data_reader->bit_pointer += 8;
  val |= data_reader->data_in[data_reader->bit_pointer >> 3];
  data_reader->bit_pointer += 8;
  return val;
}

void put_vector_0(Vect *vect, DataWriter *data_writer) {
    put_word(vect->x[0], data_writer);
    put_word(vect->y[0], data_writer);
    put_word(vect->z[0], data_writer);
}

/// @TODO(David): Use the FLAC functions, they are way more efficient
void rice_encode(int32_t res, DataWriter *data_writer) {
  uint16_t residual;
  if (res < 0)
    residual = -res*2-1;
  else
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

int32_t rice_decode(DataReader *data_reader) {
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

int encode_block(Vect accel, Vect gyro, Vect mag, uint32_t sample_count, DataWriter *data_writer, uint32_t *block_len) {

  // place the first value uncompressed
  put_vector_0(&accel, data_writer);
  put_vector_0(&gyro, data_writer);
  put_vector_0(&mag, data_writer);

  long res;
  for (unsigned int s=1; s < sample_count; s++) {
    res = accel.x[s] - accel.x[s-1];
    rice_encode(res, data_writer);
    res = accel.y[s] - accel.y[s-1];
    rice_encode(res, data_writer);
    res = accel.z[s] - accel.z[s-1];
    rice_encode(res, data_writer);

    res = gyro.x[s] - gyro.x[s-1];
    rice_encode(res, data_writer);
    res = gyro.y[s] - gyro.y[s-1];
    rice_encode(res, data_writer);
    res = gyro.z[s] - gyro.z[s-1];
    rice_encode(res, data_writer);

    res = mag.x[s] - mag.x[s-1];// - gyro.x[s-1];
    rice_encode(res, data_writer);
    res = mag.y[s] - mag.y[s-1];// - gyro.y[s-1];
    rice_encode(res, data_writer);
    res = mag.z[s] - mag.z[s-1];// - gyro.z[s-1];
    rice_encode(res, data_writer);

  }
  *block_len = data_writer->bit_pointer >> 3;
  return 0;
}

int decode_block(DataReader *data_reader, Vect *accel, Vect *gyro, Vect *mag, uint32_t sample_count) {
  accel->x[0] = get_word(data_reader);
  accel->y[0] = get_word(data_reader);
  accel->z[0] = get_word(data_reader);

  gyro->x[0] = get_word(data_reader);
  gyro->y[0] = get_word(data_reader);
  gyro->z[0] = get_word(data_reader);

  mag->x[0] = get_word(data_reader);
  mag->y[0] = get_word(data_reader);
  mag->z[0] = get_word(data_reader);

  for (unsigned int s=1; s < sample_count; s++) {
    accel->x[s] = accel->x[s-1] + rice_decode(data_reader);
    accel->y[s] = accel->y[s-1] + rice_decode(data_reader);
    accel->z[s] = accel->z[s-1] + rice_decode(data_reader);

    gyro->x[s] = gyro->x[s-1] + rice_decode(data_reader);
    gyro->y[s] = gyro->y[s-1] + rice_decode(data_reader);
    gyro->z[s] = gyro->z[s-1] + rice_decode(data_reader);

    mag->x[s] = mag->x[s-1] + rice_decode(data_reader);
    mag->y[s] = mag->y[s-1] + rice_decode(data_reader);
    mag->z[s] = mag->z[s-1] + rice_decode(data_reader);
  }
  return 0;
}

int check_errors(Vect *accel_in, Vect *accel_out, Vect *gyro_in, Vect *gyro_out, Vect *mag_in, Vect *mag_out, uint32_t sample_count) {
  for (unsigned int s=0; s < sample_count; s++) {
    if (accel_in->x[s] != accel_out->x[s] ||
        accel_in->y[s] != accel_out->y[s] ||
        accel_in->z[s] != accel_out->z[s] ||
        gyro_in->x[s] != gyro_out->x[s] ||
        gyro_in->y[s] != gyro_out->y[s] ||
        gyro_in->z[s] != gyro_out->z[s] ||
        mag_in->x[s] != mag_out->x[s] ||
        mag_in->y[s] != mag_out->y[s] ||
        mag_in->z[s] != mag_out->z[s]
		) {
      printf("ERROR @%d: %d => %d\n", s, accel_in->x[s], accel_out->x[s]);
	  exit(0);
	}
  }
  return 0;
}

int main() {
  int sample_count = 965286;
  Vect accel, mag, gyro;
  Vect accel_out, mag_out, gyro_out;
  accel_out.x = calloc(1, sizeof(uint16_t) * sample_count);
  accel_out.y = calloc(1, sizeof(uint16_t) * sample_count);
  accel_out.z = calloc(1, sizeof(uint16_t) * sample_count);
  gyro_out.x = calloc(1, sizeof(uint16_t) * sample_count);
  gyro_out.y = calloc(1, sizeof(uint16_t) * sample_count);
  gyro_out.z = calloc(1, sizeof(uint16_t) * sample_count);
  mag_out.x = calloc(1, sizeof(uint16_t) * sample_count);
  mag_out.y = calloc(1, sizeof(uint16_t) * sample_count);
  mag_out.z = calloc(1, sizeof(uint16_t) * sample_count);
  int fd;
  fd = open("../data/raw_data/0_AccelX", O_RDONLY, 0);
  accel.x = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_AccelY", O_RDONLY, 0);
  accel.y = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_AccelZ", O_RDONLY, 0);
  accel.z = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);

  fd = open("../data/raw_data/0_GyroX", O_RDONLY, 0);
  gyro.x = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_GyroY", O_RDONLY, 0);
  gyro.y = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_GyroZ", O_RDONLY, 0);
  gyro.z = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);

  fd = open("../data/raw_data/0_MagX", O_RDONLY, 0);
  mag.x = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_MagY", O_RDONLY, 0);
  mag.y = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  fd = open("../data/raw_data/0_MagZ", O_RDONLY, 0);
  mag.z = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);

  for (int i=0; i < sample_count; i++) {
    //printf("%d ", accel.x[i]);
  }
  printf("\n");
  // @TODO(David): how do I know this is big enough?!
  uint8_t *data_out = calloc(1, sample_count * 9 * 2 * 10); // worse than .1 compression ration will seg fault
  DataWriter data_writer;
  data_writer.data_out = data_out;
  data_writer.bit_pointer = 0;
  params.rice_k = 3;

  DataReader data_reader;
  data_reader.data_in = data_out;
  data_reader.bit_pointer = 0;
  uint32_t block_len = 0;
  encode_block(accel, gyro, mag, sample_count, &data_writer, &block_len);
  decode_block(&data_reader, &accel_out, &gyro_out, &mag_out, sample_count);
  check_errors(&accel, &accel_out, &gyro, &gyro_out, &mag, &mag_out, sample_count);

  printf("\nRICE: %d\n", params.rice_k);
  printf("Output byte len: %d\n", (int)block_len);
  printf("Compression ratio: %f\n", ((double)(sample_count*2*9)/block_len));

  if (data_out)
    free(data_out);
  return 0;
}
