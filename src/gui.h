#ifndef __PAPGUI__
#define __PAPGUI__

#define BELOW_HEIGHT    35
#define EXTRA_HEIGHT    35
#define PLAYLIST_WIDTH  500
#define PLAYLIST_HEIGHT 300
#define INFO_WIDTH 300
#define INFO_HEIGHT 200

#include "microui.h"

enum LoopEnum {
  LOOP_NONE,
  LOOP_SONG,
  LOOP_ALL
};

extern int LoopStatus;

int TextWidth(mu_Font font, const char *text, int len);
int TextHeight(mu_Font font);
void ProcessContextFrame(mu_Context *Context);
void InitializeGUI();

#endif
