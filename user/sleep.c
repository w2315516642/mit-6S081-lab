#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char *e = "arguments error, need to be 2.\n";

  if (argc != 2) {
    write(2, e, strlen(e));
    exit(1);
  }

  int ticks = atoi(argv[1]);
  pause(ticks);
  exit(0);
}
