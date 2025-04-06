#include <SDL3/SDL.h>
#include <assert.h>

#include <stdint.h>

#include "render.h"
#include "atlas.inl"

#include "window.h"

#define BUFFER_SIZE 512

typedef struct {
  mu_Rect Texture;
  mu_Rect Source;
  mu_Color Color;
} TextureData;

static uint32_t Background;

static mu_Rect Clip;

TextureData TextData[BUFFER_SIZE];
TextureData OtherData[BUFFER_SIZE];

static unsigned int OtherIndex = 0;
static unsigned int TextIndex = 0;

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
  assert(X < Texture->w);
  X += Texture->x;
  assert(Y < Texture->h);
  Y += Texture->y;

  return atlas_texture[Y * ATLAS_WIDTH + X];
}

static inline mu_Color BlendPixel(mu_Color Destination, mu_Color Source) {
  int ia = 0xff - Source.a;
  Destination.r = ((Source.r * Source.a) + (Destination.r * ia)) >> 8;
  Destination.g = ((Source.g * Source.a) + (Destination.g * ia)) >> 8;
  Destination.b = ((Source.b * Source.a) + (Destination.b * ia)) >> 8;

  return Destination;
}

void r_init() {
  OpenWindow();
  Background = ColorToNumber(mu_color(44, 44, 44, 255));
  Clip = mu_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void FlushBuffers() {
  for (unsigned int i = 0; i < OtherIndex; i++) {
    mu_Rect *Source = &OtherData[i].Source;

    unsigned int Y = mu_max(Source->y, Clip.y);
    unsigned int X = mu_max(Source->x, Clip.x);
    unsigned int Width = mu_min(Source->x + Source->w, Clip.x + Clip.w);
    unsigned int Height = mu_min(Source->y + Source->h, Clip.y + Clip.h);

    for (unsigned int CurrentY = Y; CurrentY < Height; CurrentY++) {
      for (unsigned int CurrentX = X; CurrentX < Width; CurrentX++) {
        assert(InRectangle(*Source, CurrentX, CurrentY));
        assert(InRectangle(Clip, CurrentX, CurrentY));
        
        PAP_PutPixel(CurrentX, CurrentY, ColorToNumber(OtherData[i].Color));
      }
    }
  }

  for (unsigned int i = 0; i < TextIndex; i++) {
    mu_Rect *Source = &TextData[i].Source;
    mu_Rect *Texture = &TextData[i].Texture;

    unsigned int Y = mu_max(Source->y, Clip.y);
    unsigned int X = mu_max(Source->x, Clip.x);
    unsigned int Width = mu_min(Source->x + Source->w, Clip.x + Clip.w);
    unsigned int Height = mu_min(Source->y + Source->h, Clip.y + Clip.h);

    for (unsigned int CurrentY = Y; CurrentY < Height; CurrentY++) {
      for (unsigned int CurrentX = X; CurrentX < Width; CurrentX++) {
        assert(InRectangle(*Source, CurrentX, CurrentY));
        assert(InRectangle(Clip, CurrentX, CurrentY));
        
        uint8_t TextureColor = GetAtlasColor(Texture, CurrentX - Source->x, CurrentY - Source->y);
        uint32_t CurrentPixel = PAP_GetPixel(CurrentX, CurrentY);
        PAP_PutPixel(CurrentX, CurrentY, CurrentPixel | ColorToNumber(mu_color(TextureColor, TextureColor, TextureColor, 255)));
      }
    }
  }
 
  
  TextIndex = 0;
  OtherIndex = 0;
}

void PushRectangle(mu_Rect Source, mu_Rect Texture, mu_Color Color, bool IsText) {
  if (OtherIndex == BUFFER_SIZE || TextIndex == BUFFER_SIZE)
    FlushBuffers();
  
  unsigned int *Index = IsText ? &TextIndex : &OtherIndex;
  TextureData *Data = IsText ? TextData : OtherData;
  
  Data[*Index].Texture = Texture;
  Data[*Index].Source = Source;
  Data[*Index].Color = Color;
  
  ++*Index;
}

void r_set_clip_rect(mu_Rect Rect) {
  FlushBuffers();

  unsigned int Y = mu_max(0, Rect.y);
  unsigned int X = mu_max(0, Rect.x);
  unsigned int Height = mu_min(WINDOW_HEIGHT, Rect.y + Rect.h) - Y;
  unsigned int Width = mu_min(WINDOW_WIDTH, Rect.x + Rect.w) - X;

  Clip = mu_rect(X, Y, Width, Height);
}

void r_draw_rect(mu_Rect Rect, mu_Color Color) {
  PushRectangle(Rect, atlas[ATLAS_WHITE], Color, false);
}

void r_draw_text(const char *Text, mu_Vec2 Position, mu_Color Color) {
  mu_Rect Destination = {Position.x, Position.y, 0, 0};
  
  for (const char *Pointer = Text; *Pointer; Pointer++) {
    if ((*Pointer & 0xc0) == 0x80) 
      continue;

    int Character = mu_min((unsigned char) *Pointer, 127);
    mu_Rect Source = atlas[ATLAS_FONT + Character];
    Destination.w = Source.w;
    Destination.h = Source.h;
    
    PushRectangle(Destination, Source, Color, true);

    Destination.x += Destination.w;
  }
}

void r_draw_icon(int IconID, mu_Rect Rect, mu_Color Color) {
  mu_Rect Source = atlas[IconID];

  unsigned int X = Rect.x + (Rect.w - Source.w) / 2;
  unsigned int Y = Rect.y + (Rect.h - Source.h) / 2;

  PushRectangle(mu_rect(X, Y, Source.w, Source.h), Source, Color, false);
}

int r_get_text_width(const char *Text, int Length) {
  int Width = 0;
  
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

void r_clear() {
  FlushBuffers();
  ClearWindow(Background);
}

void r_present(void) {
  FlushBuffers();
  RefreshWindow();
}
