#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#define VERSION "0.9"
#define FAILURE 1
#define SUCCESS 0

int rdhead(FILE *ifile ,int *ver);
int mkreltbl(FILE *ifile,FILE *ofile,int ver);
int unpack(FILE *ifile,FILE *ofile);
void wrhead(FILE *ofile);

#ifdef __cplusplus
}
#endif
