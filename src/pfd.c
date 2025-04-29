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
    pclose(FilePointer);
    return ResultBuffer;
  }
  
  pclose(FilePointer);

  return NULL;
}
#else
#include <windows.h>
#include <shlobj.h>
#include <stdbool.h>

char ResultBuffer[MAX_PATH];
LPITEMIDLIST PidlRoot = NULL;

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

void PopulateBrowseInfo(BROWSEINFO *BI) {
  BI->hwndOwner = GetHWND();
  BI->pidlRoot = PidlRoot;
  BI->pszDisplayName = ResultBuffer;
  BI->lpszTitle = "Select a folder";
  BI->ulFlags = 0;
  BI->lpfn = NULL;
  BI->lParam = 0;
  BI->iImage = -1;
}

const char *OpenDialogue(enum DialogueOption Option) {
  if (Option == PFD_DIRECTORY) {
    BROWSEINFO BI = {0};
    LPITEMIDLIST Pidl;
    bool Status = false;

    PopulateBrowseInfo(&BI);
    Pidl = SHBrowseForFolder(&BI);
    Status = SHGetPathFromIDList(Pidl, ResultBuffer);
    
    if (PidlRoot) {
      CoTaskMemFree(PidlRoot);
      PidlRoot = NULL;
    }
    
    return Status == true ? ResultBuffer : NULL;
  } else {
    OPENFILENAME OFN;
    PopulateOFN(&OFN);
  
    return GetOpenFileName(&OFN) == TRUE ? OFN.lpstrFile : NULL;
  }
}

#endif
