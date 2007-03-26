/*
 * selsync by Michael Witrant <mike @ lepton . fr>
 * Copyright (c) 2001-2007 Michael Witrant.
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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "libselsync.h"

#define MAGIC 0x41545687

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
  XtVaSetValues(selsync->widget, "selsync", selsync, NULL);
}

void selsync_init_selection(struct selsync *selsync)
{
  Display* d = XtDisplay(selsync->widget);
  selsync->selection = XInternAtom(d, "PRIMARY", 0);
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
  selsync->hostname = NULL;
  selsync->port = 0;
  
  selsync_create_widget(selsync);
  selsync_init_selection(selsync);
  
  return selsync;
}

void selsync_free(struct selsync *selsync)
{
  selsync_debug(selsync, "selsync_free");
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

void selsync_connect_client(struct selsync *selsync)
{
  struct sockaddr_in addr;
  int sock;
  struct hostent* host;
  
  selsync_debug(selsync, "selsync_connect_client");
  
  host = gethostbyname(selsync->hostname);

  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    selsync_perror(selsync, "unable to create client socket");
    return;
  } else {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = *(long*)host->h_addr_list[0];
    addr.sin_port        = htons(selsync->port);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      selsync_perror(selsync, "unable to connect client socket");
      return;
    }
    
    selsync->socket = sock;
    selsync_debug(selsync, "connected");
  }
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
  
  selsync_debug(selsync, "connected");
  selsync->socket = sock;
}

void selsync_accept_connections(struct selsync *selsync)
{
  struct sockaddr_in addr;
  int sock;

  selsync_debug(selsync, "selsync_accept_connections");
  
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    selsync_perror(selsync, "unable to create server socket");
    return;
  } else {
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
      (XtPointer) (XtInputReadMask|XtInputWriteMask|XtInputExceptMask),
      selsync_process_server_event,
      (XtPointer)selsync);
    selsync_debug(selsync, "listening");
  }
}

void selsync_start(struct selsync *selsync)
{
  selsync_debug(selsync, "selsync_start");
  if (selsync->client) {
    selsync_connect_client(selsync);
    selsync_own_selection(selsync);
  } else
    selsync_accept_connections(selsync);
}

void selsync_main_loop(struct selsync *selsync)
{
}

void selsync_process_next_event(struct selsync *selsync)
{
  XtAppProcessEvent(XtWidgetToApplicationContext(selsync->widget), XtIMAll);
}

Boolean selsync_selection_requested(Widget w, Atom *selection, Atom *target,
                                    Atom *type, XtPointer *value,
                                    unsigned long *length, int *format)
{
  //selsync_debug(selsync, "selsync_selection_requested");
  return False;
}

void selsync_selection_lost(Widget w, Atom *selection)
{
  //selsync_debug(selsync, "selsync_selection_lost");
}


int selsync_own_selection(struct selsync *selsync)
{
  Display* d = XtDisplay(selsync->widget);
  
  selsync_debug(selsync, "selsync_own_selection");
  
  if (XtOwnSelection(selsync->widget, selsync->selection,
                     XtLastTimestampProcessed(d),
                     selsync_selection_requested, selsync_selection_lost, NULL) == True)
    return 1;
  else
    return 0; 
}

int selsync_owning_selection(struct selsync *selsync)
{
  Window window = XtWindow(selsync->widget);
  Display* d = XtDisplay(selsync->widget);
  return (XGetSelectionOwner(d, selsync->selection) == window);
}
