/*
 * selsync by Michael Witrant <mike @ lepton . fr>
 * Copyright (c) 2007 Michael Witrant.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "libselsync.h"

#define MAGIC 0x20070326

XContext selsync_context = 0;

void selsync_create_widget(struct selsync *selsync)
{
  XtAppContext context;
  Widget box;
  Widget top;
  Display *dpy;
  int argc = 0;
  char **argv = NULL;
  
  top = XtVaAppInitialize(&context, "selsync",
        NULL, 0, &argc, argv, NULL,
        XtNoverrideRedirect, True,
        XtNgeometry, "-10-10",
        NULL);

  box = XtCreateManagedWidget("box", boxWidgetClass, top, NULL, 0);
  dpy = XtDisplay(top);

  XtRealizeWidget(top);
  
  selsync->widget = box;
  if (selsync_context == 0)
    selsync_context = XUniqueContext();
  
  XSaveContext(dpy, (XID)selsync->widget, selsync_context, (XPointer)selsync);
}

void selsync_init_selection(struct selsync *selsync)
{
  Display* d = XtDisplay(selsync->widget);
  selsync->selection = XInternAtom(d, "PRIMARY", 0);
  selsync->utf8_string = XInternAtom(d, "UTF8_STRING", 0);
}

void selsync_error(struct selsync* selsync, char *message)
{
  if (selsync->debug)
    printf("Error: %s\n", message);
  selsync->error = message;
}

void selsync_debug(struct selsync* selsync, char *message)
{
  if (selsync->debug)
    printf("Debug: %s\n", message);
}

void selsync_perror(struct selsync* selsync, char *message)
{
  if (selsync->debug)
    perror(message);
  selsync->error = message;
}

struct selsync *selsync_init()
{
  struct selsync *selsync;
  size_t len = sizeof(struct selsync);
  
  selsync = malloc(len);
  memset(selsync, 0, len);
  selsync->magic = MAGIC;
  selsync->reconnect_delay = 1000;
  
  selsync_create_widget(selsync);
  selsync_init_selection(selsync);
  
  return selsync;
}

void selsync_free(struct selsync *selsync)
{
  selsync_debug(selsync, "selsync_free");
  if (selsync->socket) {
    close(selsync->socket);
    selsync->socket = 0;
  }
  if (selsync->server) {
    close(selsync->server);
    selsync->server = 0;
  }
  selsync->magic = 0;
  free(selsync);
}

int selsync_valid(struct selsync *selsync)
{
  return (selsync->magic == MAGIC);
}

int selsync_parse_arguments(struct selsync *selsync, int argc, char **argv)
{
  if (argc == 3) {
    selsync->client = 1;
    selsync->hostname = argv[1];
    selsync->port = atoi(argv[2]);
  } else if (argc == 2) {
    selsync->client = 0;
    selsync->hostname = NULL;
    selsync->port = atoi(argv[1]);
  } else
    return 0;
  
  return 1;
}

void selsync_print_usage(struct selsync *selsync)
{
  printf("usage:\n");
  printf("server: selsync <port>\n");
  printf("client: selsync <hostname> <port>\n");
}

void selsync_set_reconnect_delay(struct selsync *selsync, int milliseconds)
{
  selsync->reconnect_delay = milliseconds;
}

void selsync_reconnect_timeout(struct selsync *selsync, XtIntervalId* i)
{
  selsync_connect_client(selsync);
}

void selsync_connect_client(struct selsync *selsync)
{
  struct sockaddr_in addr;
  int sock;
  struct hostent* host;
  int failure;
  
  selsync_debug(selsync, "selsync_connect_client");
  
  host = gethostbyname(selsync->hostname);

  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    selsync_perror(selsync, "unable to create client socket");
    failure = 1;
  } else {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = *(long*)host->h_addr_list[0];
    addr.sin_port        = htons(selsync->port);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      selsync_perror(selsync, "unable to connect client socket");
      failure = 1;
    } else {
      selsync->socket = sock;
      selsync_debug(selsync, "connected");
      failure = 0;
      selsync_register_socket(selsync);
      selsync_own_selection(selsync);
    }
  }
  if (failure)
    XtAppAddTimeOut(XtWidgetToApplicationContext(selsync->widget), selsync->reconnect_delay,
      (XtTimerCallbackProc)selsync_reconnect_timeout, (XPointer)selsync);
}

void selsync_process_server_event(struct selsync *selsync, int *fd, XtInputId *xid)
{
  struct sockaddr_in addr;
  unsigned int len = sizeof(addr);
  int sock;
  
  selsync_debug(selsync, "activity on server socket");
  
  sock = accept(selsync->server, (struct sockaddr *) &addr, &len);
  if (sock < 0) {
    selsync_perror(selsync, "unable to accept connection");
    return;
  }
  
  if (selsync->socket) {
    selsync_debug(selsync, "closing incoming connection: already connected");
    close(sock);
  } else {
    selsync_debug(selsync, "connected");
    selsync->socket = sock;
    selsync_register_socket(selsync);
  }
}

void selsync_accept_connections(struct selsync *selsync)
{
  struct sockaddr_in addr;
  int sock;
  int on = 1;

  selsync_debug(selsync, "selsync_accept_connections");
  
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    selsync_perror(selsync, "unable to create server socket");
    return;
  } else {
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(selsync->port);

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      selsync_perror(selsync, "unable to bind server socket");
      return;
    }
    
    if (listen(sock, 1) < 0) {
      selsync_perror(selsync, "unable to listen server socket");
      return;
    }
    selsync->server = sock;
    XtAppAddInput (XtWidgetToApplicationContext(selsync->widget),
      sock,
      (XtPointer)XtInputReadMask,
      (XtInputCallbackProc)selsync_process_server_event,
      (XtPointer)selsync);
    selsync_debug(selsync, "listening");
  }
}

void selsync_selection_value_received(Widget widget, XtPointer client_data, Atom *selection,
                                      Atom *type, XtPointer value,
                                      unsigned long *received_length, int *format)
{
  struct selsync *selsync = selsync_from_widget(widget);
  char *content;
  int len;
  
  selsync_debug(selsync, "selsync_selection_value_received");
  if (*type == 0) {
    content = "[nobody owns the selection]";
    len = strlen(content);
    selsync_debug(selsync, "nobody owns the selection");
  } else if (*type == XA_STRING || *type == selsync->utf8_string) {
    selsync_debug(selsync, "selection value received as string");
    content = (char*)value;
    len = *received_length;
  } else {
    content = "[invalid type received]";
    len = strlen(content);
    selsync_error(selsync, "invalid type received");
  }

  selsync_debug(selsync, "sending result packet");
  write(selsync->socket, "\6\1", 2);
  write(selsync->socket, &len, 4);
  write(selsync->socket, content, len);
  
  XtFree(value);
}

void selsync_reconnect(struct selsync *selsync)
{
  if (selsync->client)
    selsync_connect_client(selsync);
}

void selsync_process_socket_event(struct selsync *selsync, int *fd, XtInputId *xid)
{
  int size = 32;
  XSelectionEvent *ev;
  Display* d;
  char *value;
  char message;
  char selsync_message;
  
  d = XtDisplay(selsync->widget);

  selsync_debug(selsync, "socket event");
  size = read(selsync->socket, &message, 1);
  if (size == 1) {
    size = read(selsync->socket, &selsync_message, 1);
    switch (selsync_message) {
      case 0:
        selsync_debug(selsync, "request packet received");
        XtGetSelectionValue(selsync->widget, selsync->selection, selsync->utf8_string,
          selsync_selection_value_received, NULL, CurrentTime);
        break;
      case 1:
        selsync_debug(selsync, "result packet received");
        ev = selsync->selection_event;
        read(selsync->socket, &size, 4);
        value = malloc(size);
        read(selsync->socket, value, size);
        XChangeProperty(d, ev->requestor, ev->property,
          XA_STRING, 8, PropModeReplace,
          (unsigned char *)value, (int)size);
        
        XSendEvent(d, ev->requestor, False, (unsigned long)NULL, (XEvent *)ev);
        
        XtFree((XPointer)ev);
        break;
      case 2:
        selsync_debug(selsync, "selection lost packet received");
        selsync_own_selection(selsync);
        break;
      default:
        selsync_error(selsync, "unknown packet received");
    }
  } else {
    selsync_debug(selsync, "disconnected");
    selsync->socket = 0;
    XtRemoveInput(selsync->input_id);
    selsync_reconnect(selsync);
  }
}

void selsync_start(struct selsync *selsync)
{
  selsync_debug(selsync, "selsync_start");
  if (selsync->socket) {
    selsync_register_socket(selsync);
  } else if (selsync->client) {
    selsync_connect_client(selsync);
  } else
    selsync_accept_connections(selsync);
}

void selsync_register_socket(struct selsync *selsync)
{
  selsync->input_id = XtAppAddInput(XtWidgetToApplicationContext(selsync->widget),
    selsync->socket,
    (XtPointer)XtInputReadMask,
    (XtInputCallbackProc)selsync_process_socket_event,
    (XtPointer)selsync);
}

void selsync_process_next_event(struct selsync *selsync)
{
  XtAppProcessEvent(XtWidgetToApplicationContext(selsync->widget), XtIMAll);
}

void selsync_process_next_events(struct selsync *selsync)
{
  while (XtAppPending(XtWidgetToApplicationContext(selsync->widget)))
    selsync_process_next_event(selsync);
}

struct selsync *selsync_from_widget(Widget widget)
{
  union {
    struct selsync *selsync;
    XPointer xpointer;
  } value;
  
  Display* d = XtDisplay(widget);
  XFindContext(d, (XID)widget, selsync_context, &value.xpointer);
  return value.selsync;
}

void selsync_selection_lost(struct selsync *selsync)
{
  char lost;
  
  if (selsync->owning_selection) {
    selsync_debug(selsync, "selsync_selection_lost");
    lost = 6;
    send(selsync->socket, &lost, sizeof(lost), 0);
    lost = 2;
    send(selsync->socket, &lost, sizeof(lost), 0);
    selsync->owning_selection = 0;
  }
}

void selsync_handle_selection_event(
    Widget widget,
    XtPointer closure,
    XEvent *event,
    Boolean *cont)
{
  XSelectionEvent *ev;
  struct selsync *selsync = selsync_from_widget(widget);
  Atom target;
  Display *display;

  switch (event->type) {
  case SelectionClear:
    selsync_selection_lost(selsync);
    break;
    
  case SelectionRequest:
    target = event->xselectionrequest.target;
    display = event->xselectionrequest.display;
    
    if (target == XA_STRING || target == selsync->utf8_string) {
      selsync_debug(selsync, "selection requested as string");
      write(selsync->socket, "\6\0", 2);
      ev = (XSelectionEvent*)XtMalloc((Cardinal) sizeof(XSelectionEvent));
      ev->type = SelectionNotify;
      ev->display = event->xselectionrequest.display;
      ev->requestor = event->xselectionrequest.requestor;
      ev->selection = event->xselectionrequest.selection;
      ev->time = event->xselectionrequest.time;
      ev->target = event->xselectionrequest.target;
      ev->property = event->xselectionrequest.property;
      
      selsync->selection_event = ev;
    } else if (target == XA_TARGETS(display)) {
      int size = 2;
      Atom *targets = (Atom*)XtMalloc(sizeof(Atom) * size);
      Atom *p = targets;
      *p++ = selsync->utf8_string;
      *p++ = XA_STRING;
      
      selsync_debug(selsync, "target list requested");
      
      ev = (XSelectionEvent*)XtMalloc((Cardinal) sizeof(XSelectionEvent));
      ev->type = SelectionNotify;
      ev->display = event->xselectionrequest.display;
      ev->requestor = event->xselectionrequest.requestor;
      ev->selection = event->xselectionrequest.selection;
      ev->time = event->xselectionrequest.time;
      ev->target = event->xselectionrequest.target;
      ev->property = event->xselectionrequest.property;
      
      XChangeProperty(display, ev->requestor, ev->property,
        XA_ATOM, 32, PropModeReplace,
        (unsigned char *)targets, (int)size);
      
      XSendEvent(display, ev->requestor, False, (unsigned long)NULL, (XEvent *)ev);
      
      XtFree((XPointer)ev);
    } else {
      selsync_debug(selsync, "unknown target requested:");
      selsync_debug(selsync, XGetAtomName(display, target));
    
      ev = (XSelectionEvent*)XtMalloc((Cardinal) sizeof(XSelectionEvent));
      ev->type = SelectionNotify;
      ev->display = event->xselectionrequest.display;
      ev->requestor = event->xselectionrequest.requestor;
      ev->selection = event->xselectionrequest.selection;
      ev->time = event->xselectionrequest.time;
      ev->target = event->xselectionrequest.target;
      ev->property = None;
      
      XSendEvent(display, ev->requestor, False, (unsigned long)NULL, (XEvent *)ev);
      
      XtFree((XPointer)ev);
    }
    
    break;
  }
}

int selsync_own_selection(struct selsync *selsync)
{
  Display* d = XtDisplay(selsync->widget);
  Window window;
  
  selsync_debug(selsync, "selsync_own_selection");
  
  window = XtWindow(selsync->widget);
  
  XSetSelectionOwner(d, selsync->selection, window, CurrentTime);
  if (XGetSelectionOwner(d, selsync->selection) != window) {
    selsync_error(selsync, "unable to own selection");
    return 0;
  }
  selsync->owning_selection = 1;
  XtAddEventHandler(selsync->widget, (EventMask)0, TRUE,
    selsync_handle_selection_event, (XtPointer)selsync);
  return 1;
}

void selsync_disown_selection(struct selsync *selsync)
{
  Display* d = XtDisplay(selsync->widget);
  selsync_debug(selsync, "selsync_disown_selection");
  
  XSetSelectionOwner(d, selsync->selection, None, CurrentTime);
  selsync_selection_lost(selsync);
}

int selsync_owning_selection(struct selsync *selsync)
{
  Window window = XtWindow(selsync->widget);
  Display* d = XtDisplay(selsync->widget);
  return (XGetSelectionOwner(d, selsync->selection) == window);
}

void selsync_set_socket(struct selsync *selsync, int socket)
{
  selsync->socket = socket;
}

void selsync_set_debug(struct selsync *selsync, int level)
{
  selsync->debug = level;
}
