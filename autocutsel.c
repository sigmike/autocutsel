/*
 * autocutsel.c by Michael Witrant <mike @ lepton . fr>
 * Synchronizes the cutbuffer and the selection
 * version 0.6.2
 * Copyright (c) 2001,2002,2004 Michael Witrant.
 * 
 * Most code taken from:
 * * clear-cut-buffers.c by "E. Jay Berkenbilt" <ejb @ ql . org>
 *   in this messages:
 *     http://boudicca.tux.org/mhonarc/ma-linux/2001-Feb/msg00824.html
 * 
 * * xcutsel.c by Ralph Swick, DEC/Project Athena
 *   from the XFree86 project: http://www.xfree86.org/
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

#ifdef WITH_DMALLOC
#include "dmalloc.h"
#endif

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
  {"-pause",     "pause",     XrmoptionSepArg, NULL},
  {"-p",         "pause",     XrmoptionSepArg, NULL},
};

int Syntax(call)
     char *call;
{
  fprintf (stderr,
	   "usage:  %s [-selection <name>] [-cutbuffer <number>] [-pause <milliseconds>] [-debug] [-verbose]\n", 
	   call);
  exit (1);
}

typedef struct {
  String  selection_name;
  int     buffer;
  String  debug_option;
  String  verbose_option;
  String  kill;
  int     pause;
  int     debug; 
  int     verbose; 
  Atom    selection;
  char*   value;
  int     length;
  int     own_selection;
} OptionsRec;

OptionsRec options;

#define Offset(field) XtOffsetOf(OptionsRec, field)

static XtResource resources[] = {
  {"selection", "Selection", XtRString, sizeof(String),
   Offset(selection_name), XtRString, "CLIPBOARD"},
  {"cutBuffer", "CutBuffer", XtRInt, sizeof(int),
   Offset(buffer), XtRImmediate, (XtPointer)0},
  {"debug", "Debug", XtRString, sizeof(String),
   Offset(debug_option), XtRString, "off"},
  {"verbose", "Verbose", XtRString, sizeof(String),
   Offset(verbose_option), XtRString, "off"},
  {"kill", "kill", XtRString, sizeof(String),
   Offset(kill), XtRString, "off"},
  {"pause", "Pause", XtRInt, sizeof(int),
   Offset(pause), XtRImmediate, (XtPointer)500},
};

#undef Offset

static void PrintValue(value, length)
     char *value;
     int length;
{
  unsigned char c;
  int len = 0;
  
  putc('"', stdout);
  for (; length > 0; length--, value++)
    {
      c = (unsigned char)*value;
      switch (c)
	{
	case '\n':
	  printf("\\n");
	  break;
	case '\r':
	  printf("\\r");
	  break;
	case '\t':
	  printf("\\t");
	  break;
	default:
	  if (c < 32 || c > 127)
	    printf("\\x%02X", c);
	  else
	    putc(c, stdout);
	}
      len++;
      if (len >= 48)
	{
	  printf("\"...");
	  return;
	}
	    
    }
  putc('"', stdout);
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
	printf("Giving selection as string: ");
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

    if (options.debug)
      printf("Giving selection list length: %d\n", *temp);

    return True;
  }
  if (*target == XA_LENGTH(d)) {
    long *temp = (long *) XtMalloc (sizeof(long));
    *temp = options.length;
    *value = (XtPointer) temp;
    *type = XA_INTEGER;
    *length = 1;
    *format = 32;

    if (options.debug)
      printf("Giving selection length: %d\n", *temp);

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
  if (options.debug)
    printf("Unable to convert selection\n");

  return False;
}

// Called when we no longer own the selection
static void LoseSelection(w, selection)
     Widget w;
     Atom *selection;
{
  if (options.debug)
    printf("Selection lost\n");
  options.own_selection = 0;
}

// Returns true if value (or length) is different
// than current ones.
static int ValueDiffers(value, length)
     char *value;
     int length;
{
  return (!options.value ||
	  length != options.length ||
	  memcmp(options.value, value, options.length)
	  );
}

// Update the current value
static void ChangeValue(value, length)
     char *value;
     int length;
{
  if (options.value)
    XtFree(options.value);

  options.length = length;
  options.value = XtMalloc(options.length);
  if (!options.value)
    printf("WARNING: Unable to allocate memory to store the new value\n");
  else
    {
      memcpy(options.value, value, options.length);

      if (options.debug)
	{
	printf("New value saved: ");
	PrintValue(options.value, options.length);
	printf("\n");
	}
    }
}

// Called just before owning the selection, to ensure we don't
// do it if the selection already has the same value
static void OwnSelectionIfDiffers(w, client_data, selection, type, value, received_length, format)
     Widget w;
     XtPointer client_data;
     Atom *selection, *type;
     XtPointer value;
     unsigned long *received_length;
     int *format;
{
  int length = *received_length;
  
  if (*type == 0 || 
      *type == XT_CONVERT_FAIL || 
      length == 0 || 
      ValueDiffers(value, length))
    {
      if (options.debug)
	printf("Selection is out of date. Owning it\n");
      
      if (options.verbose)
	{
	  printf("cut -> sel: ");
	  PrintValue(options.value, options.length);
	  printf("\n");
	}
      
      if (XtOwnSelection(box, options.selection,
			 0, //XtLastTimestampProcessed(dpy),
			 ConvertSelection, LoseSelection, NULL) == True)
	{
	  if (options.debug)
	    printf("Selection owned\n");
	  
	  options.own_selection = 1;
	}
      else
	printf("WARNING: Unable to own selection!\n");
    }
  XtFree(value);
}

// Look for change in the buffer, and update
// the selection if necessary
static void CheckBuffer()
{
  char *value;
  int length;
  
  value = XFetchBuffer(dpy, &length, buffer);
  
  if (length > 0 && ValueDiffers(value, length))
    {
      if (options.debug)
	{
	printf("Buffer changed: ");
	PrintValue(value, length);
	printf("\n");
	}
      
      ChangeValue(value, length);
      XtGetSelectionValue(box, selection, XA_STRING,
			  OwnSelectionIfDiffers, NULL,
			  XtLastTimestampProcessed(XtDisplay(box)));
    }

  XFree(value);
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
    {
      if (length > 0 && ValueDiffers(value, length))
	{
	  if (options.debug)
	    {
	      printf("Selection changed: ");
	      PrintValue((char*)value, length);
	      printf("\n");
	    }
	  
	  ChangeValue((char*)value, length);
	  if (options.verbose)
	    {
	      printf("sel -> cut: ");
	      PrintValue(options.value, options.length);
	      printf("\n");
	    }
	  if (options.debug)
	    printf("Updating buffer\n");
	  
	  XStoreBuffer(XtDisplay(w),
		       (char*)options.value,
		       (int)(options.length),
		       buffer );
	  
	  XtFree(value);
	  return;
	}
    }
  XtFree(value);
  // Unless a new selection value is found, check the buffer value
  CheckBuffer();
}

// Called each <pause arg=500> milliseconds
void timeout(XtPointer p, XtIntervalId* i)
{
  if (options.own_selection)
    CheckBuffer();
  else
    XtGetSelectionValue(box, selection, XA_STRING,
			SelectionReceived, NULL,
			CurrentTime);
  
  XtAppAddTimeOut(context, options.pause, timeout, 0);
}

int main(int argc, char* argv[])
{
  Widget top;
  top = XtVaAppInitialize(&context, "AutoCutSel",
			  optionDesc, XtNumber(optionDesc), &argc, argv, NULL,
			  XtNoverrideRedirect, True,
			  XtNgeometry, "-10-10",
			  0);

  if (argc != 1) Syntax(argv[0]);

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

  options.own_selection = 0;
   
  box = XtCreateManagedWidget("box", boxWidgetClass, top, NULL, 0);
  dpy = XtDisplay(top);
  
  selection = XInternAtom(dpy, options.selection_name, 0);
  options.selection = selection;
  buffer = 0;
   
  options.value = XFetchBuffer(dpy, &options.length, buffer);

  XtAppAddTimeOut(context, options.pause, timeout, 0);
  XtRealizeWidget(top);
  XtAppMainLoop(context);
  return 0;
}


