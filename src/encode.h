#ifndef __encode_h__
#define __encode_h__
#include <stdint.h>
#include "data_format.h"

/// @TODO(David): Use the FLAC functions, they are way more efficient
void put_bit(uint8_t bit, CompressedDataWriter *data_write);
uint8_t get_bit(CompressedDataReader *data_reader);
void put_word(uint16_t word, CompressedDataWriter *data_writer);
uint16_t get_word(CompressedDataReader *data_reader);
void rice_encode(int32_t res, CompressedDataWriter *data_writer, uint8_t rice_k);
int32_t rice_decode(CompressedDataReader *data_reader, uint8_t rice_k);

#endif
