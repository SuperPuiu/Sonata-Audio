#ifndef RENDERER_H
#define RENDERER_H

#include "microui.h"
#include <stdbool.h>
#include <SDL3/SDL.h>

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

extern bool Running;
extern SDL_Window *ProgramWindow;

void InitializeRender(void);
void RenderDrawRect(mu_Rect rect, mu_Color color);
void RenderDrawText(const char *text, mu_Vec2 pos, mu_Color color);
void RenderDrawIcon(int IconID, mu_Rect Rect, mu_Color Color);
 int RenderGetTextWidth(const char *Text, int Length);
 int RenderGetTextHeight(void);
void RenderSetClipRect(mu_Rect rect);
void RenderQuit();
void RenderClear();
void RenderPresent(void);

#endif
