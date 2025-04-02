#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef WINDOWS
#include <linux/limits.h>
#include <SDL3_mixer/SDL_mixer.h>
#else
#include <SDL3/SDL_mixer.h>
#endif

#include "gui.h"
#include "audio.h"

static SDL_AudioSpec Specifications = {
  .freq = MIX_DEFAULT_FREQUENCY,
  .format = MIX_DEFAULT_FORMAT,
  .channels = MIX_DEFAULT_CHANNELS
};

char SongTitles[PAP_MAX_SONGS][256];
char Songs[PAP_MAX_SONGS][PATH_MAX];
char *CurrentPath;

int CurrentIndex;
int AudioVolume = MIX_MAX_VOLUME;

static Mix_Music *Music;

double MusicDuration, CurrentPosition;
static double LoopStart, LoopEnd, LoopLength;

const char *TagTitle = NULL;
const char *TagArtist = NULL;
const char *TagAlbum = NULL;
const char *TagCopyright = NULL;

void InitializeAudio() {
  CurrentPosition = 0;
  MusicDuration = 0;

  if (!Mix_OpenAudio(0, &Specifications)) {
    SDL_Log("Couldn't open audio %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  } else {
    Mix_QuerySpec(&Specifications.freq, &Specifications.format, &Specifications.channels);
  }
  
  for (int i = 0; i < PAP_MAX_SONGS; i++)
    memset(Songs[i], 0, PATH_MAX);
  
  Mix_VolumeMusic(AudioVolume);
}

int GetEmptyIndex() {
  for (int i = 0; i < PAP_MAX_SONGS; i++)
    if (Songs[i][0] == 0)
      return i;
  return -1;
}

int GetNextIndex(int Index) {
  if (Index > PAP_MAX_SONGS || Index + 1 > PAP_MAX_SONGS)
    return 0;

  if (Songs[Index + 1][0] == 0)
    return 0;
  else
    return Index + 1;
}

int GetSongIndex(char *Path) {
  if (Path == NULL)
    return -1;

  for (int i = 0; i < PAP_MAX_SONGS; i++) {
    if (Songs[i][0] == 0)
      continue;

    if (strcmp(Songs[i], Path) == 0)
      return i;
  }

  return -1;
}

int AddSong(char *Path) {
  if (GetSongIndex(Path) != -1)
    return -1;

  int Index = GetEmptyIndex();
  Mix_Music *Music;
  char LocalTagTitle[256];

  if (Index == -1)
    return -1;

  Music = Mix_LoadMUS(Path);

  if (!Music) {
    SDL_Log("Failed to load %s: %s", Path, SDL_GetError());
    return -1;
  }
  
  if (Mix_GetMusicTitle(Music)[0] != '\0') {
    char *Local = (char*)Mix_GetMusicTitle(Music);

    memset(LocalTagTitle, 0, sizeof(LocalTagTitle));
    memcpy(LocalTagTitle, Local, strlen(Local));
  } else {
    SDL_Log("WARNING: LocalTagTitle is empty.");
    memset(LocalTagTitle, 0, sizeof(LocalTagTitle)); 

    char *LocalPath = Path;
    char *LastPathPointer;

    while (*(LocalPath += strspn(LocalPath, "/")) != '\0') {
      size_t Length = strcspn(LocalPath, "/");
      LastPathPointer = LocalPath;
      LocalPath += Length;
    }
    
    memcpy(LocalTagTitle, LastPathPointer, strlen(LastPathPointer));
  }

  memcpy(SongTitles[Index], LocalTagTitle, strlen(LocalTagTitle));
  memcpy(Songs[Index], Path, strlen(Path));

  Mix_FreeMusic(Music);
  return Index;
}

void UpdateSongPosition() {
  if (Mix_PlayingMusic())
    CurrentPosition = Mix_GetMusicPosition(Music);

  if (!Mix_PlayingMusic()) {
    if (LoopStatus == LOOP_SONG) {
      if (GetSongIndex(CurrentPath) != -1)
        PlayAudio(CurrentPath);
    } else if (LoopStatus == LOOP_ALL) {
      if (GetSongIndex(CurrentPath) != -1)
        PlayAudio(Songs[GetNextIndex(CurrentIndex)]);
    }
  }
}

double PlayAudio(char *Path) {
  int Index = GetSongIndex(Path);

  if (Index == -1)
    Index = AddSong(Path);
  
  if (Music != NULL) {
    Mix_FreeMusic(Music);
    Music = NULL;
  }

  Music = Mix_LoadMUS(Path);
  
  if (Music) {
    CurrentIndex = Index;
    CurrentPath = Path;

    MusicDuration = Mix_MusicDuration(Music);
    CurrentPosition = 0;
    AudioPosition = 0;

    TagTitle = Mix_GetMusicTitle(Music);
    TagArtist = Mix_GetMusicArtistTag(Music);
    TagCopyright = Mix_GetMusicCopyrightTag(Music);
    TagAlbum = Mix_GetMusicAlbumTag(Music);

    LoopStart = Mix_GetMusicLoopStartTime(Music);
    LoopEnd = Mix_GetMusicLoopEndTime(Music);
    LoopLength = Mix_GetMusicLoopLengthTime(Music);
    
    Mix_FadeInMusic(Music, false, 1000);
    printf("New index: %i Path: %s Duration: %f Position: %f\n", Index, Path, MusicDuration, CurrentPosition);
    return LoopLength;
  }

  return 0;
}
