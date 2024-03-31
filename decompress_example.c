// Example decompressing into memory only
#include "decompress.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("usage: decompress_example packedfile\n");
		return 1;
	}
	size_t len;
	char* buf = decompress(argv[1], &len);

	free(buf);
	return 0;
}
