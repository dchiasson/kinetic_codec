/* Command line tool
 *
 */

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "stream_encoder.h"


int main(int argc, char *argv[])
{
	int opt;
  int rice_order, tech;
  char filter_loc[200];
  char data_loc[200];
	while ((opt = getopt(argc, argv, "t:hk:f:")) != -1) {
		switch (opt) {
			case 't':
      {
        if (strcmp(optarg, "auto-homo") == 0) {
          tech = TECH__AUTOHOMO;
        } else if (strcmp(optarg, "auto-hetero") == 0) {
          tech = TECH__AUTOHETERO;
        } else if (strcmp(optarg, "cross-homo") == 0) {
          tech = TECH__CROSSHOMO;
        } else if (strcmp(optarg, "cross-hetero") == 0) {
          tech = TECH__CROSSHETERO;
        } else {
				  fprintf(stderr, "Unexpected technique name: %s\n", optarg);
          return 1;
        }
				break;
      }
			case 'k':
        rice_order = atoi(optarg);
				break;
			case 'f':
        strcpy(filter_loc, optarg);
				break;
			case 'h':
			default:
				fprintf(stderr, "Usage: %s [-t technique] [-k rice order] [-f filter_location] data_location\n", argv[0]);
				fprintf(stderr, "unexpected input %c\n", opt);
				break;
		}
	}
  if (optind >= argc) {
		fprintf(stderr, "Need data location\n");
    return 1;
  }
  strcpy(data_loc, argv[optind]);
  return compress_main(data_loc, filter_loc, tech, rice_order);
}
