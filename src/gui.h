#ifndef __SAGUI__
#define __SAGUI__

#include "microui.h"
#include "render.h"

#define BELOW_HEIGHT      80
#define INFO_WIDTH        300
#define INFO_HEIGHT       200
#define CATEGORY_WIDTH    80
#define CATEGORY_HEIGHT   WINDOW_HEIGHT - BELOW_HEIGHT
#define POPUP_WIDTH       250
#define POPUP_HEIGHT      80
#define PLAYLIST_WIDTH    WINDOW_WIDTH - CATEGORY_WIDTH - 5
#define PLAYLIST_HEIGHT   WINDOW_HEIGHT - BELOW_HEIGHT - 30 /* -30 for the SEARCH_HEIGHT */
#define SEARCH_WIDTH      PLAYLIST_WIDTH
#define SEARCH_HEIGHT     30

#define SA_MAX_CATEGORIES 32

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
