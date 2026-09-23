#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int sz = 4096 * 128;
  char *buf = sbrk(sz);
  if(buf == (char *)0xffffffffffffffff) {
    printf("sbrk failed\n");
    exit(1);
  }

  int n = 0;
  for(int i = 0; i < sz; i++) {
    char c = buf[i];
    if((c >= '0' && c <= '9') ||
       (c >= 'A' && c <= 'Z') ||
       (c >= 'a' && c <= 'z')) {
      n++;
    } else {
      if(n >= 4) {
        write(1, buf + i - n, n);
        write(1, "\n", 1);
      }
      n = 0;
    }
  }
  if(n >= 4) {
    write(1, buf + sz - n, n);
    write(1, "\n", 1);
  }
  exit(0);
}
