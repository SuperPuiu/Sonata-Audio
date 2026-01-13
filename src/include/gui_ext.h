#ifndef __GUIEXT__
#define __GUIEXT__

#include "microui.h"

int SA_AudioButton(mu_Context *Context, const char *Name, int AudioID);
int SA_CategoryButton(mu_Context *Context, const char *Text, int Opt);
int SA_Slider(mu_Context *Context, mu_Real *Value, int Low, int High);

/* gui.c */
void RefreshPlaylist();

extern int SelectedAudio;

#endif
