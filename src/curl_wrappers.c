#include <SDL3/SDL_log.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "curl_wrappers.h"

CURL *CurlHandle;

size_t WriteMemoryCallback(void *Contents, size_t Size, size_t nmemb, void *userp) {
  size_t RequiredSize = Size * nmemb;
  MemoryStruct *Memory = (MemoryStruct *)userp;

  char *Pointer = realloc(Memory->Memory, Memory->Size + RequiredSize + 1);

  if (Pointer == NULL) {
    printf("WirteMemoryCallback: not enough memory.\n");
    return 0;
  }

  Memory->Memory = Pointer;
  memcpy(&(Memory->Memory[Memory->Size]), Contents, RequiredSize);
  Memory->Size += RequiredSize;
  Memory->Memory[Memory->Size] = 0;

  return RequiredSize;
}

void SetupHandle() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  CurlHandle = curl_easy_init();

  curl_easy_setopt(CurlHandle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(CurlHandle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(CurlHandle, CURLOPT_USERAGENT, "SonataAudioAgent");
  curl_easy_setopt(CurlHandle, CURLOPT_SSL_VERIFYPEER, 0L);
}

void DestroyHandle() {
  curl_easy_cleanup(CurlHandle);
  curl_global_cleanup();
}

CURLcode CurlGet(MemoryStruct *Chunk, char *WithURL) {
  curl_easy_setopt(CurlHandle, CURLOPT_URL, WithURL);
  curl_easy_setopt(CurlHandle, CURLOPT_WRITEDATA, (void *)Chunk);
  return curl_easy_perform(CurlHandle);
}
