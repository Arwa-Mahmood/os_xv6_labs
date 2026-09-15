#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *)&s, sizeof(s));

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;

    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");

    printf("Example 4:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 5:\n");
    memdump("sccccc", (char *)&example, sizeof(example));
  } else if (argc == 2) {
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      int nn = read(0, data + n, sizeof(data) - n);
      if (nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

void
memdump(char *fmt, char *data, int len)
{
  //`data` holds `len` valid bytes.
  int pos = 0;

  for (int i = 0; fmt[i] != '\0'; i++) {
    char f = fmt[i];
    int need = 0;

    if (f == 'i')
      need = 4;
    else if (f == 'p')
      need = 8;
    else if (f == 'h')
      need = 2;
    else if (f == 'c')
      need = 1;
    else if (f == 's')
      need = 8;
    // 'S' uses whatever remains, no fixed size needed

    if (f != 'S' && pos + need > len) {
      printf("memdump: not enough data for '%c'\n", f);
      return;
    }

    if (f == 'i') {
      uint64 v = 0;
      for (int k = 3; k >= 0; k--)
        v = (v << 8) | (unsigned char)data[pos + k];
      printf("%d\n", (int)v);
      pos += 4;
    } else if (f == 'p') {
      uint64 v = 0;
      for (int k = 7; k >= 0; k--)
        v = (v << 8) | (unsigned char)data[pos + k];
      char hexdigits[] = "0123456789abcdef";
      char buf[17];
      buf[16] = 0;
      for (int k = 15; k >= 0; k--) {
        buf[k] = hexdigits[v & 0xf];
        v >>= 4;
      }
      printf("%s\n", buf);
      pos += 8;
    } else if (f == 'h') {
      uint64 v = 0;
      for (int k = 1; k >= 0; k--)
        v = (v << 8) | (unsigned char)data[pos + k];
      printf("%d\n", (short)v);
      pos += 2;
    } else if (f == 'c') {
      printf("%c\n", data[pos]);
      pos += 1;
    } else if (f == 's') {
      uint64 ptr = 0;
      for (int k = 7; k >= 0; k--)
        ptr = (ptr << 8) | (unsigned char)data[pos + k];
      printf("%s\n", (char *)ptr);
      pos += 8;
    } else if (f == 'S') {
      int j = pos;
      while (j < len && data[j] != '\0')
        j++;
      for (int k = pos; k < j; k++)
        printf("%c", data[k]);
      printf("\n");
      pos = len;
    }
  }
}
