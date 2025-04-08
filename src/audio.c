#include <stdlib.h>
#include <string.h>

#ifndef WINDOWS
#include <linux/limits.h>
#include <SDL3_mixer/SDL_mixer.h>

const char *PathDelimiter = "/";
#else
#include <SDL3/SDL_mixer.h>

const char *PathDelimiter = "\\";
#endif

#include "gui.h"
#include "audio.h"

static SDL_AudioSpec Specifications = {
  .freq = MIX_DEFAULT_FREQUENCY,
  .format = MIX_DEFAULT_FORMAT,
  .channels = MIX_DEFAULT_CHANNELS
};

AudioData Audio[PAP_MAX_AUDIO];

bool LoopLock = false; /* Used by LOOP_ALL functionality */

int AudioCurrentIndex = -1;
int AudioVolume = MIX_MAX_VOLUME;

static Mix_Music *Music;

double AudioDuration, AudioPosition;
static double LoopStart, LoopEnd, LoopLength;

char *AudioCurrentPath = NULL;

void InitializeAudio() {
  AudioPosition = 0;
  AudioDuration = 0;

  if (!Mix_OpenAudio(0, &Specifications)) {
    SDL_Log("Couldn't open audio %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  } else {
    Mix_QuerySpec(&Specifications.freq, &Specifications.format, &Specifications.channels);
  }
  
  for (int i = 0; i < PAP_MAX_AUDIO; i++)
    memset(Audio[i].Path, 0, PATH_MAX);
  
  Mix_VolumeMusic(AudioVolume);
}

void AudioRemove(int Index) {
  memset(&Audio[Index], 0, sizeof(AudioData));
}

int GetEmptyIndex() {
  for (int i = 0; i < PAP_MAX_AUDIO; i++)
    if (Audio[i].Path[0] == 0)
      return i;
  return -1;
}

int GetNextIndex(int Index) {
  if (Index > PAP_MAX_AUDIO || Index + 1 > PAP_MAX_AUDIO)
    return 0;

  if (Audio[Index + 1].Path[0] == 0)
    return 0;
  else
    return Index + 1;
}

int GetAudioIndex(char *Path) {
  if (Path == NULL)
    return -1;

  for (int i = 0; i < PAP_MAX_AUDIO; i++) {
    if (Audio[i].Path[0] == 0)
      continue;

    if (strcmp(Audio[i].Path, Path) == 0)
      return i;
  }

  return -1;
}

int AddAudio(char *Path) {
  if (GetAudioIndex(Path) != -1)
    return -1;

  int Index = GetEmptyIndex();
  Mix_Music *l_Music;
  char LocalTagTitle[256];

  const char *TagArtist = NULL;
  const char *TagAlbum = NULL;
  const char *TagCopyright = NULL;

  if (Index == -1)
    return -1;

  l_Music = Mix_LoadMUS(Path);

  if (!l_Music) {
    SDL_Log("Failed to load \"%s\": %s", Path, SDL_GetError());
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
    
    while (*(LocalPath += strspn(LocalPath, PathDelimiter)) != '\0') {
      size_t Length = strcspn(LocalPath, PathDelimiter);
      LastPathPointer = LocalPath;
      LocalPath += Length;
    }
    
    memcpy(LocalTagTitle, LastPathPointer, strlen(LastPathPointer));
  }
  
  TagArtist = Mix_GetMusicArtistTag(Music);
  TagCopyright = Mix_GetMusicCopyrightTag(Music);
  TagAlbum = Mix_GetMusicAlbumTag(Music);
  
  if (TagArtist[0] == 0) {TagArtist = "N/A";}
  if (TagCopyright[0] == 0) {TagCopyright = "N/A";}
  if (TagAlbum[0] == 0) {TagAlbum = "N/A";}

  memcpy(Audio[Index].Title, LocalTagTitle, strlen(LocalTagTitle));
  memcpy(Audio[Index].Path, Path, strlen(Path));
  memcpy(Audio[Index].TagArtist, TagArtist, strlen(TagArtist));
  memcpy(Audio[Index].TagAlbum, TagAlbum, strlen(TagAlbum));
  memcpy(Audio[Index].TagCopyright, TagCopyright, strlen(TagCopyright));

  Mix_FreeMusic(l_Music);
  return Index;
}

void UpdateAudioPosition() {
  if (Mix_PlayingMusic()) {
    LoopLock = false;
    AudioPosition = Mix_GetMusicPosition(Music);
  }

  if (!Mix_PlayingMusic()) {
    if (LoopStatus == LOOP_SONG) {
      if (GetAudioIndex(AudioCurrentPath) != -1)
        PlayAudio(AudioCurrentPath);
    } else if (LoopStatus == LOOP_ALL && LoopLock == false) {
      LoopLock = true;

      if (GetAudioIndex(AudioCurrentPath) != -1)
        PlayAudio(Audio[GetNextIndex(AudioCurrentIndex)].Path);
    } else if (LoopStatus == LOOP_ALL && LoopLock == true) {
      /* Probably not the best way to handle it */
      PlayAudio(Audio[AudioCurrentIndex].Path);
    } else if (LoopStatus == LOOP_NONE) {
      Mix_FreeMusic(Music);

      Music = NULL;
      AudioCurrentIndex = -1;
    }
  }
}

double PlayAudio(char *Path) {
  int Index = GetAudioIndex(Path);

  if (Index == -1)
    Index = AddAudio(Path);
  
  if (Music != NULL) {
    Mix_FreeMusic(Music);
    Music = NULL;
  }

  Music = Mix_LoadMUS(Path);
  
  SDL_Log("Attempting to load \"%s\"", Path);

  if (Music) {
    AudioCurrentIndex = Index;
    AudioCurrentPath = Path;

    AudioDuration = Mix_MusicDuration(Music);
    AudioPosition = 0;

    LoopStart = Mix_GetMusicLoopStartTime(Music);
    LoopEnd = Mix_GetMusicLoopEndTime(Music);
    LoopLength = Mix_GetMusicLoopLengthTime(Music);
    
    SDL_Log("Loaded!");
    
    Mix_PlayMusic(Music, 0);
    Mix_SetMusicPosition(0);
    return LoopLength;
  }
  
  return 0;
}
