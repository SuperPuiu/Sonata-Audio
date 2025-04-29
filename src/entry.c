#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio.h"
#include "render.h"
#include "microui.h"
#include "map.h"
#include "gui.h"

bool Running = true;
unsigned int FPS = 240, DefaultFPS = 240;

int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  r_init();
  InitializeAudio();
  InitializeGUI();

  mu_Context *Context = malloc(sizeof(mu_Context));
  mu_init(Context);

  Context->text_width = TextWidth;
  Context->text_height = TextHeight;

  if (argc > 1) {
    AddAudio(argv[1], NULL);
    PlayAudio(argv[1]);

    for (int i = 2; i < argc; i++)
      AddAudio(argv[i], NULL);
  }

  while (Running) {
    uint64_t Start = SDL_GetPerformanceCounter();

    UpdateAudioPosition();
    SDL_Event Event;

    while(SDL_PollEvent(&Event)) {
      switch(Event.type) {
        case SDL_EVENT_QUIT:
          Running = false;
          break;
        case SDL_EVENT_MOUSE_MOTION:
          mu_input_mousemove(Context, Event.motion.x, Event.motion.y);
          break;
        case SDL_EVENT_MOUSE_WHEEL:
          mu_input_scroll(Context, 0, Event.wheel.y * -30);         
          break;
        case SDL_EVENT_TEXT_INPUT:
          mu_input_text(Context, Event.text.text);
          break;
        
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
          int b = button_map[Event.button.button & 0xff];
          if (b && Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {mu_input_mousedown(Context, Event.button.x, Event.button.y, b);}
          if (b && Event.type == SDL_EVENT_MOUSE_BUTTON_UP) {mu_input_mouseup(Context, Event.button.x, Event.button.y, b);}
          break;
        }

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
          int c = key_map[Event.key.key & 0xff];
          if (c && Event.type == SDL_EVENT_KEY_DOWN) {mu_input_keydown(Context, c);}
          if (c && Event.type == SDL_EVENT_KEY_UP) {mu_input_keyup(Context, c);}
          break;
        }
        
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
          if (Event.window.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
            FPS = DefaultFPS;
          else if (Event.window.type == SDL_EVENT_WINDOW_FOCUS_LOST)
            FPS = 5;
          break;
        }
      }
    }

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
      }
    }

    r_present();
    
    uint64_t End = SDL_GetPerformanceCounter();
    float Elapsed = (End - Start) / (float)SDL_GetPerformanceFrequency();

    if (Elapsed > 0 && ((1000 / FPS) - Elapsed) > 0)
      SDL_Delay((1000 / FPS) - Elapsed);
  }

  free(Context);
  SDL_Quit();

  return 0;
}
