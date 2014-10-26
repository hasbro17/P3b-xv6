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
  int p[1000];
// Trying an old fashioned stack overflow attack!
  addr = p;
  for(i=0; i<200; i++) {
	addr= addr-1000;
//	printf(1, "Address=%x\n",addr);
	*addr=i;
	printf(1, "Address=%x Stack Entry=%d\n",addr, *addr);
  }
 
/*  int *p=0;
  printf(1,"SEG FAULT HERE=>%d", *p);
  */
  exit();
}
