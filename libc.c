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
