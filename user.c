#include <libc.h>

char buff[24];

int pid;

void e1_demo(void)
{

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

  int counter = 0;
  while(1) {
    if(counter > 10000000)
    {
      counter = 0;
      itoa(gettime(), ticks);
      write(1, ticks, strlen(ticks));
      write(1, " ", strlen(" "));
    }
    ++counter;
  }
}

void exit_demo(void)
{
  	char *msg;
	char pid[1];
	msg = "SOY EL HIJO ";
	write(1, msg, strlen(msg));
	itoa(getpid(),pid);
	write(1,pid,1);
	msg = " Y ME VOY A SUCIDAR, ADIOOOOS\n";
	write(1,msg,strlen(msg));
	exit();
	
}

void fork_demo(void)
{
  char *msg;
  int pid = fork();
  if(pid == 0){
    msg = "HOLA SOY EL HIJO\n";
    write(1, msg, strlen(msg));
  }
  else {
    msg = "HOLA SOY EL PADRE\n";
    write(1, msg, strlen(msg));
  }
  
  int counter = 0;
  int times = 0;
  while(1) {
	if(counter > 10000000)
    {
      counter = 0;
      times++;
      counter = 0;
      char c_times[1];
      itoa(times, c_times);
      write(1, c_times, strlen(c_times));
      if (getpid() != 1) {
        write(1, "h", strlen("h"));
	if(times%15==0) exit_demo();
      } 
      if(getpid()==1 && times%25==0) fork();
      write(1, " ", strlen(" "));
    }
	++counter;
 }
}

void stats_basic_demo(void)
{
  char *msg;
  int pid = fork();
  if (pid == 0)
  {
    msg = "pare\n";
  }
  else
  {
    msg = "fill\n";
  }
  write(1, msg, strlen(msg));

  int counter = 0;
  while(1)
  {
    if (counter > 10000000)
    {
      if (pid == 0)
      {
        msg = "pare - ";
      }
      else
      {
        msg = "fill - ";
      }
      write(1, msg, strlen(msg));
      
      struct stats st;
      if (get_stats(pid, &st) < 0)
      {
        msg = "problema amb get_stats\n"; 
      }
      else
      {
        msg = "get_stats amb exit\n";
      }
      write(1, msg, strlen(msg));
      counter = 0;
    }
    ++counter;
  }

}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
  // e1_demo();
  //fork_demo();
  stats_basic_demo();
  return 0;
}
