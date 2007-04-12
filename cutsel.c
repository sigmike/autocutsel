/*
 * autocutsel by Michael Witrant <mike @ lepton . fr>
 * Manipulates the cutbuffer and the selection
 * Copyright (c) 2001-2006 Michael Witrant.
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


#include "common.h"

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

int Syntax(char *call)
{
  fprintf (stderr,
    "usage:  %s [-selection <name>] [-cutbuffer <number>] [-debug] [-verbose] cut|sel [<value>]\n", call);
  fprintf (stderr,
    "        %s [-selection <name>] [-cutbuffer <number>] [-debug] [-verbose] targets\n", call);
  fprintf (stderr,
    "        %s [-selection <name>] [-cutbuffer <number>] [-debug] [-verbose] length\n", call);
  fprintf (stderr,
    "        %s [-selection <name>] [-cutbuffer <number>] [-debug] [-verbose] utf8 [<value>]\n", call);
  fprintf (stderr,
    "        %s [-selection <name>] [-cutbuffer <number>] [-debug] [-verbose] request_selection <target>\n", call);
  exit (1);
}

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
};

#undef Offset

// Called when we no longer own the selection
static void LoseSelection(Widget w, Atom *selection)
{
  if (options.debug)
    printf("Selection lost\n");
  exit(0);
}

static void PrintSelection(Widget w, XtPointer client_data, Atom *selection,
                           Atom *type, XtPointer value,
                           unsigned long *received_length, int *format)
{
  Display* d = XtDisplay(w);
  
  if (*type == 0)
    printf("Nobody owns the selection\n");
  else if (*type == XA_STRING)
      printf("%s\n", (char*)value);
  else
    printf("Invalid type received: %s\n", XGetAtomName(d, *type));

  XtFree(value);
  exit(0);
}

static void TargetsReceived(Widget w, XtPointer client_data, Atom *selection,
                           Atom *type, XtPointer value,
                           unsigned long *length, int *format)
{
  Display* d = XtDisplay(w);
  int i;
  Atom *atoms;
  
  if (*type == 0)
    printf("No target received\n");
  else if (*type == XA_ATOM) {
    atoms = (Atom*)value;
    printf("%lu targets (%i bits each):\n", *length, *format);
    for (i=0; i<*length; i++)
      printf("%s\n", XGetAtomName(d, atoms[i]));
  } else
    printf("Invalid type received: %s\n", XGetAtomName(d, *type));

  XtFree(value);
  exit(0);
}

static void LengthReceived(Widget w, XtPointer client_data, Atom *selection,
                           Atom *type, XtPointer value,
                           unsigned long *received_length, int *format)
{
  Display* d = XtDisplay(w);
  
  if (*type == 0)
    printf("No length received\n");
  else if (*type == XA_INTEGER) {
      printf("Length is %" PRIx32 "\n", *(CARD32*)value);
  } else
      printf("Invalid type received: %s\n", XGetAtomName(d, *type));

  XtFree(value);
  exit(0);
}

static void Utf8Received(Widget w, XtPointer client_data, Atom *selection,
                         Atom *type, XtPointer value,
                         unsigned long *received_length, int *format)
{
  Display* d = XtDisplay(w);
  
  if (*type == 0)
    printf("No UTF8_STRING received\n");
  else if (*type == XInternAtom(d, "UTF8_STRING", False)) {
      printf("%s\n", (char*)value);
  } else
      printf("Invalid type received: %s\n", XGetAtomName(d, *type));

  XtFree(value);
  exit(0);
}

static void SelectionReceived(Widget w, XtPointer client_data, Atom *selection,
                              Atom *type, XtPointer value,
                              unsigned long *length, int *format)
{
  Display* d = XtDisplay(w);
  int i;
  
  if (*type == 0)
    printf("Type: None\n");
  else {
    printf("Type: %s\n", XGetAtomName(d, *type));
    if (*type == XA_ATOM) {
      Atom *atoms = (Atom*)value;
      for (i=0; i<*length; i++)
        printf("%s\n", XGetAtomName(d, atoms[i]));
    } else {
      fwrite((char*)value, *length, 1, stdout);
    }
  }
  
  XtFree(value);
  exit(0);
}

void OwnSelection(XtPointer p, XtIntervalId* i)
{
  if (XtOwnSelection(box, options.selection,
                     0, //XtLastTimestampProcessed(dpy),
                     ConvertSelection, LoseSelection, NULL) == True) {
    if (options.debug)
      printf("Selection owned\n");
  } else
    printf("WARNING: Unable to own selection!\n");
}

Boolean ConvertSelectionToUtf8Only(Widget w, Atom *selection, Atom *target,
                                   Atom *type, XtPointer *value,
                                   unsigned long *length, int *format)
{
  Display* d = XtDisplay(w);
  XSelectionRequestEvent* req =
    XtGetSelectionRequest(w, *selection, (XtRequestId)NULL);
  
  if (options.debug) {
    printf("Window 0x%lx requested %s of selection %s.\n",
      req->requestor,
      XGetAtomName(d, *target),
      XGetAtomName(d, *selection));
  }
  
  if (*target == XA_TARGETS(d)) {
    Atom *targetP, *atoms;
    XPointer std_targets;
    unsigned long std_length;
    int i;
    
    *value = XtMalloc(sizeof(Atom)*2);
    targetP = *(Atom**)value;
    atoms = targetP;
    *length = 2;
    *targetP++ = XInternAtom(d, "UTF8_STRING", True);
    *targetP++ = XA_TARGETS(d);
    memmove( (char*)targetP, (char*)std_targets, sizeof(Atom)*std_length);
    XtFree((char*)std_targets);
    *type = XA_ATOM;
    *format = 32;
    
    if (options.debug) {
      printf("Targets are: ");
      for (i=0; i<*length; i++)
        printf("%s ", XGetAtomName(d, atoms[i]));
      printf("\n");
    }

    return True;
  }
  
  if (*target == XInternAtom(d, "UTF8_STRING", True)) {
    *type = XA_STRING;
    *value = XtMalloc((Cardinal) options.length);
    memmove((char *)*value, options.value, options.length);
    *length = options.length;
    *format = 8;

    if (options.debug) {
      printf("Returning ");
      PrintValue((char*)*value, *length);
      printf("\n");
    }
   
    return True;
  }
  
  /* else */
  if (options.debug)
    printf("Target not allowed\n");

  return False;
}

void OwnSelectionAsUtf8(XtPointer p, XtIntervalId* i)
{
  if (XtOwnSelection(box, options.selection,
                     0, //XtLastTimestampProcessed(dpy),
                     ConvertSelectionToUtf8Only, LoseSelection, NULL) == True) {
    if (options.debug)
      printf("Selection owned\n");
  } else
    printf("WARNING: Unable to own selection!\n");
}

void GetSelection(XtPointer p, XtIntervalId* i)
{
  XtGetSelectionValue(box, selection, XA_STRING,
    PrintSelection, NULL,
    CurrentTime);
}

void GetTargets(XtPointer p, XtIntervalId* i)
{
  Display* d = XtDisplay(box);
  XtGetSelectionValue(box, selection, XA_TARGETS(d),
    TargetsReceived, NULL,
    CurrentTime);
}

void GetLength(XtPointer p, XtIntervalId* i)
{
  Display* d = XtDisplay(box);
  XtGetSelectionValue(box, selection, XA_LENGTH(d),
    LengthReceived, NULL,
    CurrentTime);
}

void GetUtf8(XtPointer p, XtIntervalId* i)
{
  Display* d = XtDisplay(box);
  XtGetSelectionValue(box, selection, XInternAtom(d, "UTF8_STRING", False),
    Utf8Received, NULL,
    CurrentTime);
}

void RequestSelection(XtPointer p, XtIntervalId* i)
{
  char *target = (char*)p;
  Display* d = XtDisplay(box);
  
  XtGetSelectionValue(box, selection, XInternAtom(d, target, False),
    SelectionReceived, NULL,
    CurrentTime);
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
        NULL);

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

  if (options.debug || options.verbose)
    printf("cutsel v%s\n", VERSION);
   
  options.value = NULL;
  options.length = 0;

  box = XtCreateManagedWidget("box", boxWidgetClass, top, NULL, 0);
  dpy = XtDisplay(top);
   
  selection = XInternAtom(dpy, options.selection_name, 0);
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
      if (options.length)
        printf("%s\n", options.value);
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
  } else if (strcmp(argv[1], "targets") == 0) {
    XtAppAddTimeOut(context, 10, GetTargets, 0);
  } else if (strcmp(argv[1], "length") == 0) {
    XtAppAddTimeOut(context, 10, GetLength, 0);
  } else if (strcmp(argv[1], "utf8") == 0) {
    if (argc > 2) {
      options.value = argv[2];
      options.length = strlen(argv[2]);
      XtAppAddTimeOut(context, 10, OwnSelectionAsUtf8, 0);
    } else {
      XtAppAddTimeOut(context, 10, GetUtf8, 0);
    }
  } else if (strcmp(argv[1], "request_selection") == 0) {
    XtAppAddTimeOut(context, 10, RequestSelection, argv[2]);
  } else {
    Syntax(argv[0]);
  }

  XtRealizeWidget(top);
  XtAppMainLoop(context);
  return 0;
}
