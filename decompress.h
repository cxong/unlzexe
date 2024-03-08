#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

char *decompress(const char *ipath, size_t *len);

#ifdef __cplusplus
}
#endif
