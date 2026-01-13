#include <stdlib.h>
#include <stdio.h>
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

AudioData *Audio;

bool LoopLock = false; /* Used for LOOP_ALL functionality */

uint32_t SA_TotalAudio = 2, SA_ExternalAudio = 0;
int32_t AudioVolume = 100, AudioCurrentIndex = -1;

MIX_Audio *Music;
MIX_Track *DefaultTrack;
static MIX_Mixer *DefaultMixer;

double AudioDuration = 0, AudioPosition = 0;

char *AudioCurrentPath = NULL;

void InitializeAudio() {
  if (!(DefaultMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL))) {
    SDL_Log("Couldn't open audio %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  Audio = malloc(sizeof(AudioData) * SA_TotalAudio);
  DefaultTrack = MIX_CreateTrack(DefaultMixer);
}

void AudioRemove(uint32_t Index) {
  if (Index == SA_TotalAudio || Audio[Index].Path[0] == 0)
    return;

  if (Audio[Index].Stream) {
    free(Audio[Index].StreamMemory);
    SDL_CloseIO(Audio[Index].Stream);
  }

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

int32_t GetAudioIndexByPath(char *Path) {
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

int32_t AddAudio(char *Path, char *Category, SDL_IOStream *Stream) {
  if (GetAudioIndexByPath(Path) != -1) {
    SDL_Log("\"%s\" is already loaded.", Path);
    return -1;
  }

  Category = Category == NULL ? "All" : Category;

  int32_t Index = GetEmptyIndex();
  MIX_Audio *l_Music;

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

  if (!Stream)
    l_Music = MIX_LoadAudio(DefaultMixer, Path, 0);
  else
    l_Music = MIX_LoadAudio_IO(DefaultMixer, Stream, 0, 0);

  if (!l_Music) {
    SDL_Log("Failed to load \"%s\": %s", Path, SDL_GetError());
    return -1;
  }

  SDL_PropertiesID Properties = MIX_GetAudioProperties(l_Music);

  if (SDL_GetStringProperty(Properties, MIX_PROP_METADATA_TITLE_STRING, NULL) != NULL) {
    const char *MusicTitle = SDL_GetStringProperty(Properties, MIX_PROP_METADATA_TITLE_STRING, NULL);
    memcpy(Audio[Index].Title, MusicTitle, strlen(MusicTitle));
  } else {
    SDL_Log("WARNING: LocalTagTitle is empty.");

    if (Stream) {
      const char *l_TitleBuffer = "External Audio";
      memcpy(Audio[Index].Title, l_TitleBuffer, strlen(l_TitleBuffer));
    } else {
      char *LocalPath = Path;
      char *LastPathPointer = "";

      while (*(LocalPath += strspn(LocalPath, PathDelimiter)) != '\0') {
        size_t Length = strcspn(LocalPath, PathDelimiter);
        LastPathPointer = LocalPath;
        LocalPath += Length;
      }

      memcpy(Audio[Index].Title, LastPathPointer, strlen(LastPathPointer));
    }
  }

  TagArtist = SDL_GetStringProperty(Properties, MIX_PROP_METADATA_ARTIST_STRING, NULL);
  TagCopyright = SDL_GetStringProperty(Properties, MIX_PROP_METADATA_COPYRIGHT_STRING, NULL);
  TagAlbum = SDL_GetStringProperty(Properties, MIX_PROP_METADATA_ALBUM_STRING, NULL);

  if (!TagArtist) {TagArtist = "N/A";}
  if (!TagCopyright) {TagCopyright = "N/A";}
  if (!TagAlbum) {TagAlbum = "N/A";}

  if (!Stream)
    memcpy(Audio[Index].Path, Path, strlen(Path));
  else
    sprintf(Audio[Index].Path, "External-%i", SA_ExternalAudio);

  memcpy(Audio[Index].TagArtist, TagArtist, strlen(TagArtist));
  memcpy(Audio[Index].TagAlbum, TagAlbum, strlen(TagAlbum));
  memcpy(Audio[Index].TagCopyright, TagCopyright, strlen(TagCopyright));
  memcpy(Audio[Index].AssignedList, Category, strlen(Category));

  Audio[Index].LayoutOrder = Index;
  Audio[Index].Stream = Stream ? Stream : NULL;

  RefreshUI();
  MIX_DestroyAudio(l_Music);
  return Index;
}

void UpdateAudioPosition() {
  if (MIX_TrackPlaying(DefaultTrack)) {
    LoopLock = false;
    AudioPosition = MIX_TrackFramesToMS(DefaultTrack, MIX_GetTrackPlaybackPosition(DefaultTrack)) / 1000;
  } else {
    if (LoopStatus == LOOP_SONG) {
      if (GetAudioIndexByPath(AudioCurrentPath) != -1) {
        MIX_PlayTrack(DefaultTrack, 0);
      }
    } else if (LoopStatus == LOOP_ALL && LoopLock == false) {
      LoopLock = true;

      if (GetAudioIndexByPath(AudioCurrentPath) != -1)
        PlayAudio(Audio[GetNextIndex(AudioCurrentIndex)].Path);
    } else if (LoopStatus == LOOP_ALL && LoopLock == true) {
      /* Probably not the best way to handle it */
      if (GetAudioIndexByPath(AudioCurrentPath) != -1) {
        MIX_PauseTrack(DefaultTrack);
        PlayAudio(Audio[AudioCurrentIndex].Path);
      }
    } else if (LoopStatus == LOOP_NONE) {
      MIX_DestroyAudio(Music);

      Music = NULL;
      AudioCurrentIndex = -1;
    }
  }
}

int8_t PlayAudio(char *Path) {
  int Index = GetAudioIndexByPath(Path);

  if (Index == -1)
    Index = AddAudio(Path, NULL, NULL);

  if (Music != NULL) {
    MIX_DestroyAudio(Music);
    Music = NULL;
  }

  SDL_Log("Attempting to load \"%s\"", Path);
  if (Audio[Index].Stream) {
    SDL_SeekIO(Audio[Index].Stream, 0, SDL_IO_SEEK_SET);
    Music = MIX_LoadAudio_IO(DefaultMixer, Audio[Index].Stream, 0, 0);
  } else {
    Music = MIX_LoadAudio(DefaultMixer, Path, 0);
  }

  if (Music) {
    if (AudioCurrentIndex != Index)
      UpdateActivityRPC(Audio[Index].Title, Audio[Index].TagArtist);

    MIX_SetTrackAudio(DefaultTrack, Music);

    AudioCurrentIndex = Index;
    AudioCurrentPath = Path;

    AudioDuration = MIX_AudioFramesToMS(Music, MIX_GetAudioDuration(Music)) / 1000;
    AudioPosition = 0;

    MIX_SetTrackPlaybackPosition(DefaultTrack, 0);

    if (PausedMusic)
      MIX_ResumeTrack(DefaultTrack);
    MIX_PlayTrack(DefaultTrack, 0);

    return 0;
  } else {
    SDL_Log("Error loading music: %s", SDL_GetError());
  }

  return -1;
}
