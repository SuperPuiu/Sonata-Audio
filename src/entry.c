#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "discord.h"
#include "audio.h"
#include "render.h"
#include "microui.h"
#include "map.h"
#include "gui.h"

#define REFRESH_EVENT (SDL_EVENT_USER + 1)

bool Running = true;

int AudioThread(void *NULLABLE) {
  while (Running) {
    SDL_Event Event = {.type = REFRESH_EVENT};

    UpdateAudioPosition();

    SDL_PushEvent(&Event);
    SDL_Delay(200);
  }

  unused(NULLABLE);
  return 0;
}

int main(int argc, char **argv) {
  SDL_Thread *AudioThreadID;

  setenv("SDL_VIDEODRIVER", "x11", 1);

  if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    SDL_Log("%s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  MIX_Init();
  r_init();
  InitializeAudio();
  InitializeGUI();
  InitializeRPC();

  mu_Context *Context = malloc(sizeof(mu_Context));
  mu_init(Context);

  Context->text_width = TextWidth;
  Context->text_height = TextHeight;

  if (argc > 1) {
    AddAudio(argv[1], NULL, NULL);
    PlayAudio(argv[1]);

    for (int i = 2; i < argc; i++)
      AddAudio(argv[i], NULL, NULL);
  }

  AudioThreadID = SDL_CreateThread(AudioThread, "AudioThread", NULL);

  while (Running) {
    SDL_Event Event;
    SDL_WaitEvent(&Event);

    do {
      switch(Event.type) {
        case SDL_EVENT_QUIT:
          Running = false;
          goto exit;
          break;
        case SDL_EVENT_MOUSE_MOTION:
          mu_input_mousemove(Context, Event.motion.x, Event.motion.y);
          break;
        case SDL_EVENT_MOUSE_WHEEL:
          mu_input_scroll(Context, 0, Event.wheel.y * -30);
          break;
        case SDL_EVENT_TEXT_INPUT:
          if (!(SDL_GetModState() & SDL_KMOD_CTRL && (Event.text.text[0] != 'v' || Event.text.text[0] != 'V')))
            mu_input_text(Context, Event.text.text);
          break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
          int b = button_map[Event.button.button & 0xff];
          if (b && Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {mu_input_mousedown(Context, Event.button.x, Event.button.y, b);}
          else if (b && Event.type == SDL_EVENT_MOUSE_BUTTON_UP) {mu_input_mouseup(Context, Event.button.x, Event.button.y, b);}
          break;
        }

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
          int c = key_map[Event.key.key & 0xff];
          if (c && Event.type == SDL_EVENT_KEY_DOWN) {mu_input_keydown(Context, c);}
          else if (c && Event.type == SDL_EVENT_KEY_UP) {mu_input_keyup(Context, c);}

          if (Event.key.key == SDLK_V && SDL_GetModState() & SDL_KMOD_CTRL && Event.type == SDL_EVENT_KEY_DOWN) {
            char *l_Buffer = SDL_GetClipboardText();

            if (strlen(l_Buffer) < sizeof(Context->input_text))
              mu_input_text(Context, l_Buffer);

            SDL_free(l_Buffer);
          };
          break;
        }
      }
    } while (SDL_PollEvent(&Event));

    /* process frame */
    ProcessContextFrame(Context);

    /* render */
    r_clear();

    mu_Command *cmd = NULL;
    while (mu_next_command(Context, &cmd)) {
      switch (cmd->type) {
        case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
        case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
        case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
        case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
        case MU_COMMAND_INPUT:
          if (cmd->input.status)
            SDL_StartTextInput(ProgramWindow);
          else
            SDL_StopTextInput(ProgramWindow);
          break;
      }
    }

    r_present();

  exit:
  }

  SDL_WaitThread(AudioThreadID, NULL);

  free(Context);
  SDL_Quit();
  ShutdownRPC();

  return 0;
}
