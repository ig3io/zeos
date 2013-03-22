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
  //int counter =0;
  char *msg = "Hi! Welcome User!\n";
  int res = 0;
  write(1, msg, strlen(msg));

  msg = "We're going to generate a bad descriptor file error... watch out\n";
  res = write(1, msg, strlen(msg));
  if (res == strlen(msg)) {
    write(1, "Write return OK!\n", strlen("Write return OK!\n"));
  }

  write(3, "error", strlen("error"));
  msg = "Done! What does perror say?\n";
  write(1, msg, strlen(msg));

  perror("This is Perror, reporting in:");
  msg = "\n\nNumber of clock ticks:\n";
  write(1, msg, strlen(msg));

  char ticks[5] = {[0 ... 3] = ' ', [4] = '\n'};
  itoa(gettime(), ticks);
  long counter = 0;
  int times = 0;
  ///////PRUEBAS DE FORK///////////////
  int pid = fork();
  if(pid == 0){
	char c_pid;
	itoa(getpid(),c_pid);
	 msg = "HOLA SOY EL PUTO HIJO Y MI PID ES";
	write(1,msg,strlen(msg));
	write(1,c_pid,strlen(c_pid));
  }
  else/* {
	char c_pid;
	itoa(getpid(),c_pid);
	msg = "EL FORK NO FUNCIONA, PRINGADOS!!!";
	write(1,msg,strlen(msg));
	write(1,c_pid,strlen(c_pid));
  }*/
  while(1) {
	if(counter > 10000000)
    {
      times++;
      counter = 0;
      itoa(gettime(), ticks);
      write(1, ticks, strlen(ticks));
      write(1, " ", strlen(" "));
    }
    if(times > 2)
    {
      exit();
    }
	++counter;
 }
}
