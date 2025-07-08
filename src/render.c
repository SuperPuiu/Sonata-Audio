/* MIT License
 * Copyright (c) 2025 SuperPuiu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

#include <SDL3/SDL.h>
#include <assert.h>

#include <stdint.h>

#include "render.h"
#include "atlas.inl"

#include "window.h"

#define BUFFER_SIZE 512

static uint32_t Background;

static mu_Rect Clip = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
static mu_Rect TextureBuffer[BUFFER_SIZE];
static mu_Rect SourceBuffer[BUFFER_SIZE];
static mu_Color ColorBuffer[BUFFER_SIZE];

static uint32_t BufferIndex = 0;

SDL_Window *ProgramWindow;

static inline uint32_t ColorToNumber(mu_Color Color) {
  return ((uint32_t)Color.a << 24) | ((uint32_t)Color.r << 16) | ((uint32_t)Color.g << 8) | Color.b;
}

static inline mu_Color NumberToColor(uint32_t Color) {
  return mu_color((Color >> 16) & 0xff, (Color >> 8) & 0xff, Color & 0xff, (Color >> 24) & 0xff);
}

static inline bool InRectangle(mu_Rect Rect, int X, int Y) {
  return (X >= Rect.x && X < Rect.x + Rect.w) && (Y >= Rect.y && Y < Rect.y + Rect.h);
}

static inline uint8_t GetAtlasColor(mu_Rect *Texture, int X, int Y) {
  if (X >= Texture->w || Y >= Texture->h || Y >= Y * ATLAS_WIDTH + X)
    return 0x00;

  X += Texture->x;
  Y += Texture->y;

  return atlas_texture[Y * ATLAS_WIDTH + X];
}

static inline mu_Color BlendPixel(mu_Color Destination, mu_Color Source) {
  uint8_t ia = 0xff - Source.a;
  Destination.r = ((Source.r * Source.a) + (Destination.r * ia)) >> 8;
  Destination.g = ((Source.g * Source.a) + (Destination.g * ia)) >> 8;
  Destination.b = ((Source.b * Source.a) + (Destination.b * ia)) >> 8;
  return Destination;
}

void r_init(void) {
  OpenWindow();
  Background = ColorToNumber(mu_color(33, 33, 33, 255));
  ProgramWindow = CreatedWindow;
}

void FlushBuffers(void) {
  for (uint32_t i = 0; i < BufferIndex; i++) {
    mu_Rect *Source = &SourceBuffer[i];
    mu_Rect *Texture = &TextureBuffer[i];

    uint16_t Y = mu_max(Source->y, Clip.y);
    uint16_t X = mu_max(Source->x, Clip.x);
    uint16_t Width = mu_min(Source->x + Source->w, Clip.x + Clip.w);
    uint16_t Height = mu_min(Source->y + Source->h, Clip.y + Clip.h);
    
    uint32_t WindowArea = WINDOW_WIDTH * WINDOW_HEIGHT;

    for (uint16_t CurrentY = Y; CurrentY < Height; CurrentY++) {
      for (uint16_t CurrentX = X; CurrentX < Width; CurrentX++) {
        uint32_t PixelLocation = CurrentY * WINDOW_WIDTH + CurrentX;
        if (PixelLocation > WindowArea)
          continue;

        /* Textures */
        if (Source->w == Texture->w && Source->h == Texture->h) {
          uint8_t TextureAlpha = GetAtlasColor(Texture, CurrentX - Source->x, CurrentY - Source->y);
          uint32_t CurrentPixel = SA_GetPixel(CurrentX, CurrentY);
          mu_Color Color = {ColorBuffer[i].r, ColorBuffer[i].g, ColorBuffer[i].b, TextureAlpha};
          SA_PutPixel(CurrentX, CurrentY, ColorToNumber(BlendPixel(NumberToColor(CurrentPixel), Color)));
          /* Other */
        } else {
          mu_Color NewColor = BlendPixel(NumberToColor(SA_GetPixel(CurrentX, CurrentY)), ColorBuffer[i]);
          SA_PutPixel(CurrentX, CurrentY, ColorToNumber(NewColor));
        }

      }
    }
  }

  BufferIndex = 0;
}

void PushRectangle(mu_Rect Source, mu_Rect Texture, mu_Color Color) {
  if (BufferIndex == BUFFER_SIZE)
    FlushBuffers();

  TextureBuffer[BufferIndex] = Texture;
  SourceBuffer[BufferIndex] = Source;
  ColorBuffer[BufferIndex] = Color;

  BufferIndex += 1;
}

void r_set_clip_rect(mu_Rect Rect) {
  FlushBuffers();

  uint32_t Y = mu_max(0, Rect.y);
  uint32_t X = mu_max(0, Rect.x);
  uint32_t Height = mu_min(WINDOW_HEIGHT, Rect.y + Rect.h) - Y;
  uint32_t Width = mu_min(WINDOW_WIDTH, Rect.x + Rect.w) - X;

  Clip = (mu_Rect){X, Y, Width, Height};
}

void r_draw_rect(mu_Rect Rect, mu_Color Color) {
  PushRectangle(Rect, atlas[ATLAS_WHITE], Color);
}

void r_draw_text(const char *Text, mu_Vec2 Position, mu_Color Color) {
  mu_Rect Destination = {Position.x, Position.y, 0, 0};

  for (const char *Pointer = Text; *Pointer; Pointer++) {
    if ((*Pointer & 0xc0) == 0x80) 
      continue;

    int32_t Character = mu_min((unsigned char) *Pointer, 127);
    mu_Rect Source = atlas[ATLAS_FONT + Character];
    Destination.w = Source.w;
    Destination.h = Source.h;
    
    if (Destination.x <= WINDOW_WIDTH && Destination.x > 0 && Destination.y <= WINDOW_HEIGHT && Destination.y > 0)
      PushRectangle(Destination, Source, Color);

    Destination.x += Destination.w;
  }
}

void r_draw_icon(int IconID, mu_Rect Rect, mu_Color Color) {
  mu_Rect Source = atlas[IconID];

  uint32_t X = Rect.x + (Rect.w - Source.w) / 2;
  uint32_t Y = Rect.y + (Rect.h - Source.h) / 2;

  PushRectangle((mu_Rect){X, Y, Source.w, Source.h}, Source, Color);
}

int r_get_text_width(const char *Text, int Length) {
  int32_t Width = 0;
  
  for (const char *Pointer = Text; *Pointer && Length--; Pointer++) {
    if ((*Pointer & 0xc0) == 0x80)
      continue;

    int Character = mu_min((unsigned char)*Pointer, 127);
    Width += atlas[ATLAS_FONT + Character].w;
  }
  
  return Width;
}

int r_get_text_height(void) {
  return 18;
}

void r_clear(void) {
  FlushBuffers();
  ClearWindow(Background);
}

void r_present(void) {
  FlushBuffers();
  RefreshWindow();
}
