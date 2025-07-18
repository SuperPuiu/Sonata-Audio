#include "audio.h"
#include "gui_ext.h"
#include "gui.h"

#include <string.h>

static int SlidingAudio = -1;
static mu_Id CurrentCategoryID = 0;

int32_t SA_GetAudioByOrder(uint8_t RequestedOrder) {
  for (uint32_t i = 0; i < SA_TotalAudio; i++) {
    if (Audio[i].LayoutOrder == RequestedOrder)
      return i;
  }

  return -1;
}

int SA_AudioButton(mu_Context *Context, const char *Name, int AudioID) {
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
    int UpperAudio = SA_GetAudioByOrder(Audio[AudioID].LayoutOrder + 1);
    int LowerAudio = SA_GetAudioByOrder(Audio[AudioID].LayoutOrder - 1);

    if (LowerAudio > -1) {
      if (mu_mouse_over(Context, (mu_Rect){Slider.x, Slider.y - Slider.h - Context->style->padding, Slider.w, Slider.h})) {
        uint32_t LowerOrder = Audio[LowerAudio].LayoutOrder;
        
        Audio[LowerAudio].LayoutOrder = Audio[AudioID].LayoutOrder;
        Audio[AudioID].LayoutOrder = LowerOrder;
      }
    }

    if (UpperAudio > -1) {
      if (mu_mouse_over(Context, (mu_Rect){Slider.x, Slider.y + Slider.h + Context->style->padding, Slider.w, Slider.h})) {
        uint32_t UpperOrder = Audio[UpperAudio].LayoutOrder;
        
        Audio[UpperAudio].LayoutOrder = Audio[AudioID].LayoutOrder;
        Audio[AudioID].LayoutOrder = UpperOrder;
      }
    }

    RefreshPlaylist();
  }
  
  mu_draw_control_frame(Context, ButtonID, Slider, SlidingAudio != AudioID ? MU_COLOR_BUTTON : MU_COLOR_BASE, MU_OPT_NOBORDER);
  mu_draw_control_frame(Context, ButtonID, MainRect, AudioCurrentIndex != AudioID ? MU_COLOR_BUTTON : MU_COLOR_BASE, MU_OPT_NOBORDER);
  mu_draw_control_text(Context, Name, MainRect, MU_COLOR_TEXT, 0);

  return Result;
}

/* Based off of mu_button_ex implementation. */
int SA_CategoryButton(mu_Context *Context, const char *Text, int Opt) {
  int Result = 0;
  mu_Rect Rect = mu_layout_next(Context);
  
  mu_Id ID = mu_get_id(Context, Text, strlen(Text));
  mu_update_control(Context, ID, Rect, Opt);
  
  if (CurrentCategoryID == 0)
    CurrentCategoryID = ID;

  if (Context->mouse_pressed == MU_MOUSE_LEFT && Context->focus == ID) {
    CurrentCategoryID = ID;
    Result |= MU_RES_SUBMIT;
  }
  
  mu_draw_control_frame(Context, ID, Rect, ID != CurrentCategoryID ? MU_COLOR_BUTTON : MU_COLOR_BASE, Opt);
  mu_draw_control_text(Context, Text, Rect, MU_COLOR_TEXT, Opt);

  return Result;
}

int SA_Slider(mu_Context *Context, mu_Real *Value, int Low, int High) {
  int Result = 0, ScrollSpeed = Context->key_down & MU_KEY_SHIFT ? 1 : 2;
  mu_Real Last = *Value, l_Value = Last;
  mu_Rect l_Rect = mu_layout_next(Context);

  mu_push_id(Context, &Value, sizeof(Value));
  mu_Id ID = mu_get_id(Context, &Value, sizeof(Value)); 
  
  mu_update_control(Context, ID, l_Rect, MU_OPT_ALIGNCENTER);
  
  if (Context->focus == ID && (Context->mouse_down | Context->mouse_pressed) == MU_MOUSE_LEFT)
    l_Value = Low + (Context->mouse_pos.x - l_Rect.x) * (High - Low) / l_Rect.w;

  if (Context->scroll_delta.y != 0 && mu_mouse_over(Context, l_Rect))
    l_Value += Context->scroll_delta.y / (Context->scroll_delta.y / ScrollSpeed) * (Context->scroll_delta.y > 0 ? -1 : 1);
  
  if (Last != l_Value) {
    *Value = l_Value = mu_clamp(l_Value, Low, High);
    Result |= MU_RES_CHANGE;
  }
  
  mu_draw_control_frame(Context, ID, l_Rect, MU_COLOR_BASE, MU_OPT_NOINTERACT);
  mu_draw_control_frame(Context, ID, (mu_Rect){l_Rect.x, l_Rect.y, mu_clamp(l_Value / High * l_Rect.w, 0, l_Rect.w), l_Rect.h}, MU_COLOR_TEXT, MU_OPT_NOINTERACT);

  mu_pop_id(Context);
  return Result;
}
