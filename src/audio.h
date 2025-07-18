#ifndef __PAPAUDIO__
#define __PAPAUDIO__

#include <stdint.h>

#ifndef WINDOWS
#include <linux/limits.h>
#else
#include <windows.h>
#endif

typedef struct {
  char Title[128];
  char Path[PATH_MAX];
  
  char TagArtist[128];
  char TagCopyright[128];
  char TagAlbum[128];

  char AssignedList[128];

  uint32_t LayoutOrder;
} AudioData;

extern AudioData *Audio;
extern bool PausedMusic;
extern double AudioDuration, AudioPosition;
extern int32_t AudioVolume, AudioCurrentIndex;
extern uint32_t SA_TotalAudio; 

void AudioRemove(uint32_t Index);
void UpdateAudioPosition();
void InitializeAudio();
int32_t AddAudio(char *Path, char *Category);
int8_t PlayAudio(char *Path);

#endif
