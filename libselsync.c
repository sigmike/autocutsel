
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

