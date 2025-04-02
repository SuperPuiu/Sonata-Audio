#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>

#include "pfd.h"

#ifndef WINDOWS
#include <limits.h>
char ResultBuffer[PATH_MAX];

const char *OpenDialogue(enum DialogueOption Option) {
  FILE *FilePointer;
  
  if (Option == PFD_DIRECTORY)
    FilePointer = popen("zenity --file-selection --directory", "r");
  else
    FilePointer = popen("zenity --file-selection", "r");

  if (!FilePointer) {
    printf("error: %s\n", strerror(errno));
    return NULL;
  }

  size_t Length = fread(ResultBuffer, 1, sizeof(ResultBuffer) - 1, FilePointer);
  
  if (Length > 0) {
    ResultBuffer[Length - 1] = '\0';
    return ResultBuffer;
  }

  return NULL;
}
#else
#include <windows.h>

char ResultBuffer[320];

struct WindowInfo {
  unsigned long ProcessId;
  void *HandleRoot;
  void *HandleFirst;
};

int EnumCallback(HWND Handle, LPARAM Param) {
  struct WindowInfo *Info = (struct WindowInfo*)Param;

  unsigned long ProcessId;
  GetWindowThreadProcessId(Handle, &ProcessId);

  if (Info->ProcessId == ProcessId) {
    Info->HandleFirst = Handle;
    
    if (GetWindow(Handle, GW_OWNER) == 0 && IsWindowVisible(Handle)) {
      Info->HandleRoot = Handle;
      return 0;
    }
  }

  return 1;
}

HWND GetHWND() {
  struct WindowInfo Info = {
    .ProcessId = GetCurrentProcessId()
  };
  
  EnumWindows(EnumCallback, (LPARAM)&Info);
  return Info.HandleRoot;
}

void PopulateOFN(OPENFILENAME *OFN) {
  memset(OFN, 0, sizeof(*OFN));
  OFN->lStructSize      = sizeof(*OFN);
  OFN->hwndOwner        = GetHWND();
  OFN->lpstrFile        = ResultBuffer;
  OFN->lpstrFile[0]     = '\0';
  OFN->nMaxFile         = sizeof(ResultBuffer);
  OFN->lpstrFilter      = "All\0";
  OFN->nFilterIndex     = 1;
  OFN->lpstrFileTitle   = NULL;
  OFN->nMaxFileTitle    = 0;
  OFN->lpstrInitialDir  = NULL;
  OFN->lpstrDefExt      = NULL;
  OFN->Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

const char *OpenDialogue(enum DialogueOption Option) {
  OPENFILENAME OFN;
  PopulateOFN(&OFN);
  
  // TODO: Make choosing directories work too
  return GetOpenFileName(&OFN) == TRUE ? OFN.lpstrFile : NULL;
}

#endif
