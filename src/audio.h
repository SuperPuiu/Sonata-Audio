#ifndef __PAPAUDIO__
#define __PAPAUDIO__

#define PAP_MAX_AUDIO 256

#ifndef WINDOWS
#include <linux/limits.h>
#else
#include <windows.h>
#endif

typedef struct {
  char Title[256];
  char Path[PATH_MAX];
  
  char TagArtist[128];
  char TagCopyright[128];
  char TagAlbum[128];
} AudioData;

extern AudioData Audio[];
extern double AudioDuration, AudioPosition;
extern int AudioVolume;
extern int AudioCurrentIndex;

void AudioRemove(int Index);
void UpdateAudioPosition();
void InitializeAudio();
int AddAudio(char *Path);
double PlayAudio(char *Path);

#endif
