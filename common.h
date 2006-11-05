
#include "config.h"

#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/StdSel.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Shell.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xmd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>


typedef struct {
  String  selection_name;
  int     buffer;
  String  debug_option;
  String  verbose_option;
  String  fork_option;
  String  buttonup_option;
  String  kill;
  int     pause;
  int     debug; 
  int     verbose; 
  int     fork;
  Atom    selection;
  char*   value;
  int     length;
  int     own_selection;
  int     buttonup;
} OptionsRec;

extern Widget box;
extern Display* dpy;
extern XtAppContext context;
extern Atom selection;
extern int buffer;
extern OptionsRec options;


void PrintValue(char *value, int length);
Boolean ConvertSelection(Widget w, Atom *selection, Atom *target,
                                Atom *type, XtPointer *value,
                                unsigned long *length, int *format);
