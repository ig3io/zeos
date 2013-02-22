#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  /*char test[3];
  test[0] = 'A';
  test[1] = 'B';
  test[2] = '\0';
  write(1, test, 3);*/
  char ticks[5];
  ticks[4] = '\n';
  int counter =0;  
  write(0, "hola", strlen("hola"));
  perror("hola");
  while(1);
  /*while(1) {
	itoa(gettime(),ticks);
	if(counter%100000==0)write(1, ticks,5);
	++counter;
 }*/
}
