
// Do not modify this file. It will be replaced by the grading scripts
// when checking your project.

#include "types.h"
#include "stat.h"
#include "user.h"
int *addr;
int i;
int
main(int argc, char *argv[])
{
  int *ptr= (int*) 0x3000;//address after the code segment
  printf(1,"dereference ptr at %x as val=%d\n", ptr, *ptr);
  exit();
}
