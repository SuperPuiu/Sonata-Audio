#include "microui.h"
#include "gui.h"
#include "render.h"
#include "pfd.h"
#include "audio.h"

#ifndef WINDOWS
#include <dirent.h>
#include <sys/stat.h>
#include <SDL3_mixer/SDL_mixer.h>
#else
#include <windows.h>
#include <SDL3/SDL_mixer.h>
#endif

#include <stdio.h>
#include <string.h>

int LoopStatus = LOOP_NONE;
static int PlaylistWidths[PAP_MAX_AUDIO];
static int TitleOpt     = MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOFRAME | MU_OPT_ANCHORED;
static int BelowOpt     = MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NOFRAME;
static int PlaylistOpt  = MU_OPT_NOTITLE | MU_OPT_NOBORDER | MU_OPT_NOINTERACT;
static int CategoryOpt  = MU_OPT_NOTITLE | MU_OPT_NOBORDER | MU_OPT_NOINTERACT;

static int PopupOpt = 0;
static int InfoFrameOpt = 0;
static int SelectedAudio = -1;
static int SlidingAudio = -1;

float l_AudioPosition;
static float AudioFloat = MIX_MAX_VOLUME;

static bool InfoOpen = false, PopupOpen = false;
static bool PausedMusic = false; /* Paused using the button */

static mu_Rect PAP_Title, PAP_Below;
static mu_Rect PAP_Playlist, PAP_Popup;
static mu_Rect PAP_InfoFrame, PAP_Category;
static mu_Rect PAP_Popup;

char *CurrentCategory = "All";
char Categories[PAP_MAX_CATEGORIES][128];
static const char *InteractButtonText = "Pause";
static const char *LoopButtonText = "No loop";

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

int PAP_GetAudioByOrder(uint8_t RequestedOrder) {
  for (int i = 0; i < PAP_MAX_AUDIO; i++) {
    if (Audio[i].LayoutOrder == RequestedOrder)
      return i;
  }

  return -1;
}

void FuncRemoveAudio() {
  AudioRemove(SelectedAudio);
}

int PAP_AudioButton(mu_Context *Context, const char *Name, int AudioID) {
  mu_Id ButtonID = mu_get_id(Context, &AudioID, sizeof(AudioID));
  
  mu_Rect Rect = mu_layout_next(Context);
  mu_Rect MainRect = {Rect.x + 20, Rect.y, Rect.w - 20, Rect.h};
  mu_Rect Slider = {Rect.x, Rect.y, 15, Rect.h}; /* Might need a better name for this variable */

  mu_update_control(Context, ButtonID, MainRect, 0);

  int Result = 0;
  
  if (Context->mouse_pressed == MU_MOUSE_LEFT && mu_mouse_over(Context, MainRect)) {
    PlayAudio(Audio[AudioID].Path);
    Result |= MU_RES_CHANGE;
  } else if (Context->mouse_pressed == MU_MOUSE_RIGHT && mu_mouse_over(Context, MainRect) && AudioID != AudioCurrentIndex) {
    mu_open_popup(Context, "Menu");
    SelectedAudio = AudioID;
    Result |= MU_RES_CHANGE;
  } else if (Context->mouse_pressed == MU_MOUSE_LEFT && mu_mouse_over(Context, Slider)) {
    SlidingAudio = AudioID;
  } else if (Context->mouse_down != MU_MOUSE_LEFT && AudioID == SlidingAudio) {
    SlidingAudio = -1;
  }

  if (SlidingAudio == AudioID) {
    int UpperAudio = PAP_GetAudioByOrder(Audio[AudioID].LayoutOrder + 1);
    int LowerAudio = PAP_GetAudioByOrder(Audio[AudioID].LayoutOrder - 1);

    if (LowerAudio > -1) {
      if (mu_mouse_over(Context, (mu_Rect){Slider.x, Slider.y - Slider.h - Context->style->padding, Slider.w, Slider.h})) {
        Audio[AudioID].LayoutOrder -= 1;
        Audio[LowerAudio].LayoutOrder += 1;
      }
    }

    if (UpperAudio > -1) {
      if (mu_mouse_over(Context, (mu_Rect){Slider.x, Slider.y + Slider.h + Context->style->padding, Slider.w, Slider.h})) {
        Audio[AudioID].LayoutOrder += 1;
        Audio[UpperAudio].LayoutOrder -= 1;
      }
    }
  }
  
  mu_draw_control_frame(Context, ButtonID, Slider, MU_COLOR_BUTTON, MU_OPT_NOBORDER);
  mu_draw_control_frame(Context, ButtonID, MainRect, AudioCurrentIndex != AudioID ? MU_COLOR_BUTTON : MU_COLOR_BASE, MU_OPT_NOBORDER); /* Main button frame */
  mu_draw_control_text(Context, Name, MainRect, MU_COLOR_TEXT, 0);
  // mu_draw_control_text();

  return Result;
}

int PAP_Slider(mu_Context *Context, mu_Real *Value, int Low, int High) {
  int Result = 0, ScrollSpeed = Context->key_down & MU_KEY_SHIFT ? 1 : 2;
  mu_Real Last = *Value, l_Value = Last;
  mu_Rect l_Rect = mu_layout_next(Context);

  mu_push_id(Context, &Value, sizeof(Value));
  mu_Id ID = mu_get_id(Context, &Value, sizeof(Value)); 
  
  mu_update_control(Context, ID, l_Rect, MU_OPT_ALIGNCENTER);
  
  if (Context->focus == ID && (Context->mouse_down | Context->mouse_pressed) == MU_MOUSE_LEFT)
    l_Value = Low + (Context->mouse_pos.x - l_Rect.x) * (High - Low) / l_Rect.w;

  if (Context->scroll_delta.y != 0 && mu_mouse_over(Context, l_Rect))
    *Value += Context->scroll_delta.y / (Context->scroll_delta.y / ScrollSpeed) * (Context->scroll_delta.y > 0 ? -1 : 1);
  
  if (Last != l_Value) {
    *Value = l_Value = mu_clamp(l_Value, Low, High);
    Result |= MU_RES_CHANGE;
  }
  
  mu_draw_control_frame(Context, ID, l_Rect, MU_COLOR_BASE, MU_OPT_NOINTERACT);
  mu_draw_control_frame(Context, ID, (mu_Rect){l_Rect.x, l_Rect.y, mu_clamp(l_Value / High * l_Rect.w, 0, l_Rect.w), l_Rect.h}, MU_COLOR_TEXT, MU_OPT_NOINTERACT);

  mu_pop_id(Context);
  return Result;
}

void InitializeGUI() {  
  for (int i = 0; i < PAP_MAX_AUDIO; i++)
    PlaylistWidths[i] = PLAYLIST_WIDTH - 25;

  PAP_Title = (mu_Rect){0, 0, WINDOW_WIDTH, 20};
  PAP_Below = (mu_Rect){0, WINDOW_HEIGHT - BELOW_HEIGHT, WINDOW_WIDTH, BELOW_HEIGHT};
  PAP_Playlist = (mu_Rect){WINDOW_WIDTH / 2 - PLAYLIST_WIDTH / 2, WINDOW_HEIGHT / 2 - PLAYLIST_HEIGHT / 2, PLAYLIST_WIDTH, PLAYLIST_HEIGHT};
  PAP_InfoFrame = (mu_Rect){WINDOW_WIDTH / 2 - INFO_WIDTH / 2, WINDOW_HEIGHT / 2 - INFO_HEIGHT / 2, INFO_WIDTH, INFO_HEIGHT};
  PAP_Category = (mu_Rect){PAP_Playlist.x, PAP_Playlist.y - CATEGORY_HEIGHT - 3, CATEGORY_WIDTH, CATEGORY_HEIGHT};
  PAP_Popup = (mu_Rect){WINDOW_WIDTH / 2 - POPUP_WIDTH / 2, WINDOW_HEIGHT / 2 - POPUP_HEIGHT / 2, POPUP_WIDTH, POPUP_HEIGHT};
}

void MainWindow(mu_Context *Context) {
  mu_Container *InfoContainer = mu_get_container(Context, "INFO");
  mu_Container *PopupContainer = mu_get_container(Context, "POPUP");

  if (!InfoContainer->open) {InfoOpen = false;}
  if (!PopupContainer->open) {PopupOpen = false;}

  /* Title */
  if (mu_begin_window_ex(Context, "Puius Audio Player", PAP_Title, TitleOpt)) {
    mu_end_window(Context);
  } else {
    Running = false;
    return;
  }

  /* Below */
  if (mu_begin_window_ex(Context, "BELOW", PAP_Below, BelowOpt)) {
    mu_Rect InteractionRect = {WINDOW_WIDTH / 2 - 105, 50, 100, 20};
    mu_Rect LoopRect = {InteractionRect.x + 105, InteractionRect.y, InteractionRect.w, InteractionRect.h};
    mu_Rect VolumeRect = {WINDOW_WIDTH - 110, 50, 100, 20};

    mu_layout_set_next(Context, (mu_Rect){2, 50, 100, 20}, 1);
    if (mu_button(Context, "Load directory")) {
      const char *Path = OpenDialogue(PFD_DIRECTORY);
      size_t PathLen = strlen(Path);

      if (Path) {
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
        } else {
          SDL_Log("Directory is NULL.");
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
        PausedMusic = false;
      } else {
        Mix_PauseMusic();
        InteractButtonText = "Resume";
        PausedMusic = true;
      }
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
    if (PAP_Slider(Context, &l_AudioPosition, 0, AudioDuration)) {
      if (!Mix_PausedMusic())
        Mix_PauseMusic();

      AudioPosition = (double)l_AudioPosition;
      Mix_SetMusicPosition(AudioPosition);
    } else if (Context->mouse_down != MU_MOUSE_LEFT && !PausedMusic) {
      Mix_ResumeMusic();
    }

    l_AudioPosition = AudioPosition;
    
    mu_layout_set_next(Context, VolumeRect, 1);
    if (PAP_Slider(Context, &AudioFloat, 0, 128)) {
      AudioVolume = (int)AudioFloat;
      Mix_VolumeMusic(AudioVolume);
    }

    mu_end_window(Context);
  }
  
  /* Playlist */
  if (mu_begin_window_ex(Context, "PLAYLIST", PAP_Playlist, PlaylistOpt)) {
    int l_Width[] = {PLAYLIST_WIDTH - 20};

    mu_Container *Container = mu_get_container(Context, "Menu");
    if (SelectedAudio == -1) {Container->open = 0;}
    
    AudioData l_Audio[PAP_MAX_AUDIO] = {0};
    uint8_t l_AudioIDs[PAP_MAX_AUDIO] = {0};

    for (int i = 0; i < PAP_MAX_AUDIO; i++) {
      if (Audio[i].Path[0] != 0) {
        l_Audio[Audio[i].LayoutOrder] = Audio[i];
        l_AudioIDs[Audio[i].LayoutOrder] = i;
      }
    }

    for (int i = 0; i < PAP_MAX_AUDIO; i++) {
      if (l_Audio[i].Path[0] == 0)
        continue;

      if (strcmp(l_Audio[i].AssignedList, CurrentCategory) != 0)
        continue;

      mu_layout_row(Context, 1, l_Width, 25);
      PAP_AudioButton(Context, l_Audio[i].Title, l_AudioIDs[i]);
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
  if (mu_begin_window_ex(Context, "CATEGORIES", PAP_Category, CategoryOpt)) { 
    int l_Widths[PAP_MAX_CATEGORIES];
    static uint8_t TotalCategoryButtons = 2;
    
    for (uint8_t i = 0; i < TotalCategoryButtons - 1; i++)
      l_Widths[i] = 70;
    
    l_Widths[TotalCategoryButtons - 1] = 20;
    
    mu_layout_row(Context, TotalCategoryButtons, l_Widths, TotalCategoryButtons < 8 ? 20 : 15);
    if (mu_button(Context, "All"))
      CurrentCategory = "All";
    
    for (uint8_t i = 0; i < PAP_MAX_CATEGORIES; i++) {
      if (Categories[i][0] == 0)
        continue;
       
      if (mu_button(Context, Categories[i]))
        CurrentCategory = Categories[i];
    }
    
    if (mu_button(Context, "+")) {
      for (uint8_t i = 0; i < PAP_MAX_CATEGORIES; i++) {
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

    mu_end_window(Context);
  }
  
  PopupContainer->open = PopupOpen;
  PopupContainer->zindex = 3;

  /* Popup */
  if (mu_begin_window_ex(Context, "POPUP", PAP_Popup, PopupOpt)) {
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
  if (mu_begin_window_ex(Context, "INFO", PAP_InfoFrame, InfoFrameOpt)) {
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
