struct selsync {
  int magic;
  int client;
  char *hostname;
  int port;
};

struct selsync *selsync_init();
int selsync_valid(struct selsync *selsync);
void selsync_free(struct selsync *selsync);
int selsync_parse_arguments(struct selsync *selsync, int argc, char **argv);
void selsync_start(struct selsync *selsync);
void selsync_main_loop(struct selsync *selsync);
