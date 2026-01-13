#ifndef __CURL_WRAPPERS__
#define __CURL_WRAPPERS__

#include <stddef.h>
#include <curl/curl.h>

typedef struct MemoryStruct {
  char    *Memory;
  size_t  Size;
} MemoryStruct;

void      SetupHandle();
void      DestroyHandle();
CURLcode  CurlGet(MemoryStruct *Chunk, char *WithURL);

#endif
