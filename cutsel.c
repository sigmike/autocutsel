/*
 * cutsel.c by Michael Witrant <mike @ lepton . fr>
 * Manipulates the cutbuffer and the selection
 * version 0.1
 * Copyright (c) 2002 Michael Witrant.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This program is distributed under the terms
 * of the GNU General Public License (read the COPYING file)
 * 
 */


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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Widget box;
static Display* dpy;
static XtAppContext context;
static Atom selection;
static int buffer;

static XrmOptionDescRec optionDesc[] = {
  {"-selection", "selection", XrmoptionSepArg, NULL},
  {"-select",    "selection", XrmoptionSepArg, NULL},
  {"-sel",       "selection", XrmoptionSepArg, NULL},
  {"-s",         "selection", XrmoptionSepArg, NULL},
  {"-cutbuffer", "cutBuffer", XrmoptionSepArg, NULL},
  {"-cut",       "cutBuffer", XrmoptionSepArg, NULL},
  {"-c",         "cutBuffer", XrmoptionSepArg, NULL},
  {"-debug",     "debug",     XrmoptionNoArg,  "on"},
  {"-d",         "debug",     XrmoptionNoArg,  "on"},
  {"-verbose",   "verbose",   XrmoptionNoArg,  "on"},
  {"-v",         "verbose",   XrmoptionNoArg,  "on"},
};

int Syntax(call)
     char *call;
{
  fprintf (stderr,
	   "usage:  %s [-selection <name>] [-cutbuffer <number>] [-debug] [-verbose] cut|sel [value]\n",
	   call);
  exit (1);
}

typedef struct {
  String  selection_name;
  int     buffer;
  String  debug_option;
  String  verbose_option;
  int     debug; 
  int     verbose; 
  Atom    selection;
  char*   value;
  int     length;
} OptionsRec;

OptionsRec options;

#define Offset(field) XtOffsetOf(OptionsRec, field)

static XtResource resources[] = {
  {"selection", "Selection", XtRString, sizeof(String),
   Offset(selection_name), XtRString, "PRIMARY"},
  {"cutBuffer", "CutBuffer", XtRInt, sizeof(int),
   Offset(buffer), XtRImmediate, (XtPointer)0},
  {"debug", "Debug", XtRString, sizeof(String),
   Offset(debug_option), XtRString, "off"},
  {"verbose", "Verbose", XtRString, sizeof(String),
   Offset(verbose_option), XtRString, "off"},
};

#undef Offset

static void PrintValue(char *value, int length)
{
  write(1, value, length);
  write(1, "\n", 1);
}

// called when someone requests the selection value
static Boolean ConvertSelection(w, selection, target,
                                type, value, length, format)
     Widget w;
     Atom *selection, *target, *type;
     XtPointer *value;
     unsigned long *length;
     int *format;
{
  Display* d = XtDisplay(w);
  XSelectionRequestEvent* req =
    XtGetSelectionRequest(w, *selection, (XtRequestId)NULL);
   
  if (options.debug)
    printf("%p requested the selection\n", target);
   
  if (*target == XA_TARGETS(d)) {
    Atom* targetP;
    Atom* std_targets;
    unsigned long std_length;
    XmuConvertStandardSelection(w, req->time, selection, target, type,
				(XPointer*)&std_targets, &std_length, format);
    *value = XtMalloc(sizeof(Atom)*(std_length + 4));
    targetP = *(Atom**)value;
    *length = std_length + 4;
    *targetP++ = XA_STRING;
    *targetP++ = XA_TEXT(d);
    *targetP++ = XA_LENGTH(d);
    *targetP++ = XA_LIST_LENGTH(d);
    /*
     *targetP++ = XA_CHARACTER_POSITION(d);
     */
    memmove( (char*)targetP, (char*)std_targets, sizeof(Atom)*std_length);
    XtFree((char*)std_targets);
    *type = XA_ATOM;
    *format = 32;

    return True;
  }
  if (*target == XA_STRING || *target == XA_TEXT(d)) {
    *type = XA_STRING;
    *value = XtMalloc((Cardinal) options.length);
    memmove( (char *) *value, options.value, options.length);
    *length = options.length;
    *format = 8;

    if (options.debug)
      {
	printf("Giving value as string: ");
	PrintValue((char*)*value, *length);
	printf("\n");
      }
   
    return True;
  }
  if (*target == XA_LIST_LENGTH(d)) {
    long *temp = (long *) XtMalloc (sizeof(long));
    *temp = 1L;
    *value = (XtPointer) temp;
    *type = XA_INTEGER;
    *length = 1;
    *format = 32;
    return True;
  }
  if (*target == XA_LENGTH(d)) {
    long *temp = (long *) XtMalloc (sizeof(long));
    *temp = options.length;
    *value = (XtPointer) temp;
    *type = XA_INTEGER;
    *length = 1;
    *format = 32;
    return True;
  }
#ifdef notdef
  if (*target == XA_CHARACTER_POSITION(d)) {
    long *temp = (long *) XtMalloc (2 * sizeof(long));
    temp[0] = ctx->text.s.left + 1;
    temp[1] = ctx->text.s.right;
    *value = (XtPointer) temp;
    *type = XA_SPAN(d);
    *length = 2;
    *format = 32;
    return True;
  }
#endif /* notdef */
  if (XmuConvertStandardSelection(w, req->time, selection, target, type,
				  (XPointer *)value, length, format))
    return True;
   
  /* else */
  return False;
}

// Called when we no longer own the selection
static void LoseSelection(w, selection)
     Widget w;
     Atom *selection;
{
  if (options.debug)
    printf("Selection lost\n");
  exit(0);
}

// Called when the requested selection value is availiable
static void SelectionReceived(w, client_data, selection, type, value, received_length, format)
     Widget w;
     XtPointer client_data;
     Atom *selection, *type;
     XtPointer value;
     unsigned long *received_length;
     int *format;
{
  int length = *received_length;
  
  if (*type != 0 && *type != XT_CONVERT_FAIL)
      PrintValue(value, length);

  XtFree(value);
  exit(0);
}

void OwnSelection(XtPointer p, XtIntervalId* i)
{
  if (XtOwnSelection(box, options.selection,
		     0, //XtLastTimestampProcessed(dpy),
		     ConvertSelection, LoseSelection, NULL) == True)
    {
      if (options.debug)
	printf("Selection owned\n");
    }
  else
    printf("WARNING: Unable to own selection!\n");
}

void GetSelection(XtPointer p, XtIntervalId* i)
{
  XtGetSelectionValue(box, selection, XA_STRING,
		      SelectionReceived, NULL,
		      XtLastTimestampProcessed(XtDisplay(box)));
}

void Exit(XtPointer p, XtIntervalId* i)
{
  exit(0);
}

int main(int argc, char* argv[])
{
  Widget top;
  top = XtVaAppInitialize(&context, "CutSel",
			  optionDesc, XtNumber(optionDesc), &argc, argv, NULL,
			  XtNoverrideRedirect, True,
			  XtNgeometry, "-10-10",
			  0);

  if (argc < 2) Syntax(argv[0]);

  XtGetApplicationResources(top, (XtPointer)&options,
			    resources, XtNumber(resources),
			    NULL, ZERO );


  if (strcmp(options.debug_option, "on") == 0)
    options.debug = 1;
  else
    options.debug = 0;
   
  if (strcmp(options.verbose_option, "on") == 0)
    options.verbose = 1;
  else
    options.verbose = 0;
   
  options.value = NULL;
  options.length = 0;

  box = XtCreateManagedWidget("box", boxWidgetClass, top, NULL, 0);
  dpy = XtDisplay(top);
   
  selection = XInternAtom(dpy, "PRIMARY", 0);
  options.selection = selection;
  buffer = 0;

  if (strcmp(argv[1], "cut") == 0) {
    if (argc > 2) {
      XStoreBuffer(dpy,
		   argv[2],
		   strlen(argv[2]),
		   buffer);
      XtAppAddTimeOut(context, 10, Exit, 0);
    } else {
      options.value = XFetchBuffer(dpy, &options.length, buffer);
      PrintValue(options.value, options.length);
      exit(0);
    }
  } else if (strcmp(argv[1], "sel") == 0) {
    if (argc > 2) {
      options.value = argv[2];
      options.length = strlen(argv[2]);
      XtAppAddTimeOut(context, 10, OwnSelection, 0);
    } else {
      XtAppAddTimeOut(context, 10, GetSelection, 0);
    }
  } else {
    Syntax(argv[0]);
  }

  XtRealizeWidget(top);
  XtAppMainLoop(context);
  return 0;
}


