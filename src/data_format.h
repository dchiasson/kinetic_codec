#ifndef __data_format_h__
#define __data_format_h__
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
  int16_t *pointers[18];
  int pointer_count;
  FIRFilterState fir_state[18];
  CrossFIRFilterState cross_fir_state[18];
  int filter_type;
}StreamFir;

typedef struct{
  Position position;
  Orientation orientation;
  StreamFir stream_fir; // @TODO(David): we can make this a void pointer to make it more flexible
}ObjectState;

typedef struct{
  uint8_t *data_out;
  uint64_t bit_pointer;
  //ObjectState input_state = {};
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
#endif // __data_format_h__
