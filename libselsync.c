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
#include "libselsync.h"

#define MAGIC 0x41545687

struct selsync *selsync_init()
{
  struct selsync *selsync;
  selsync = malloc(sizeof(struct selsync));
  selsync->magic = MAGIC;
  selsync->hostname = NULL;
  selsync->port = -1;
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

void selsync_start(struct selsync *selsync)
{
}

void selsync_main_loop(struct selsync *selsync)
{
}

