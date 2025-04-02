#ifndef __PAPAUDIO__
#define __PAPAUDIO__

#define PAP_MAX_SONGS 256

#ifndef WINDOWS
#include <linux/limits.h>
#else
#include <windows.h>
#endif

extern double MusicDuration, CurrentPosition;
extern int AudioVolume;
extern char SongTitles[PAP_MAX_SONGS][256];
extern char Songs[PAP_MAX_SONGS][PATH_MAX];
extern const char *TitleTag;
extern const char *AlbumTag;
extern const char *TagArtist;
extern const char *TagCopyright;

void UpdateSongPosition();
void InitializeAudio();
int AddSong(char *Path);
double PlayAudio(char *Path);

#endif
