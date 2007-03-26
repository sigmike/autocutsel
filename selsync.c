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

#include "libselsync.h"

int main(int argc, char* argv[])
{
  struct selsync *selsync;
  
  selsync = selsync_init();
  selsync->debug = 1;
  if (selsync_parse_arguments(selsync, argc, argv)) {
    selsync_start(selsync);
    while (1)
      selsync_process_next_event(selsync);
    return 0;
  } else
    selsync_print_usage(selsync);
  selsync_free(selsync);
  return 1;
}
