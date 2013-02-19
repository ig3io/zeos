/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
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
  
  while (a[i]!=0) i++;
  
  return i;
}

int write(int fd, char * buffer, int size)
{
  int rvalue = 0;

  __asm__ __volatile__ (
    "movl %1, %%ebx\n\t"
    "movl %2, %%ecx\n\t"
    "movl %3, %%edx\n\t"
    "movl %4, %%eax\n\t"
    "int $0x80\n\t"
    "movl %%eax, %0\n\t"
    : "=r" (rvalue)
    : "g" (fd), "g" (buffer), "g" (size), "g" (0x80)
    : "eax", "ebx", "ecx", "edx" // Is this necessary?
  );

  // Return processing...
  if (rvalue < 0)
  {
    // Move code to errno
    rvalue = -1;
  }
  return rvalue;
}
