#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void)
{
  printf("sbrk(0) before = %p\n", sbrk(0));
  printf("=== before sbrk ===\n");
  vmprint();

  char *p = sbrk(1);
  printf("sbrk(1) returned = %p\n", p);
  if(p == (char*)-1){
    printf("sbrk failed\n");
    exit(1);
  }

  printf("=== after sbrk, before write ===\n");
  vmprint();

  *p = 0x42;
  printf("=== after write ===\n");
  vmprint();

  printf("sbrk(0) after = %p\n", sbrk(0));
  exit(0);
}
