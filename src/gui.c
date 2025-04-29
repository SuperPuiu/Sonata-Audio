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

#include <string.h>

int LoopStatus = LOOP_NONE;
static int PlaylistWidths[PAP_MAX_AUDIO];
static int TitleOpt     = MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOFRAME | MU_OPT_ANCHORED;
static int BelowOpt     = MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NOFRAME;
static int PlaylistOpt  = MU_OPT_NOTITLE | MU_OPT_NOBORDER | MU_OPT_NOINTERACT;
static int ExtraOpt     = MU_OPT_NOTITLE | MU_OPT_NOFRAME | MU_OPT_NOSCROLL;
static int DirsOpt      = MU_OPT_NOTITLE | MU_OPT_NOBORDER;

static int PopupOpt = 0;
static int InfoFrameOpt = 0;
static int SelectedAudio = -1;
static int SlidingAudio = -1;

float l_AudioPosition;
static float AudioFloat = MIX_MAX_VOLUME;

static bool InfoOpen = false, PopupOpen = false;

static mu_Rect PAP_Title, PAP_Below;
static mu_Rect PAP_Extra, PAP_Playlist;
static mu_Rect PAP_InfoFrame, PAP_Dirs;
static mu_Rect PAP_Popup;

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
  mu_Id ButtonID = mu_get_id(Context, Name, sizeof(Name));
  
  mu_Rect Rect = mu_layout_next(Context);
  mu_Rect MainRect = {Rect.x + 20, Rect.y, Rect.w - 20, Rect.h};
  mu_Rect Slider = {Rect.x, Rect.y, 15, Rect.h}; /* Might need a better name for this variable */

  mu_update_control(Context, ButtonID, MainRect, 0);

  int Result = 0;
  
  if (Context->mouse_pressed == MU_MOUSE_LEFT && Context->focus == ButtonID) {
    PlayAudio(Audio[AudioID].Path);
    Result |= MU_RES_CHANGE;
  } else if (Context->mouse_pressed == MU_MOUSE_RIGHT && Context->focus == ButtonID && AudioID != AudioCurrentIndex) {
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
      if (mu_mouse_over(Context, (mu_Rect){Slider.x, Slider.y - Slider.h - 5, Slider.w, Slider.h})) {
        Audio[AudioID].LayoutOrder -= 1;
        Audio[LowerAudio].LayoutOrder += 1;
      }
    }

    if (UpperAudio > -1) {
      if (mu_mouse_over(Context, (mu_Rect){Slider.x, Slider.y + Slider.h + 5, Slider.w, Slider.h})) {
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
  int Result = 0;
  mu_Rect *l_Rect;

  mu_push_id(Context, &Value, sizeof(Value));

  /* Get the next rect, save it and push it back */
  mu_Rect Rect = mu_layout_next(Context);
  l_Rect = &Rect;
  mu_layout_set_next(Context, Rect, 0);

  Result = mu_slider_ex(Context, Value, Low, High, 0, "%.2f", MU_OPT_ALIGNCENTER);

  if (Context->scroll_delta.y != 0 && mu_mouse_over(Context, *l_Rect)) {
    *Value += Context->scroll_delta.y / (Context->scroll_delta.y / 2) * (Context->scroll_delta.y > 0 ? -1 : 1);
    Result |= MU_RES_CHANGE;
  }

  mu_pop_id(Context);
  return Result;
}

void InitializeGUI() {
  for (int i = 0; i < PAP_MAX_AUDIO; i++)
    PlaylistWidths[i] = PLAYLIST_WIDTH - 25;
  
  PAP_Title = (mu_Rect){0, 0, WINDOW_WIDTH, 20};
  PAP_Below = (mu_Rect){0, WINDOW_HEIGHT - BELOW_HEIGHT, WINDOW_WIDTH, BELOW_HEIGHT};
  PAP_Playlist = (mu_Rect){WINDOW_WIDTH / 2 - PLAYLIST_WIDTH / 2, WINDOW_HEIGHT / 2 - PLAYLIST_HEIGHT / 2, PLAYLIST_WIDTH, PLAYLIST_HEIGHT};
  PAP_Extra = (mu_Rect){0, WINDOW_HEIGHT - BELOW_HEIGHT - EXTRA_HEIGHT, WINDOW_WIDTH, EXTRA_HEIGHT};
  PAP_InfoFrame = (mu_Rect){WINDOW_WIDTH / 2 - INFO_WIDTH / 2, WINDOW_HEIGHT / 2 - INFO_HEIGHT / 2, INFO_WIDTH, INFO_HEIGHT};
  PAP_Dirs = (mu_Rect){PAP_Playlist.x, PAP_Playlist.y - DIRS_LIST_HEIGHT - 3, DIRS_LIST_WIDTH, DIRS_LIST_HEIGHT};
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
    mu_layout_row(Context, 4, (int[]){100, 100, 300, 100}, 25);

    if (mu_button(Context, "Choose directory")) {
      const char *Path = OpenDialogue(PFD_DIRECTORY);
      size_t PathLen = strlen(Path);

      if (Path) {
        #ifndef WINDOWS
        DIR *Directory;
        struct dirent *Entry; // I guess the name is right

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
              AddAudio(FullPath);
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

              AddAudio(AudioPath);
            }
          } while (FindNextFile(HandleFind, &FileData) != 0);
        } else {
          SDL_Log("INVALID_HANDLE_VALUE returned by FindFirstFile.");
        }
        #endif
      }
    }

    if (mu_button(Context, "Choose file")) {
      const char *Path = OpenDialogue(PFD_FILE);

      if (Path)
        AddAudio((char *)Path);
      else
        SDL_Log("Path is NULL.\n");
    }
    
    mu_layout_set_next(Context, (mu_Rect){WINDOW_WIDTH / 2 - 150, 0, 300, 25}, 1);
    if (mu_slider_ex(Context, &l_AudioPosition, 0, AudioDuration, 0, MU_SLIDER_FMT, MU_OPT_ALIGNCENTER)) {
      // Mix_PauseMusic();
      AudioPosition = (double)l_AudioPosition;
      Mix_SetMusicPosition(AudioPosition);
    }

    l_AudioPosition = AudioPosition;
    
    mu_layout_set_next(Context, (mu_Rect){WINDOW_WIDTH - 110, 0, 100, 25}, 1);
    if (PAP_Slider(Context, &AudioFloat, 0, 128)) {
      AudioVolume = (int)AudioFloat;
      Mix_VolumeMusic(AudioVolume);
    }

    mu_end_window(Context);
  }
  
  /* Playlist */
  if (mu_begin_window_ex(Context, "Playlist", PAP_Playlist, PlaylistOpt)) {
    mu_Container *Container = mu_get_container(Context, "Menu");
    if (SelectedAudio == -1) {Container->open = 0;}
    
    AudioData l_Audio[PAP_MAX_AUDIO];
    uint8_t l_AudioIDs[PAP_MAX_AUDIO];

    memset(l_Audio, 0, sizeof(AudioData) * PAP_MAX_AUDIO);
    memset(l_AudioIDs, 0, sizeof(uint8_t) * PAP_MAX_AUDIO);

    for (int i = 0; i < PAP_MAX_AUDIO; i++) {
      if (Audio[i].Path[0] != 0) {
        l_Audio[Audio[i].LayoutOrder] = Audio[i];
        l_AudioIDs[Audio[i].LayoutOrder] = i;
      }
    }

    for (int i = 0; i < PAP_MAX_AUDIO; i++) {
      if (l_Audio[i].Path[0] == 0)
        continue;

      mu_layout_row(Context, 0, NULL, 25);
      mu_layout_width(Context, PLAYLIST_WIDTH - 25);

      PAP_AudioButton(Context, l_Audio[i].Title, l_AudioIDs[i]);
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
  if (mu_begin_window_ex(Context, "DIRS", PAP_Dirs, DirsOpt)) {
    if (mu_button(Context, "All")) {

    }

    mu_end_window(Context);
  }

  /* Extra */
  if (mu_begin_window_ex(Context, "EXTRA", PAP_Extra, ExtraOpt)) {
    mu_Rect InteractionRect = (mu_Rect){WINDOW_WIDTH / 2 - 105, 10, 100, 20};
    mu_Rect LoopRect = (mu_Rect){InteractionRect.x + 105, InteractionRect.y, InteractionRect.w, InteractionRect.h};

    mu_layout_set_next(Context, InteractionRect, 1);
    
    if (mu_button_ex(Context, InteractButtonText, 0, MU_OPT_ALIGNCENTER)) {
      if (Mix_PausedMusic()) {
        Mix_ResumeMusic();
        InteractButtonText = "Pause";
      } else {
        Mix_PauseMusic();
        InteractButtonText = "Resume";
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
    
    if (mu_button_ex(Context, "Abort", 0, MU_OPT_ALIGNCENTER)) {
      PopupContainer->open = 0;
    }

    mu_end_window(Context);
  }

  InfoContainer->open = InfoOpen;
  InfoContainer->zindex = 2;
  
  /* INFO */
  if (mu_begin_window_ex(Context, "INFO", PAP_InfoFrame, InfoFrameOpt)) {
    Context->hover_root = Context->next_hover_root = InfoContainer;
    mu_bring_to_front(Context, InfoContainer);

    char ArtistBuf[128 + 8];
    char CopyrightBuf[128 + 11];
    char AlbumBuf[128 + 7];

    memset(ArtistBuf, 0, 136);
    memset(CopyrightBuf, 0, 139);
    memset(AlbumBuf, 0, 135);

    memcpy(ArtistBuf, "Artist: ", 8);
    memcpy(ArtistBuf + 8, Audio[SelectedAudio].TagArtist, strlen(Audio[SelectedAudio].TagArtist));
    
    memcpy(CopyrightBuf, "Copyright: ", 11);
    memcpy(CopyrightBuf + 11, Audio[SelectedAudio].TagCopyright, strlen(Audio[SelectedAudio].TagCopyright));
    memcpy(AlbumBuf, "Album: ", 7);
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
