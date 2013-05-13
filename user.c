#include <libc.h>

#define TOTAL_THREADS 5
#define WAIT_FACTOR 100

char buff[24];

int pid;

void silly_print(char * msg)
{
  write(1, msg, strlen(msg));
}

void silly_wait()
{
  int i = 0;
  while (i++ < WAIT_FACTOR)
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

void silly_print_digit(int digit)
{
  char digit_c;
  itoa(digit, &digit_c);
  silly_print(&digit_c);
}

void fork_basic(int total_forks)
{
  silly_print("Fork: Basic: START: ");
  silly_print_digit(total_forks);
  silly_print(" fork calls\n");

  int forks_ok = 0;
  int forks_ko = 0;
  
  int i;
  for (i = 0; i < total_forks; i++)
  {
    int pid = fork();
    
    if (pid > 0)
    {
      forks_ok++;
    }
    else if (pid == 0)
    {
      silly_wait();
      exit();
    }
    else
    {
      forks_ko++;
    }
  }

  silly_print("Fork: Basic Total forks:     ");
  silly_print_digit(forks_ok + forks_ko);
  silly_print("\n");
  silly_print("Fork: Basic: Total OK forks: ");
  silly_print_digit(forks_ok);
  silly_print("\n");
  silly_print("Fork: Basic: Total KO forks: ");
  silly_print_digit(forks_ko);
  silly_print("\n");

  silly_print("Fork: Basic: END\n");
}

void * clone_basic_function(void)
{
  silly_wait();
  // Just to add some local variables
  int i = 0;
  int j = 0;
  while (i++ < 10)
  {
    j += 2;
  }
  exit();

  // In order to avoid gcc warnings
  return (void *)0;
}

void clone_basic(int total_clones)
{
  unsigned long stack[10][1024];

  if (total_clones > 10)
  {
    silly_print("Clone: Basic: ERROR: total_clones must be lower or equal than 10\n");
    return;
  }
  else
  {
    silly_print("Clone: Basic: START: ");
    silly_print_digit(total_clones);
    silly_print(" clones planned\n");
  }

  int clones_ok = 0;
  int clones_ko = 0;

  int i;
  for (i = 0; i < total_clones; i++)
  {
    int res = clone(&clone_basic_function, &stack[i][1024]);
    if (res > 0)
    {
      clones_ok++;
    }
    else
    {
      clones_ko++;
    }
  }

  silly_print("Clone: Basic Total clones:     ");
  silly_print_digit(clones_ok + clones_ko);
  silly_print("\n");
  silly_print("Clone: Basic: Total OK clones: ");
  silly_print_digit(clones_ok);
  silly_print("\n");
  silly_print("Clone: Basic: Total KO clones: ");
  silly_print_digit(clones_ko);
  silly_print("\n");

  silly_print("Clone: Basic: END\n");
}

void * semaphores_basic_function(void)
{
  silly_print("Semaphores: Basic: Thread: Waiting...\n");
  if (sem_wait(0) < 0)
  {
    silly_print("Semaphores: Basic: Thread: Wait ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Basic: Thread: Wait OK\n");
  }

  //while(1);

  exit();

  return (void *)0;
}

void semaphores_basic(void)
{
  sem_init(0, 0);
  unsigned long stack[1024];
  if (clone(&semaphores_basic_function, &stack[1024]) < 0)
  {
    silly_print("Semaphores: Basic: Master: Thread creation ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Basic: Master: Thread creation OK\n");
    silly_wait();
    silly_print("Semaphores: Basic: Master: Releasing semaphore\n");
    sem_signal(0);
  }

  //while(1);
  silly_print("SemaphoreS: Basic: Master: Destroying semaphore\n");
  sem_destroy(0);
}

void * semaphores_medium_function_a(void)
{
  silly_print("Semaphores: Medium: Thread A: Waiting...\n");
  if (sem_wait(0) < 0)
  {
    silly_print("Semaphores: Medium: Thread A: Wait ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Medium: Thread A: Wait OK\n");
  }

  //while(1);

  exit();

  return (void *)0;
}

void * semaphores_medium_function_b(void)
{
  silly_print("Semaphores: Medium: Thread B: Waiting...\n");
  if (sem_wait(0) < 0)
  {
    silly_print("Semaphores: Medium: Thread B: Wait ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Medium: Thread B: Wait OK\n");
  }

  //while(1);

  exit();

  return (void *)0;
}

void semaphores_medium(void)
{
  unsigned long stacks[2][1024];
  sem_init(0, 0);

  if (clone(&semaphores_medium_function_a, &stacks[0][1024]) < 0)
  {
    silly_print("Semaphores: Basic: Master: Thread A creation ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Basic: Master: Thread A creation OK\n");
  }
  
  if (clone(&semaphores_medium_function_b, &stacks[1][1024]) < 0)
  {
    silly_print("Semaphores: Basic: Master: Thread B creation ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Basic: Master: Thread B creation OK\n");
  }

  silly_wait();
  silly_print("Semaphores: Basic: Master: Releasing semaphore\n");
  sem_signal(0);
  
  silly_wait();
  silly_print("Semaphores: Basic: Master: Releasing semaphore\n");
  sem_signal(0);

  //while(1);
  silly_print("SemaphoreS: Basic: Master: Destroying semaphore\n");
  sem_destroy(0);
}

void * semaphores_advanced_function_a(void)
{
  silly_print("Semaphores: Advanced: Thread A: Hello\n");
  if (sem_wait(0) < 0)
  {
    silly_print("Semaphores: Advanced: Thread A: sem_wait ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Advanced: Thread A: sem_wait OK\n");
  }
  silly_wait();
  silly_wait();
  silly_wait();
  silly_print("Semaphores: Advanced: Thread A: Ciao!\n");
  exit();

  return (void *)0;
}

void * semaphores_advanced_function_b(void)
{
  silly_print("Semaphores: Advanced: Thread B: Hello\n");
  if (sem_wait(0) < 0)
  {
    silly_print("Semaphores: Advanced: Thread B: sem_wait ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Advanced: Thread B: sem_wait OK\n");
  }
  silly_wait();
  silly_wait();
  silly_wait();
  silly_print("Semaphores: Advanced: Thread B: Ciao!\n");
  exit();

  return (void *)0;
}

void * semaphores_advanced_function_c(void)
{
  silly_print("Semaphores: Advanced: Thread C: Hello\n");
  if (sem_wait(0) < 0)
  {
    silly_print("Semaphores: Advanced: Thread C: sem_wait ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Advanced: Thread C: sem_wait OK\n");
  }
  silly_wait();
  silly_wait();
  silly_wait();
  silly_print("Semaphores: Advanced: Thread C: Ciao!\n");
  exit();

  return (void *)0;
}

void * semaphores_advanced_function_d(void)
{
  silly_print("Semaphores: Advanced: Thread D: Hello\n");
  if (sem_wait(0) < 0)
  {
    silly_print("Semaphores: Advanced: Thread D: sem_wait ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Advanced: Thread D: sem_wait OK\n");
  }
  silly_wait();
  silly_wait();
  silly_wait();
  silly_print("Semaphores: Advanced: Thread D: Ciao!\n");
  exit();

  return (void *)0;
}

void semaphores_advanced(void)
{
  unsigned long stacks[4][1024];

  sem_init(0, 1);

  if (clone(&semaphores_advanced_function_a, &stacks[0][1024]) < 0)
  {
    silly_print("Semaphores: Advanced: Master: Thread A creation ERROR\n");
  }
  if (clone(&semaphores_advanced_function_b, &stacks[1][1024]) < 0)
  {
    silly_print("Semaphores: Advanced: Master: Thread B creation ERROR\n");
  }
  if (clone(&semaphores_advanced_function_c, &stacks[2][1024]) < 0)
  {
    silly_print("Semaphores: Advanced: Master: Thread C creation ERROR\n");
  }
  if (clone(&semaphores_advanced_function_d, &stacks[3][1024]) < 0)
  {
    silly_print("Semaphores: Advanced: Master: Thread D creation ERROR\n");
  }

  silly_print("Semaphores: Advanced: Master: Threads A, B, C, and D created!\n");

  silly_wait();

  silly_print("Semaphores: Advanced: Master: Signaling semaphore 0\n");
  sem_signal(0);
  silly_wait();
  silly_wait();

  silly_print("Semaphores: Advanced: Master: Signaling semaphore 0\n");
  sem_signal(0);
  silly_wait();
  silly_wait();

  if (sem_destroy(0) < 0)
  {
    silly_print("Semaphores: Advanced: Master: sem_destroy ERROR\n");
  }
  else
  {
    silly_print("Semaphores: Advanced: Master: sem_destroy OK\n");
  }

  silly_wait();
  silly_wait();

  silly_print("Semaphores: Advanced: Master: Ciao!\n");

  exit();

}

void read_easy_test(){
  int fd = 1;
  int size = 4;
  char parbuf[4];
  int pid = fork();
  if(pid==0){
    while (1)
    {
      int len = read(fd, parbuf, size);
      if (len == 4) {
        write(1, parbuf, len);
      }
    }
  }
  else silly_print("padre_sale");
}

void sbrk_easy_test(){

  char *string =(char*) sbrk(1000);
  string[0] = 'H';
  string[1] = 'O';
  string[2] = 'L';
  string[3] = 'A';
  string[4] = ' ';
    //sbrk(500);
  silly_print(string);
  silly_print("WORKS ");
  char *string2 = (char*) sbrk(1000);
  string2[0] = 'A';
  string2[1] = 'D';
  string2[2] = 'E';
  string2[3] = 'U';
  string2[4] = ' ';
  silly_print(string2);
  silly_print("WORKS_2 ");

  char * string3 = (char*) sbrk(-1000);
  silly_print(string3);

  //string2 = (char)
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
  //fork_basic(20);
  //clone_basic(10);
  //semaphores_basic();
  //semaphores_medium();
  //semaphores_advanced();
  //read_easy_test();
  sbrk_easy_test();
  while(1);
  
  return 0;
}
