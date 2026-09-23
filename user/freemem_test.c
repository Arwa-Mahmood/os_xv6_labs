#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  printf("free memory: %ld bytes\n", freemem());
  exit(0);
}
