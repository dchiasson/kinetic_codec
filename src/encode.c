#include "encode.h"

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


/// @TODO(David): Use the FLAC functions, they are way more efficient
void rice_encode(int32_t res, CompressedDataWriter *data_writer, uint8_t rice_k) {
  uint16_t residual;
  if (res < 0) // negative is odd
    residual = -res*2-1;
  else // positive is even
    residual = res*2;
  //printf("%lu : %d \n", data_writer->bit_pointer, residual);
  // exponent
  data_writer->bit_pointer += residual >> rice_k; // faster than put_bit
  // one termination
  put_bit(1, data_writer);
  // remainder
  for (int i=0; i < rice_k; i++) {
    put_bit((residual >> i) & 1, data_writer);
  }
}

int32_t rice_decode(CompressedDataReader *data_reader, uint8_t rice_k) {
  uint16_t res = 0;
  while (!get_bit(data_reader)) {
    res++;
  }
  res <<= rice_k;
  for (int i=0; i < rice_k; i++) {
    res += (get_bit(data_reader) << i);
  }
  if (res & 1) { // odd is negative
    return -(res + 1) / 2;
  } else { // even is positive
    return res / 2;
  }
}

