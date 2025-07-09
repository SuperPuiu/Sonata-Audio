#include "microui.h"
#include "gui.h"
#include "render.h"
#include "pfd.h"
#include "audio.h"
#include "gui_ext.h"

#ifndef WINDOWS
#include <dirent.h>
#include <sys/stat.h>
#include <SDL3_mixer/SDL_mixer.h>
#else
#include <windows.h>
#include <SDL3/SDL_mixer.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int TitleOpt     = MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOFRAME | MU_OPT_ANCHORED | MU_OPT_ALIGNCENTER;
static int BelowOpt     = MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NOFRAME;
static int PlaylistOpt  = MU_OPT_NOTITLE | MU_OPT_NOBORDER | MU_OPT_NOINTERACT | MU_OPT_NOFRAME;
static int CategoryOpt  = MU_OPT_NOTITLE | MU_OPT_NOBORDER | MU_OPT_NOINTERACT;
static int SearchOpt    = MU_OPT_NOTITLE | MU_OPT_NOBORDER | MU_OPT_NOINTERACT | MU_OPT_NOSCROLL;

static int PopupOpt = 0;
static int InfoFrameOpt = 0;
int SelectedAudio = -1;
int LoopStatus = LOOP_NONE;

float l_AudioPosition;
static float AudioFloat = MIX_MAX_VOLUME;

static bool InfoOpen = false, PopupOpen = false;
static bool PausedMusic = false; /* Paused using the button */

static mu_Rect SA_Title, SA_Below;
static mu_Rect SA_Playlist, SA_Popup;
static mu_Rect SA_InfoFrame, SA_Category;
static mu_Rect SA_Popup, SA_Search;

char *CurrentCategory = "All";
char Categories[SA_MAX_CATEGORIES][32];
char SearchBuffer[128] = {0};
static const char *InteractButtonText = "Pause";
static const char *LoopButtonText = "No loop";

static AudioData *PlaylistAudios;
static uint8_t *PlaylistAudioIDs;
static uint32_t PlaylistBufferSizes;

void (*PopupAction)(void);

int TextWidth(mu_Font font, const char *text, int len) {
  unused(font);

  if (len == -1) { len = strlen(text); }
  return r_get_text_width(text, len);
}

int TextHeight(mu_Font font) {
  unused(font);

  return r_get_text_height();
}

void FuncRemoveAudio() {
  AudioRemove(SelectedAudio);
}

void LowerString(char *Str) {
  for(uint32_t i = 0; Str[i]; i++)
    Str[i] = tolower(Str[i]);
}

void RefreshPlaylist() {
  char *l_SearchBuffer = strdup(SearchBuffer);
  LowerString(l_SearchBuffer);

  if (SA_TotalAudio > PlaylistBufferSizes) {
    AudioData *l_Audio = realloc(PlaylistAudios, sizeof(AudioData) * SA_TotalAudio);

    if (!l_Audio) {
      SDL_Log("[FATAL]: Unable to realloc PlaylistAudios.\n");
      exit(EXIT_FAILURE);
    }

    uint8_t *l_AudioIDs = realloc(PlaylistAudioIDs, sizeof(uint8_t) * SA_TotalAudio);

    if (!l_AudioIDs) {
      SDL_Log("[FATAL]: Unable to realloc PlaylistAudioIDs.\n");
      exit(EXIT_FAILURE);
    }

    PlaylistAudios = l_Audio;
    PlaylistAudioIDs = l_AudioIDs;
  }

  memset(PlaylistAudios, 0, sizeof(AudioData) * SA_TotalAudio);
  memset(PlaylistAudioIDs, 0, sizeof(uint8_t) * SA_TotalAudio);

  for (uint32_t i = 0; i < SA_TotalAudio; i++) {
    if (Audio[i].Path[0] == 0)
      continue;
    
    if (strcmp(Audio[i].AssignedList, CurrentCategory) != 0)
      continue;
    
    if (SearchBuffer[0] != 0) {
      char *LowerTitle = strdup(Audio[i].Title);
      LowerString(LowerTitle);

      if (!strstr(LowerTitle, l_SearchBuffer)) {
        free(LowerTitle);
        continue;
      }

      free(LowerTitle);
    }

    PlaylistAudios[Audio[i].LayoutOrder] = Audio[i];
    PlaylistAudioIDs[Audio[i].LayoutOrder] = i;
  }

  PlaylistBufferSizes = SA_TotalAudio;
  free(l_SearchBuffer);
}

void InitializeGUI() {
  SA_Title = (mu_Rect){0, 0, WINDOW_WIDTH, 20};
  SA_Below = (mu_Rect){0, WINDOW_HEIGHT - BELOW_HEIGHT, WINDOW_WIDTH, BELOW_HEIGHT};
  SA_Playlist = (mu_Rect){CATEGORY_WIDTH, 54, PLAYLIST_WIDTH, PLAYLIST_HEIGHT};
  SA_InfoFrame = (mu_Rect){WINDOW_WIDTH / 2 - INFO_WIDTH / 2, WINDOW_HEIGHT / 2 - INFO_HEIGHT / 2, INFO_WIDTH, INFO_HEIGHT};
  SA_Category = (mu_Rect){0, 24, CATEGORY_WIDTH, CATEGORY_HEIGHT};
  SA_Popup = (mu_Rect){WINDOW_WIDTH / 2 - POPUP_WIDTH / 2, WINDOW_HEIGHT / 2 - POPUP_HEIGHT / 2, POPUP_WIDTH, POPUP_HEIGHT};
  SA_Search = (mu_Rect){SA_Playlist.x, SA_Playlist.y - 30, SEARCH_WIDTH, SEARCH_HEIGHT};
  
  PlaylistBufferSizes = SA_TotalAudio;
  PlaylistAudios = calloc(SA_TotalAudio, sizeof(AudioData));
  PlaylistAudioIDs = calloc(SA_TotalAudio, sizeof(uint8_t));
}

void MainWindow(mu_Context *Context) {
  mu_Container *InfoContainer = mu_get_container(Context, "INFO");
  mu_Container *PopupContainer = mu_get_container(Context, "POPUP");

  if (!InfoContainer->open) {InfoOpen = false;}
  if (!PopupContainer->open) {PopupOpen = false;}

  /* Title */
  if (mu_begin_window_ex(Context, "Sonata Audio", SA_Title, TitleOpt)) {
    mu_end_window(Context);
  } else {
    Running = false;
    return;
  }

  /* Below */
  if (mu_begin_window_ex(Context, "BELOW", SA_Below, BelowOpt)) {
    mu_Rect InteractionRect = {WINDOW_WIDTH / 2 - 105, 50, 100, 20};
    mu_Rect LoopRect = {InteractionRect.x + 105, InteractionRect.y, InteractionRect.w, InteractionRect.h};
    mu_Rect VolumeRect = {WINDOW_WIDTH - 110, 50, 100, 20};

    mu_layout_set_next(Context, (mu_Rect){2, 50, 100, 20}, 1);
    if (mu_button(Context, "Load directory")) {
      const char *Path = OpenDialogue(PFD_DIRECTORY);
      
      if (!Path) {
        SDL_Log("Directory is NULL.");
      } else {
        size_t PathLen = strlen(Path);

        #ifndef WINDOWS
        DIR *Directory;
        struct dirent *Entry;

        if ((Directory = opendir(Path)) != NULL) {
          char FullPath[PATH_MAX];
          
          memcpy(FullPath, Path, PathLen);

          while ((Entry = readdir(Directory)) != NULL) {
            struct stat Stats;
            
            /* Not interested into those directories. */
            if (strcmp(Entry->d_name, ".") == 0 || strcmp(Entry->d_name, "..") == 0)
              continue;
            
            /* Compute absolute path */
            FullPath[PathLen] = '\0';
            strcat(FullPath, "/");
            strcat(FullPath, Entry->d_name);

            if (stat(FullPath, &Stats) == -1) {
              SDL_Log("stat() failed on %s.", Entry->d_name);
              continue;
            }

            if (S_ISREG(Stats.st_mode) != 0)
              AddAudio(FullPath, CurrentCategory);
          }

          closedir(Directory);
        } 
        #else
        char AudioPath[MAX_PATH];

        TCHAR DirectoryPath[MAX_PATH];
        HANDLE HandleFind = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA FileData;
        
        strcat(DirectoryPath, Path);
        strcat(DirectoryPath, "\\*");
        memcpy(AudioPath, Path, PathLen);

        HandleFind = FindFirstFile(DirectoryPath, &FileData);

        if (HandleFind != INVALID_HANDLE_VALUE) {
          do {
            /* Maybe one day we'll handle multiple directories. Maybe. */
            if (!(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
              AudioPath[PathLen] = '\0';
              strcat(AudioPath, "\\");
              strcat(AudioPath, FileData.cFileName);

              AddAudio(AudioPath, NULL);
            }
          } while (FindNextFile(HandleFind, &FileData) != 0);
        } else {
          SDL_Log("INVALID_HANDLE_VALUE returned by FindFirstFile.");
        }
        #endif
      }
    }

    /*if (mu_button(Context, "Settings")) {
      
    }*/

    mu_layout_set_next(Context, InteractionRect, 1);
    
    if (mu_button_ex(Context, InteractButtonText, 0, MU_OPT_ALIGNCENTER)) {
      if (Mix_PausedMusic()) {
        Mix_ResumeMusic();
        InteractButtonText = "Pause";
      } else {
        Mix_PauseMusic();
        InteractButtonText = "Resume";
      }

      PausedMusic = !PausedMusic;
    }

    mu_layout_set_next(Context, LoopRect, 1);
    
    if (mu_button_ex(Context, LoopButtonText, 0, MU_OPT_ALIGNCENTER)) {
      if (LoopStatus == LOOP_NONE) {
        LoopButtonText = "Looping song";
        LoopStatus = LOOP_SONG;
      } else if (LoopStatus == LOOP_SONG) {
        LoopButtonText = "Looping all";
        LoopStatus = LOOP_ALL;
      } else if (LoopStatus == LOOP_ALL) {
        LoopButtonText = "No loop";
        LoopStatus = LOOP_NONE;
      }
    }

    mu_layout_set_next(Context, (mu_Rect){VolumeRect.x - VolumeRect.w - Context->style->padding + 50, VolumeRect.y, VolumeRect.w - 50, VolumeRect.h}, 1);
    mu_label(Context, "Volume:");

    mu_layout_set_next(Context, (mu_Rect){WINDOW_WIDTH / 2 - 225, 30, 450, 15}, 1);
    if (SA_Slider(Context, &l_AudioPosition, 0, AudioDuration)) {
      if (!Mix_PausedMusic())
        Mix_PauseMusic();

      AudioPosition = (double)l_AudioPosition;
      Mix_SetMusicPosition(AudioPosition);
    } else if (Context->mouse_down != MU_MOUSE_LEFT && !PausedMusic) {
      Mix_ResumeMusic();
    }

    l_AudioPosition = AudioPosition;
    
    mu_layout_set_next(Context, VolumeRect, 1);
    if (SA_Slider(Context, &AudioFloat, 0, 128)) {
      AudioVolume = (int)AudioFloat;
      Mix_VolumeMusic(AudioVolume);
    }

    mu_end_window(Context);
  }
  
  /* Search */
  if (mu_begin_window_ex(Context, "SEARCH", SA_Search, SearchOpt)) {
    mu_layout_set_next(Context, (mu_Rect){-5, 0, SEARCH_WIDTH, 20}, 1);
    
    if (mu_textbox(Context, SearchBuffer, sizeof(SearchBuffer)) & MU_RES_CHANGE)
      RefreshPlaylist();

    mu_end_window(Context);
  }

  /* Playlist */
  if (mu_begin_window_ex(Context, "PLAYLIST", SA_Playlist, PlaylistOpt)) {
    int l_Width[] = {PLAYLIST_WIDTH - 20};

    mu_Container *Container = mu_get_container(Context, "Menu");
    if (SelectedAudio == -1) {Container->open = 0;}
    
    for (uint32_t i = 0; i < PlaylistBufferSizes; i++) {
      if (PlaylistAudios[i].Path[0] == 0)
        continue;

      if (Audio[PlaylistAudioIDs[i]].Path[0] == 0)
        continue;

      mu_layout_row(Context, 1, l_Width, 25);
      SA_AudioButton(Context, PlaylistAudios[i].Title, PlaylistAudioIDs[i]);
    }
    
    mu_Rect l_Rect = mu_layout_next(Context);
    mu_layout_set_next(Context, (mu_Rect){l_Rect.x + l_Width[0] / 2 - 35, l_Rect.y, 70, 20}, 0);

    if (mu_button(Context, "+")) {
      const char *Path = OpenDialogue(PFD_FILE);

      if (Path)
        AddAudio((char *)Path, CurrentCategory);
      else
        SDL_Log("Path is NULL.\n");
    }

    if (mu_begin_popup(Context, "Menu")) {
      if (mu_button(Context, "Remove")) {
        PopupAction = FuncRemoveAudio;
        PopupOpen = true;
        PopupContainer->open = 1;
        Container->open = 0;
      }

      if (mu_button(Context, "About")) {
        InfoOpen = true;
        InfoContainer->open = 1;
        Container->open = 0;
      }

      mu_end_popup(Context);
    }
    
    mu_end_window(Context);
  }
  
  /* Directories */
  if (mu_begin_window_ex(Context, "CATEGORIES", SA_Category, CategoryOpt)) { 
    static uint8_t TotalCategoryButtons = 2;
    
    mu_layout_row(Context, 1, (int[]){70}, 20);
    if (SA_CategoryButton(Context, "All", 0)) {
      CurrentCategory = "All";
      RefreshPlaylist();
    }
    
    for (uint8_t i = 0; i < SA_MAX_CATEGORIES; i++) {
      if (Categories[i][0] == 0)
        continue;
       
      if (SA_CategoryButton(Context, Categories[i], 0)) {
        CurrentCategory = Categories[i];
        RefreshPlaylist();
      }
    }
    
    mu_Rect PlusRect = mu_layout_next(Context);
    PlusRect.x = CATEGORY_WIDTH / 2 - 8;
    PlusRect.w = PlusRect.h = 15;
    
    mu_layout_set_next(Context, PlusRect, 0);
    if (TotalCategoryButtons - 1 < SA_MAX_CATEGORIES) {
      if (mu_button(Context, "+")) {
        for (uint8_t i = 0; i < SA_MAX_CATEGORIES; i++) {
          if (Categories[i][0] == 0) {
            char Buf[5];
          
            TotalCategoryButtons += 1;
            sprintf(Buf, "%u", i); /* me when itoa doesn't want to compile */
            memcpy(Categories[i], "Category ", 9);
            memcpy(Categories[i] + 9, Buf, strlen(Buf));
            break;
          }
        } 
      }
    }
    
    mu_end_window(Context);
  }
  
  PopupContainer->open = PopupOpen;
  PopupContainer->zindex = 3;

  /* Popup */
  if (mu_begin_window_ex(Context, "POPUP", SA_Popup, PopupOpt)) {
    Context->hover_root = Context->next_hover_root = PopupContainer;
    mu_bring_to_front(Context, PopupContainer);

    mu_Rect l_Rect = PopupContainer->rect;
    
    mu_layout_row(Context, 1, (int[]){l_Rect.w - 15}, 25);
    mu_label(Context, "Proceed with the action?");
    
    mu_layout_set_next(Context, (mu_Rect){(l_Rect.x + l_Rect.w / 2) - 105, l_Rect.y + l_Rect.h - 25, 100, 20}, 0);

    if (mu_button_ex(Context, "Continue", 0, MU_OPT_ALIGNCENTER)) {
      PopupContainer->open = 0;
      PopupAction();
    }

    mu_layout_set_next(Context, (mu_Rect){(l_Rect.x + l_Rect.w / 2), l_Rect.y + l_Rect.h - 25, 100, 20}, 0);
    
    if (mu_button_ex(Context, "Abort", 0, MU_OPT_ALIGNCENTER))
      PopupContainer->open = 0;

    mu_end_window(Context);
  }

  InfoContainer->open = InfoOpen;
  InfoContainer->zindex = 2;
  
  /* INFO */
  if (mu_begin_window_ex(Context, "INFO", SA_InfoFrame, InfoFrameOpt)) {
    Context->hover_root = Context->next_hover_root = InfoContainer;
    mu_bring_to_front(Context, InfoContainer);

    char ArtistBuf[128 + 8] = {0};
    char CopyrightBuf[128 + 11] = {0};
    char AlbumBuf[128 + 7] = {0};

    memcpy(ArtistBuf, "Artist: ", 8);
    memcpy(CopyrightBuf, "Copyright: ", 11);
    memcpy(AlbumBuf, "Album: ", 7);
    memcpy(ArtistBuf + 8, Audio[SelectedAudio].TagArtist, strlen(Audio[SelectedAudio].TagArtist));
    memcpy(CopyrightBuf + 11, Audio[SelectedAudio].TagCopyright, strlen(Audio[SelectedAudio].TagCopyright));
    memcpy(AlbumBuf + 7, Audio[SelectedAudio].TagAlbum, strlen(Audio[SelectedAudio].TagAlbum));

    /* Columns hate me */
    mu_layout_row(Context, 1, (int[]){INFO_WIDTH - 25}, 25);
    mu_label(Context, ArtistBuf);

    mu_layout_row(Context, 1, (int[]){INFO_WIDTH - 25}, 25);
    mu_label(Context, CopyrightBuf);

    mu_layout_row(Context, 1, (int[]){INFO_WIDTH - 25}, 25);
    mu_label(Context, AlbumBuf);
    
    mu_end_window(Context);
  }
}

void ProcessContextFrame(mu_Context *Context) {
  mu_begin(Context);
  MainWindow(Context);
  mu_end(Context);
}
