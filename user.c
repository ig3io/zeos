#include <libc.h>

#define TOTAL_THREADS 5

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
    if (counter > 60000000)
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

void silly_print(char * msg)
{
  write(1, msg, strlen(msg));
}

void * test_clone_function(void)
{
  silly_print("flow!\n");
  exit();
  // To avoid warnings
  return (void *)0;
}

void test_clone_basic(void)
{
  unsigned int stack[1024];
  clone(&test_clone_function, &stack);
}

void silly_wait()
{
  int i = 0;
  while (i++ < 100)
  {
    int j = 0;
    while (j++ < 100000)
    {
      __asm__ __volatile__(
          "nop\n\t"
        ); 
    }
  }
}

void * semaphores_clone_function(void)
{
  silly_wait();
  silly_print("Thread: I'm gonna wait right here...\n");
  if (sem_wait(0) < 0)
  {
    silly_print("Thread: the sem_wait did NOT go okay\n"); 
  }
  else
  {
    silly_print("Thread: the sem_wait went okay\n");
  }
  silly_print("Thread: and now I'm free\n");
  silly_wait();
  silly_print("Thread: goodbye\n");

  // TODO
  while(1)
  {
    silly_wait();
    silly_print("Thread: just chillin' inside a while(1)\n"); 
  }
  exit();
}

void semaphores_basic(void)
{
  unsigned int stack[2][1024];
  sem_init(0, 0);
  int flow;
  for (flow = 0; flow < 2; flow++)
  {
    clone(&semaphores_clone_function, &stack[flow][1024]);
  }
  
  silly_wait();
  silly_print("Master: releasing first flow...\n");
  sem_signal(0);
 
  silly_wait(); 
  silly_print("Master: I'm NOT releasing the second flow...\n");
  //sem_signal(0);
  silly_print("Master: furthermore, I'm the destroying the semaphore\n");
 
  //TODO
  int n=0;
  while(n<5)
  {
    silly_wait();
    silly_print("Master: just chillin' inside a while(1)\n");
    ++n;
  }

  if (sem_destroy(0) < 0)
  {
    silly_print("The sem_destroy did NOT go okay\n");
  }
  else
  {
    silly_print("The sem_destroy went okay\n");
  }
  
  silly_print("Master: I've never liked how Second Flow stares at me\n");

  silly_print("Master: ciao bambino!\n");


  //sem_destroy(0);
  //while(1);
  //silly_print("Master: semaphore destroyed\n");
}

void * semaphores_reunion_function(void)
{
  silly_print("Thread: hello there, I'm a thread\n");
  silly_wait();
  //silly_print("Thread: now I'm gonna wait for semaphore 0\n");
  if (sem_wait(0) < 0)
  {
    silly_print("Thread: the wait for semaphore 0 did NOT go okay\n");
  }
  else
  {
    //silly_print("Thread: the wait for semaphore 0 went okay\n");
  }

  silly_wait();
  //silly_print("Thread: now I'm gonna wait for semaphore 1\n");
  if (sem_wait(1) < 0)
  {
    silly_print("Thread: the wait for semaphore 1 did NOT go okay\n");
  }
  else
  {
    //silly_print("Thread: the wait for semaphore 1 went okay\n");
  }

  silly_wait();
  silly_print("Thread: goodbye!\n");

  exit();
}

void semaphores_reunion(void)
{
  if (sem_init(0, 3) < 0)
  {
    silly_print("Master: a new semaphore (0, 3) dit NOT get created\n");
  }
  else
  {
    //silly_print("Master: a new semaphore (0, 3) has been created\n");
  }

  if (sem_init(1, 2) < 0)
  {
    silly_print("Master: a new semaphore (1, 2) dit NOT get created\n");
  }
  else
  {
    //silly_print("Master: a new semaphore (1, 2) has been created\n");
  }


  unsigned int stack[TOTAL_THREADS][1024];
  int flow;
  for (flow = 0; flow < TOTAL_THREADS; flow++)
  {
    silly_wait();
    if (clone(&semaphores_reunion_function, &stack[flow][1024]) < 0)
    {
      silly_print("Master: the clone did NOT go okay\n");
    }
    else
    {
      //silly_print("Master: a new thread has been cloned\n");
    }
  }

  int i = 0;
  while (i++ < 3)
  {
    //silly_print("Master: just chillin' some time, here, inside a while(1)\n");
    silly_wait();
  }
  
  

  int j = 0;
  while (j++ < 1)
  {
    silly_wait();
    silly_print("Master: I'm gonna make some space for 1 thread over semaphore 0\n");
    sem_signal(0);
  }

  int k = 0;
  while (k++ < 2)
  {
    silly_wait();
    silly_print("Master: I'm gonna make some space for 1 thread over semaphore 1\n");
    sem_signal(1);
  }

  while(1);

  silly_wait();
  silly_wait();
  silly_print("Master: I'm gonna destroy semaphore 0. Time's up\n");
  if (sem_destroy(0) < 0)
  {
    silly_print("Master: I wasn't able to properly destroy the semaphore 0\n");
  }
  else
  {
    silly_print("Master: the semaphore 0 was destroyed!\n");
  }

  silly_wait();
  silly_wait();

  silly_print("Master: I'm gonna destroy semaphore 1. Time's up\n");

  if (sem_destroy(1) < 0)
  {
    silly_print("Master: I wasn't able to properly destroy the semaphore 1\n");
  }
  else
  {
    silly_print("Master: the semaphore 1 was destroyed!\n");
  }

  silly_print("Master: now I'm gonna get into a while(1). Just because\n");
  while(1);
}

void * clone_very_basic_function_a(void)
{
  silly_print("I'm a thread\n");

  silly_wait();

  silly_print("Goodbye\n");

  exit();
}

void * clone_very_basic_function_b(void)
{
  silly_print("I'm a thread\n");

  silly_wait();

  silly_print("I'm gonna while(1)\n");

  while(1);

  exit();
}
void clone_very_basic(void)
{
  sem_init(0, 0);

  unsigned long stack[3][1024];
  int i = 0;
  for (i = 0; i < 2; i++)
  {
    clone(&clone_very_basic_function_a, &stack[i][1024]);
  }

  clone(&clone_very_basic_function_b, &stack[2][1024]);

  silly_wait();
  silly_wait();
  
  silly_wait();
  silly_wait();
  silly_wait();
  exit();
  while(1);
}

void * clone_test_function_a(void)
{
  silly_print("A: blocked\n");
  sem_wait(0);

  while (1)
  {
    silly_print("I'm A\n");
    silly_wait();
  }

  exit();
}

void * clone_test_function_b(void)
{
  silly_print("B: blocked\n");
  sem_wait(1);

  exit();

  while (1)
  {
    silly_print("I'm B\n");
    silly_wait();
  }

  exit();
}

void * clone_test_function_c(void)
{
  silly_print("C: blocked\n");
  sem_wait(2);

  exit();

  while (1)
  {
    silly_print("I'm C\n");
    silly_wait();
  }

  exit();
}

void clone_test(void)
{
  unsigned long stack[3][1024];
  sem_init(0, 0);
  clone(&clone_test_function_a, &stack[0][1024]);
  sem_init(1, 0);
  clone(&clone_test_function_b, &stack[1][1024]);
  sem_init(2, 0);
  clone(&clone_test_function_c, &stack[2][1024]);

  silly_wait();
  silly_wait();
  silly_wait();

  sem_signal(0);
  sem_signal(1);
  sem_signal(2);

  while(1);
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
  // e1_demo();
  //fork_demo();
  //stats_basic_demo();
  //test_clone_basic();
  //semaphores_basic();
  //semaphores_reunion();
  //clone_very_basic();
  clone_test();
  exit();
  return 0;
}
