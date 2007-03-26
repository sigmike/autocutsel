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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "libselsync.h"

#define MAGIC 0x41545687

struct selsync *selsync_init()
{
  struct selsync *selsync;
  size_t len = sizeof(struct selsync);
  selsync = malloc(len);
  memset(selsync, 0, len);
  selsync->magic = MAGIC;
  selsync->hostname = NULL;
  selsync->port = 0;
  return selsync;
}

void selsync_free(struct selsync *selsync)
{
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

void selsync_connect_client(struct selsync *selsync)
{
  struct sockaddr_in addr;
  int sock;
  struct hostent* host;
  host = gethostbyname(selsync->hostname);

  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    selsync->error = "unable to create client socket";
    return;
  } else {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = *(long*)host->h_addr_list[0];
    addr.sin_port        = htons(selsync->port);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      selsync->error = "unable to connect client socket";
      return;
    }
    
    selsync->socket = sock;
  }
}

void selsync_accept_connections(struct selsync *selsync)
{
  struct sockaddr_in addr;
  int sock;

  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    selsync->error = "unable to create server socket";
    return;
  } else {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(selsync->port);

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      selsync->error = "unable to bind server socket";
      return;
    }
    
    if (listen(sock, 1) < 0) {
      selsync->error = "unable to listen server socket";
      return;
    }
    selsync->error = "plop";
    selsync->server = sock;
  }
}

void selsync_start(struct selsync *selsync)
{
  if (selsync->client)
    selsync_connect_client(selsync);
  else
    selsync_accept_connections(selsync);
}

void selsync_main_loop(struct selsync *selsync)
{
}

void selsync_process_server_event(struct selsync *selsync)
{
  struct sockaddr_in addr;
  unsigned int len = sizeof(addr);
  int sock;
  
  sock = accept(selsync->server, (struct sockaddr *) &addr, &len);
  if (sock < 0)
    return;
  
  selsync->socket = sock;
}

