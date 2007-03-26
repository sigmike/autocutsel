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


struct selsync {
  int magic;
  int client;
  char *hostname;
  int port;
  int socket;
  int server;
  char *error;
};

struct selsync *selsync_init();
int selsync_valid(struct selsync *selsync);
void selsync_free(struct selsync *selsync);
int selsync_parse_arguments(struct selsync *selsync, int argc, char **argv);
void selsync_start(struct selsync *selsync);
void selsync_main_loop(struct selsync *selsync);

void selsync_process_server_event(struct selsync *selsync);
