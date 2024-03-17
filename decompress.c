// Decompress into memory
#include "unlzexe.h"

#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

FILE* make_temp_file(void)
{
	char opath[FILENAME_MAX];
	strncpy(opath, "decompressXXXXXX", strlen("decompressXXXXXX") + 1);
#ifdef _MSC_VER
	char* filename = _mktemp(opath);
	return fopen(filename, "w+b");
#else
	int fd = mkstemp(opath);
	if (fd == -1) {
		printf("can't create temp file.\n");
		return NULL;
	}
	// Reopen in binary mode to be able to fseek
	close(fd);
	return fopen(opath, "w+b");
#endif
}

char* decompress(const char* ipath, size_t* len) {
	FILE* ifile = NULL;
	FILE* ofile = NULL;
	char* buf = NULL;
	int ver;

	if ((ifile = fopen(ipath, "rb")) == NULL) {
		printf("'%s' :not found\n", ipath);
		goto bail;
	}

	if (rdhead(ifile, &ver) != SUCCESS) {
		printf("'%s' is not LZEXE file.\n", ipath);
		goto bail;
	}
	printf("file '%s' is compressed by LZEXE Ver. 0.%d\n", ipath, ver);

	ofile = make_temp_file();
	if (ofile == NULL) {
		printf("can't create temp file.\n");
		goto bail;
	}

	if (mkreltbl(ifile, ofile, ver) != SUCCESS) {
		goto bail;
	}
	if (unpack(ifile, ofile) != SUCCESS) {
		goto bail;
	}
	fclose(ifile);
	wrhead(ofile);

	// Read decompressed file into memory
	if (fseek(ofile, 0, SEEK_END) != 0) {
		printf("can't seek.\n");
		goto bail;
	}
	*len = ftell(ofile);
	if (fseek(ofile, 0, SEEK_SET) != 0) {
		printf("can't seek.\n");
		goto bail;
	}
	buf = malloc(*len);
	if (buf == NULL) {
		printf("can't allocate %d bytes\n", (int)*len);
		goto bail;
	}
	if (fread(buf, 1, *len, ofile) != *len) {
		if (feof(ofile))
			printf("Error reading output file: unexpected end of file\n");
		else if (ferror(ofile))
			perror("Error reading output file");
		goto bail;
	}
	printf("Read %d bytes\n", (int)*len);

	fclose(ofile);

	return buf;

bail:
	if (ifile) {
		fclose(ifile);
	}
	if (ofile) {
		fclose(ofile);
	}
	free(buf);
	*len = 0;
	return NULL;
}
