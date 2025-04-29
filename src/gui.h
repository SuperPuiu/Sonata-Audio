#ifndef __PAPGUI__
#define __PAPGUI__

#define BELOW_HEIGHT      35
#define EXTRA_HEIGHT      35
#define PLAYLIST_WIDTH    500
#define PLAYLIST_HEIGHT   300
#define INFO_WIDTH        300
#define INFO_HEIGHT       200
#define CATEGORY_WIDTH    PLAYLIST_WIDTH
#define CATEGORY_HEIGHT   32
#define POPUP_WIDTH       250
#define POPUP_HEIGHT      85

#define PAP_MAX_CATEGORIES 32

#include "microui.h"

enum LoopEnum {
  LOOP_NONE,
  LOOP_SONG,
  LOOP_ALL
};

extern int LoopStatus;
extern char *CurrentCategory;

int TextWidth(mu_Font font, const char *text, int len);
int TextHeight(mu_Font font);
void ProcessContextFrame(mu_Context *Context);
void InitializeGUI();

#endif
