#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>

#define RICE_K (4)

typedef struct{
  int16_t *x;
  int16_t *y;
  int16_t *z;
}Vect;

void put_bit(uint8_t bit, uint8_t *data, uint64_t *bit_pointer)
{
  data[*bit_pointer >> 3] |= (bit & 1) << (*bit_pointer & 7);
  *bit_pointer++;
}

void rice_encode(int32_t res, uint8_t *data_out, uint64_t *bit_pointer) {
  uint16_t residual;
  if (res < 0)
    residual = -res*2-1;
  else
    residual = res*2;
  //printf("%lu : %d \n", *bit_pointer, residual);
  // exponent
  *bit_pointer += residual >> RICE_K;
  // one termination
  data_out[*bit_pointer >> 3] |= 1 << (*bit_pointer & 7);
  (*bit_pointer)++;
  // remainder
  for (int i=0; i < RICE_K; i++) {
    data_out[*bit_pointer >> 3] |= ((residual >> i) & 1) << (*bit_pointer & 7);
	(*bit_pointer)++;
  }
}

int encode_block(Vect accel, Vect gyro, Vect mag, uint32_t sample_count, uint8_t *data_out, uint32_t *block_len) {

  uint8_t i = 0;
  // Encode starting values
  data_out[i++] = accel.x[0] >> 8 & 0xff;
  data_out[i++] = accel.x[0] & 0xff;
  data_out[i++] = accel.y[0] >> 8 & 0xff;
  data_out[i++] = accel.y[0] & 0xff;
  data_out[i++] = accel.z[0] >> 8 & 0xff;
  data_out[i++] = accel.z[0] & 0xff;

  data_out[i++] = gyro.x[0] >> 8 & 0xff;
  data_out[i++] = gyro.x[0] & 0xff;
  data_out[i++] = gyro.y[0] >> 8 & 0xff;
  data_out[i++] = gyro.y[0] & 0xff;
  data_out[i++] = gyro.z[0] >> 8 & 0xff;
  data_out[i++] = gyro.z[0] & 0xff;

  data_out[i++] = mag.x[0] >> 8 & 0xff;
  data_out[i++] = mag.x[0] & 0xff;
  data_out[i++] = mag.y[0] >> 8 & 0xff;
  data_out[i++] = mag.y[0] & 0xff;
  data_out[i++] = mag.z[0] >> 8 & 0xff;
  data_out[i++] = mag.z[0] & 0xff;
  long res;
  uint64_t bit_pointer = i*8;
  for (int s=1; s < sample_count; s++) {
    res = gyro.x[s] - gyro.x[s-1];
    rice_encode(res, data_out, &bit_pointer);
    res = gyro.y[s] - gyro.y[s-1];
    rice_encode(res, data_out, &bit_pointer);
    res = gyro.z[s] - gyro.z[s-1];
    rice_encode(res, data_out, &bit_pointer);

    res = mag.x[s] - mag.x[s-1];// - gyro.x[s-1];
    rice_encode(res, data_out, &bit_pointer);
    res = mag.y[s] - mag.y[s-1];// - gyro.y[s-1];
    rice_encode(res, data_out, &bit_pointer);
    res = mag.z[s] - mag.z[s-1];// - gyro.z[s-1];
    rice_encode(res, data_out, &bit_pointer);

    res = accel.x[s] - accel.x[s-1];
    rice_encode(res, data_out, &bit_pointer);
    res = accel.y[s] - accel.y[s-1];
    rice_encode(res, data_out, &bit_pointer);
    res = accel.z[s] - accel.z[s-1];
    rice_encode(res, data_out, &bit_pointer);

  }
  *block_len = bit_pointer >> 3;
  return 0;
}

void main() {
  int sample_count = 965286;
  Vect accel;
  int fd = open("../data/raw_data/0_AccelX", O_RDONLY, 0);
  accel.x = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  fd = open("../data/raw_data/0_AccelY", O_RDONLY, 0);
  accel.y = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  fd = open("../data/raw_data/0_AccelZ", O_RDONLY, 0);
  accel.z = (int16_t*)mmap(NULL, sizeof(int16_t)*sample_count, PROT_READ, MAP_SHARED, fd, 0);
  for (int i=0; i < sample_count; i++) {
    //printf("%d ", accel.x[i]);
  }
  printf("\n");
  uint8_t data_out[sample_count*100];
  uint32_t block_len;
  encode_block(accel, accel, accel, sample_count, data_out, &block_len);
  printf("Output byte len: %d\n", block_len);
  printf("Compression ratio: %f\n", 1.0 - ((double)block_len/(sample_count*2*9)));
}
