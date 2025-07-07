#ifndef RENDERER_H
#define RENDERER_H

#include "microui.h"
#include <stdbool.h>
#include <SDL3/SDL.h>

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

extern bool Running;
extern SDL_Window *ProgramWindow;

void r_init(void);
void r_draw_rect(mu_Rect rect, mu_Color color);
void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color);
void r_draw_icon(int IconID, mu_Rect Rect, mu_Color Color);
 int r_get_text_width(const char *Text, int Length);
 int r_get_text_height(void);
void r_set_clip_rect(mu_Rect rect);
void r_clear();
void r_present(void);

#endif
