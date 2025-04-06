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
} AudioData;

extern AudioData Audio[];
extern double AudioDuration, AudioPosition;
extern int AudioVolume;
extern int AudioCurrentIndex;
extern const char *TitleTag;
extern const char *AlbumTag;
extern const char *TagArtist;
extern const char *TagCopyright;

void UpdateAudioPosition();
void InitializeAudio();
int AddAudio(char *Path);
double PlayAudio(char *Path);

#endif
