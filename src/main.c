/* Command line tool
 *
 */

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	int opt;
	while (opt = getopt(argc, argv, "h") != -1) {
		switch (opt) {
			case 'h':
				fprintf(stderr, "Help message\n");
				break;
			default:
				fprintf(stderr, "no input\n");
				break;
		}
	}
}
