
#include "libselsync.h"

int main(int argc, char* argv[])
{
  struct selsync *selsync;
  
  selsync = selsync_init();
  if (selsync_parse_arguments(selsync, argc, argv)) {
    selsync_start(selsync);
    selsync_main_loop(selsync);
    return 0;
  }
  selsync_free(selsync);
  return 1;
}
