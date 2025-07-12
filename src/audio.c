#include <stdlib.h>
#include <string.h>

#ifndef WINDOWS
#include <SDL3_mixer/SDL_mixer.h>

const char *PathDelimiter = "/";
#else
#include <SDL3/SDL_mixer.h>

const char *PathDelimiter = "\\";
#endif

#include "discord.h"
#include "gui.h"
#include "audio.h"

static SDL_AudioSpec Specifications = {
  .freq = MIX_DEFAULT_FREQUENCY,
  .format = MIX_DEFAULT_FORMAT,
  .channels = MIX_DEFAULT_CHANNELS
};

AudioData *Audio;

bool LoopLock = false; /* Used for LOOP_ALL functionality */

uint32_t SA_TotalAudio = 2;
int AudioVolume = MIX_MAX_VOLUME, AudioCurrentIndex = -1;

static Mix_Music *Music;

double AudioDuration = 0, AudioPosition = 0;

char *AudioCurrentPath = NULL;

void InitializeAudio() {
  if (!Mix_OpenAudio(0, &Specifications)) {
    SDL_Log("Couldn't open audio %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  } else {
    Mix_QuerySpec(&Specifications.freq, &Specifications.format, &Specifications.channels);
  }
  
  Audio = malloc(sizeof(AudioData) * SA_TotalAudio);
  
  Mix_VolumeMusic(AudioVolume);
}

void AudioRemove(uint32_t Index) {
  if (Index == SA_TotalAudio || Audio[Index].Path[0] == 0)
    return;
  
  memset(&Audio[Index], 0, sizeof(AudioData));

  for (uint32_t i = Index + 1; i < SA_TotalAudio; i++) {
    if (Audio[Index].Path[0] == 0)
      continue;

    Audio[Index].LayoutOrder -= 1;
  }
}

int32_t GetEmptyIndex() {
  for (uint32_t i = 0; i < SA_TotalAudio; i++)
    if (Audio[i].Path[0] == 0)
      return i;
  return -1;
}

int32_t GetNextIndex(uint32_t Index) {
  uint32_t LayoutOrder = Audio[Index].LayoutOrder;

  for (uint32_t i = 0; i < SA_TotalAudio; i++) {
    if (Audio[Index].Path[0] == 0)
      continue;

    if (Audio[i].LayoutOrder <= LayoutOrder)
      continue;

    if (strcmp(Audio[Index].AssignedList, Audio[i].AssignedList) == 0)
      return i;
  }
  
  return 0; /* Fallback return */
}

int32_t GetAudioIndex(char *Path) {
  if (Path == NULL)
    return -1;

  for (uint32_t i = 0; i < SA_TotalAudio; i++) {
    if (Audio[i].Path[0] == 0)
      continue;

    if (strcmp(Audio[i].Path, Path) == 0)
      return i;
  }

  return -1;
}

int32_t AddAudio(char *Path, char *Category) {
  if (GetAudioIndex(Path) != -1) {
    SDL_Log("\"%s\" is already loaded.", Path);
    return -1;
  }
  
  Category = Category == NULL ? "All" : Category;

  int32_t Index = GetEmptyIndex();
  Mix_Music *l_Music;

  const char *TagArtist = NULL;
  const char *TagAlbum = NULL;
  const char *TagCopyright = NULL;

  if (Index == -1) {
    AudioData *l_Audio = realloc(Audio, sizeof(AudioData) * (SA_TotalAudio * 2));
    Index = SA_TotalAudio;
    
    if (!l_Audio) {
      SDL_Log("Failed to reallocate Audio buffer during AddAudio call.\n");
      exit(EXIT_FAILURE);
    }

    for (uint16_t i = SA_TotalAudio; i < SA_TotalAudio * 2; i++)
      memset(&l_Audio[i], 0, sizeof(AudioData));

    SA_TotalAudio *= 2;
    Audio = l_Audio;
  }

  l_Music = Mix_LoadMUS(Path);

  if (!l_Music) {
    SDL_Log("Failed to load \"%s\": %s", Path, SDL_GetError());
    return -1;
  }

  if (Mix_GetMusicTitle(l_Music)[0] != 0) {
    char *MusicTitle = (char*)Mix_GetMusicTitle(l_Music);

    memcpy(Audio[Index].Title, MusicTitle, strlen(MusicTitle));
  } else {
    SDL_Log("WARNING: LocalTagTitle is empty.");

    char *LocalPath = Path;
    char *LastPathPointer;
    
    while (*(LocalPath += strspn(LocalPath, PathDelimiter)) != '\0') {
      size_t Length = strcspn(LocalPath, PathDelimiter);
      LastPathPointer = LocalPath;
      LocalPath += Length;
    }
    
    memcpy(Audio[Index].Title, LastPathPointer, strlen(LastPathPointer));
  }
  
  TagArtist = Mix_GetMusicArtistTag(l_Music);
  TagCopyright = Mix_GetMusicCopyrightTag(l_Music);
  TagAlbum = Mix_GetMusicAlbumTag(l_Music);
  
  if (TagArtist[0] == 0) {TagArtist = "N/A";}
  if (TagCopyright[0] == 0) {TagCopyright = "N/A";}
  if (TagAlbum[0] == 0) {TagAlbum = "N/A";}

  memcpy(Audio[Index].Path, Path, strlen(Path));
  memcpy(Audio[Index].TagArtist, TagArtist, strlen(TagArtist));
  memcpy(Audio[Index].TagAlbum, TagAlbum, strlen(TagAlbum));
  memcpy(Audio[Index].TagCopyright, TagCopyright, strlen(TagCopyright));
  memcpy(Audio[Index].AssignedList, Category, strlen(Category));
  
  Audio[Index].LayoutOrder = Index;
  
  RefreshPlaylist();
  Mix_FreeMusic(l_Music);
  return Index;
}

void UpdateAudioPosition() {
  if (Mix_PlayingMusic()) {
    LoopLock = false;
    AudioPosition = Mix_GetMusicPosition(Music);
  } else {
    if (LoopStatus == LOOP_SONG) {
      if (GetAudioIndex(AudioCurrentPath) != -1)
        PlayAudio(AudioCurrentPath);
    } else if (LoopStatus == LOOP_ALL && LoopLock == false) {
      LoopLock = true;

      if (GetAudioIndex(AudioCurrentPath) != -1)
        PlayAudio(Audio[GetNextIndex(AudioCurrentIndex)].Path);
    } else if (LoopStatus == LOOP_ALL && LoopLock == true) {
      /* Probably not the best way to handle it */
      if (GetAudioIndex(AudioCurrentPath) != -1)
        PlayAudio(Audio[AudioCurrentIndex].Path);
    } else if (LoopStatus == LOOP_NONE) {
      Mix_FreeMusic(Music);

      Music = NULL;
      AudioCurrentIndex = -1;
    }
  }
}

int8_t PlayAudio(char *Path) {
  int Index = GetAudioIndex(Path);

  if (Index == -1)
    Index = AddAudio(Path, NULL);

  if (Music != NULL) {
    Mix_FreeMusic(Music);
    Music = NULL;
  }
  
  Music = Mix_LoadMUS(Path);
  
  SDL_Log("Attempting to load \"%s\"", Path);

  if (Music) {
    if (AudioCurrentIndex != Index)
      UpdateActivityRPC(Audio[Index].Title, Audio[Index].TagArtist);

    AudioCurrentIndex = Index;
    AudioCurrentPath = Path;

    AudioDuration = Mix_MusicDuration(Music);
    AudioPosition = 0;

    Mix_PlayMusic(Music, 0);
    Mix_SetMusicPosition(0);

    return 0;
  }
  
  return -1;
}
