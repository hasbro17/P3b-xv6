// Do not modify this file. It will be replaced by the grading scripts
// when checking your project.

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int p [1000];
  int i;
  for(i=0; i<1000; i++) {
	p[i]=i;
	printf(1, "Address=%d Stack Entry=%d\n",&(p[i]), p[i]);
  }
/*  int *p=0;
  printf(1,"SEG FAULT HERE=>%d", *p);
  */
  exit();
}
