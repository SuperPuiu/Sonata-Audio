/* PickFileDialogue */

#ifndef __PAPPFD__
#define __PAPPFD__

enum DialogueOption {
  PFD_DIRECTORY,
  PFD_FILE
};

const char *OpenDialogue(enum DialogueOption Option);

#endif
