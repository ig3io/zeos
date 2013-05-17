/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

#include <errno.h>

int errno = 0;

char *errno_list[42] =  {
      [0 ... 41] = "General error",
      [EIO] = "Input output/error",
      [EINVAL] = "Invalid argument",
      [ENOSYS] = "Function not implemented",
      [EBADF] = "Bad file descriptor",
      [EACCES] = "Permission denied"
};


void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) {
    b[0]='0';
    b[1]=0;
    return;
  }
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0)
  {
    i++;
  }
  
  return i;
}


int write(int fd, char * buffer, int size)
{
  int rvalue = 0;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "b" (fd), "c" (buffer), "d" (size), "a" (0x04)
  );

  if (rvalue < 0)
  {
    errno = rvalue * -1;
    rvalue = -1;
  }
  return rvalue;
}

int read(int fd, char * buf, int count)
{
  int rvalue = 0;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "b" (fd), "c" (buf), "d" (count), "a" (0x03)
  );

  if (rvalue < 0)
  {
    errno = rvalue * -1;
    rvalue = -1;
  }
  return rvalue;
}

int gettime(){
  int rvalue = 0;
  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "a" (0x0A)
  );

  if(rvalue < 0) {
    errno = errno * -1;
    rvalue=-1;
  }
  return rvalue;
}

int getpid(){
  int rvalue = 0;
  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "a" (0x14)
  );

  if(rvalue < 0) {
    errno = errno * -1;
    rvalue=-1;
  }
  return rvalue;
}

int fork(){
  int rvalue = 0;
  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "a" (0x02)
  );

  if(rvalue < 0) {
    errno = errno * -1;
    rvalue=-1;
  }
  return rvalue;
}

int clone(void *(function) (void),void *stack)
{
  int rvalue = 0;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "b" (function), "c" (stack), "a" (0x13)
  );

  if (rvalue < 0)
  {
    errno = rvalue * -1;
    rvalue = -1;
  }
  return rvalue;
}

void *sbrk(int increment){
  int rvalue = 0;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "b" (increment), "a" (0x2D)
  );

  if (rvalue < 0)
  {
    errno = rvalue * -1;
    rvalue = -1;
  }
  return rvalue;
}


int perror(char * msg)
{
  if (msg != NULL)
  {
    write(1, msg, strlen(msg));
    write(1, "\n", strlen("\n"));
  }
  
  if (errno != 0)
  {
    write(1, errno_list[errno], strlen(errno_list[errno]));
    write(1, "\n", strlen("\n"));
  }

  return 0;
}

void exit(void)
{
  __asm__ __volatile__ (
    "int $0x80\n\t"
    :
    : "a" (0x01)
  );
}

int get_stats(int pid, struct stats * st)
{
  int rvalue = 0;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "b" (pid), "c" (st), "a" (0x23)
  );

  if (rvalue < 0)
  {
    errno = rvalue * -1;
    rvalue = -1;
  }
  return rvalue;
}

int sem_init(int n_sem, unsigned int value)
{
  int rvalue = 0;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "b" (n_sem), "c" (value), "a" (0x15)
  );

  if (rvalue < 0)
  {
    errno = rvalue * -1;
    rvalue = -1;
  }
  return rvalue;
}

int sem_wait(int n_sem)
{
  int rvalue = 0;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "b" (n_sem), "a" (0x16)
  );

  if (rvalue < 0)
  {
    errno = rvalue * -1;
    rvalue = -1;
  }
  return rvalue;
}

int sem_signal(int n_sem)
{
  int rvalue = 0;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "b" (n_sem), "a" (0x17)
  );

  if (rvalue < 0)
  {
    errno = rvalue * -1;
    rvalue = -1;
  }
  return rvalue;
}

int sem_destroy(int n_sem)
{
  int rvalue = 0;

  __asm__ __volatile__ (
    "int $0x80\n\t"
    : "=a" (rvalue)
    : "b" (n_sem), "a" (0x18)
  );

  if (rvalue < 0)
  {
    errno = rvalue * -1;
    rvalue = -1;
  }
  return rvalue;
}
