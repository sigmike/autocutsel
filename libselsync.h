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


struct selsync {
  int magic;
  int client;
  char *hostname;
  int port;
  int socket;
  int server;
  char *error;
  Widget widget;
  Atom selection;
  int debug;
};

struct selsync *selsync_init();
int selsync_valid(struct selsync *selsync);
void selsync_free(struct selsync *selsync);
int selsync_parse_arguments(struct selsync *selsync, int argc, char **argv);
void selsync_start(struct selsync *selsync);
void selsync_main_loop(struct selsync *selsync);

void selsync_process_next_event(struct selsync *selsync);
int selsync_owning_selection(struct selsync *selsync);
void selsync_print_usage(struct selsync *selsync);
int selsync_own_selection(struct selsync *selsync);
