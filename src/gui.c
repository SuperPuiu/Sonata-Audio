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
#include <SDL3/SDL_mixer.h>
#endif

#include <string.h>

float Background[3] = { 44, 44, 44 };

int LoopStatus = LOOP_NONE;
static int PlaylistWidths[PAP_MAX_SONGS];
static int TitleOpt = MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOCLOSE | MU_OPT_NOINTERACT;
static int BelowOpt = MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NOFRAME;
static int PlaylistOpt = MU_OPT_NOCLOSE | MU_OPT_NOTITLE;
static int ExtraOpt = MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NOFRAME | MU_OPT_NOSCROLL;

float AudioPosition;
static float AudioFloat = MIX_MAX_VOLUME;

static mu_Rect PAP_Title;
static mu_Rect PAP_Below;
static mu_Rect PAP_Extra;
static mu_Rect PAP_Playlist;

static const char *InteractButtonText = "Pause";
static const char *LoopButtonText = "No loop";

int TextWidth(mu_Font font, const char *text, int len) {
  if (len == -1) { len = strlen(text); }
  return r_get_text_width(text, len);
}

int TextHeight(mu_Font font) {
  return r_get_text_height();
}

void InitializeGUI() {
  for (int i = 0; i < PAP_MAX_SONGS; i++)
    PlaylistWidths[i] = PLAYLIST_WIDTH - 25;
  
  PAP_Title = mu_rect(0, 0, WINDOW_WIDTH, 20);
  PAP_Below = mu_rect(0, WINDOW_HEIGHT - BELOW_HEIGHT, WINDOW_WIDTH, BELOW_HEIGHT);
  PAP_Playlist = mu_rect(WINDOW_WIDTH / 2 - PLAYLIST_WIDTH / 2, WINDOW_HEIGHT / 2 - PLAYLIST_HEIGHT / 2, PLAYLIST_WIDTH, PLAYLIST_HEIGHT);
  PAP_Extra = mu_rect(0, WINDOW_HEIGHT - BELOW_HEIGHT - EXTRA_HEIGHT, WINDOW_WIDTH, EXTRA_HEIGHT);
}

void MainWindow(mu_Context *Context) {
  /* Title */
  if (mu_begin_window_ex(Context, "Puius Audio Player", PAP_Title, TitleOpt)) {
    mu_end_window(Context);
  }
  
  /* Below */
  if (mu_begin_window_ex(Context, "BELOW", PAP_Below, BelowOpt)) {
    mu_layout_row(Context, 4, (int[]){100, 100, 300, 100}, 25);

    if (mu_button(Context, "Choose directory")) {
      const char *Path = OpenDialogue(PFD_DIRECTORY);

      if (Path) {
        #ifndef WINDOWS
        DIR *Directory;
        struct dirent *Entry; // I guess the name is right

        if ((Directory = opendir(Path)) != NULL) {
          size_t PathLen = strlen(Path);
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
              AddSong(FullPath);
          }

          closedir(Directory);
        } else {
          SDL_Log("Directory is NULL.");
        }
        #endif
      }
    }

    if (mu_button(Context, "Choose file")) {
      const char *Path = OpenDialogue(PFD_FILE);

      if (Path)
        AddSong((char *)Path);
      else
        SDL_Log("Path is NULL.\n");
    }
    
    mu_layout_set_next(Context, mu_rect(WINDOW_WIDTH / 2 - 150, 0, 300, 25), 1);
    if (mu_slider_ex(Context, &AudioPosition, 0, MusicDuration, 0, MU_SLIDER_FMT, MU_OPT_ALIGNCENTER)) {
      // Mix_PauseMusic();
      CurrentPosition = (double)AudioPosition;
      Mix_SetMusicPosition(CurrentPosition);
    }

    AudioPosition = (float)CurrentPosition;
    
    mu_layout_set_next(Context, mu_rect(WINDOW_WIDTH - 110, 0, 100, 25), 1);
    if (mu_slider_ex(Context, &AudioFloat, 0, 128, 0, MU_SLIDER_FMT, MU_OPT_ALIGNCENTER)) {
      AudioVolume = (int)AudioFloat;
      Mix_VolumeMusic(AudioVolume);
    }

    mu_end_window(Context);
  }
  
  /* Playlist */
  if (mu_begin_window_ex(Context, "Playlist", PAP_Playlist, PlaylistOpt)) {
    for (int i = 0; i < PAP_MAX_SONGS; i++) {
      if (Songs[i][0] == 0)
        continue;
      
      mu_layout_row(Context, 0, NULL, 25);
      mu_layout_width(Context, PLAYLIST_WIDTH - 25);
      if (mu_button(Context, SongTitles[i]))
        PlayAudio(Songs[i]);
    }
    
    mu_end_window(Context);
  }

  /* Extra */
  if (mu_begin_window_ex(Context, "EXTRA", PAP_Extra, ExtraOpt)) {
    mu_Rect InteractionRect = mu_rect(WINDOW_WIDTH / 2 - 105, 10, 100, 20);
    mu_Rect LoopRect = mu_rect(InteractionRect.x + 105, InteractionRect.y, InteractionRect.w, InteractionRect.h);

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
}

void ProcessContextFrame(mu_Context *Context) {
  mu_begin(Context);
  MainWindow(Context);
  mu_end(Context);
}
