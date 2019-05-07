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
  uint8_t rice_k;
}Parameters;
Parameters params;

void put_bit(uint8_t bit, DataWriter *data_writer)
{
  data_writer->data_out[data_writer->bit_pointer >> 3] |= (bit & 1) << (data_writer->bit_pointer & 7);
  (data_writer->bit_pointer)++;
}

void put_word(uint16_t word, DataWriter *data_writer) {
    // @TODO(David): make this platform independant
  data_writer->data_out[data_writer->bit_pointer >> 3] = word >> 8 & 0xff;
  data_writer->bit_pointer += 8;
  data_writer->data_out[data_writer->bit_pointer >> 3] = word & 0xff;
  data_writer->bit_pointer += 8;
}

void put_vector_0(Vect *vect, DataWriter *data_writer) {
    put_word(vect->x[0], data_writer);
    put_word(vect->y[0], data_writer);
    put_word(vect->z[0], data_writer);
}

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

int encode_block(Vect accel, Vect gyro, Vect mag, uint32_t sample_count, DataWriter *data_writer, uint32_t *block_len) {

  // place the first value uncompressed
  put_vector_0(&accel, data_writer);
  put_vector_0(&gyro, data_writer);
  put_vector_0(&mag, data_writer);

  long res;
  for (unsigned int s=1; s < sample_count; s++) {
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

    res = accel.x[s] - accel.x[s-1];
    rice_encode(res, data_writer);
    res = accel.y[s] - accel.y[s-1];
    rice_encode(res, data_writer);
    res = accel.z[s] - accel.z[s-1];
    rice_encode(res, data_writer);

  }
  *block_len = data_writer->bit_pointer >> 3;
  return 0;
}

int main() {
  int sample_count = 965286;
  Vect accel, mag, gyro;
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
  uint8_t *data_out = calloc(1, sizeof(uint8_t) * sample_count * 100);
  DataWriter data_writer;
  data_writer.data_out = data_out;

  for(params.rice_k=0; params.rice_k < 10; params.rice_k++) {
    uint32_t block_len = 0;
    data_writer.bit_pointer = 0;
    encode_block(accel, gyro, mag, sample_count, &data_writer, &block_len);
    printf("\nRICE: %d\n", params.rice_k);
    printf("Output byte len: %d\n", (int)block_len);
    printf("Compression ratio: %f\n", ((double)(sample_count*2*9)/block_len));
  }

  if (data_out)
    free(data_out);
  return 0;
}
