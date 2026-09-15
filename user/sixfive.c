#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char *sep = " -\r\t\n./,";

void
process(int fd)
{
  char c;
  long num = 0;
  int have_digit = 0;
  int valid = 1;

  while (read(fd, &c, 1) == 1) {
    if (strchr(sep, c)) {
      if (valid && have_digit && (num % 5 == 0 || num % 6 == 0))
        printf("%d\n", (int)num);
      num = 0;
      have_digit = 0;
      valid = 1;
    } else if (c >= '0' && c <= '9') {
      num = num * 10 + (c - '0');
      have_digit = 1;
    } else {
      valid =
        0; // letter or other junk char breaks this as a "pure number" token
    }
  }

  // end of file is an implicit separator
  if (valid && have_digit && (num % 5 == 0 || num % 6 == 0))
    printf("%d\n", (int)num);
}

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    process(0);
  } else {
    for (int i = 1; i < argc; i++) {
      int fd = open(argv[i], 0);
      if (fd < 0) {
        printf("sixfive: cannot open %s\n", argv[i]);
        continue;
      }
      process(fd);
      close(fd);
    }
  }
  exit(0);
}
