#ifndef __stream_encoder_h__
#define __stream_encoder_h__

#define TECH__AUTOHOMO 1
#define TECH__AUTOHETERO 2
#define TECH__CROSSHOMO 3
#define TECH__CROSSHETERO 4

int compress_main(char *data_folder, char *filter_loc, int technique, int rice_order);

#endif // __stream_encoder_h__
