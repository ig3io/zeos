/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <interrupt.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -EACCES; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -ENOSYS; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  int PID=-1;

  // creates the child process
  
  return PID;
}

void sys_exit()
{  
}

int sys_write(int fd, char * buffer, int size)
{
  int ch_fd = check_fd(fd, ESCRIPTURA);

  if (ch_fd < 0)
  {
  	return ch_fd;
  }
  if (buffer == NULL)
  {
    return -EINVAL;
  }
  if (size < 0)
  {
    return -EINVAL;
  }
  // copy data..
  char buffer_kernel[4];
  int pending = size;
  int done = 0;
  int res = 0;
  while (pending > 4)
  {
    int res_cp = copy_from_user(buffer + done, buffer_kernel, 4);
    sys_write_console(buffer_kernel, 4);
    if (res_cp < 0)
    {
      return res_cp;
    }
    pending -= sizeof(char) * 4;
    done += sizeof(char) * 4;
  }
  int res_cp = copy_from_user(buffer + done, buffer_kernel, pending);
  sys_write_console(buffer_kernel, pending);
  if (res_cp < 0)
  {
    return -EIO;
  }
  return res;
}

int sys_gettime(){
	
	return zeos_ticks;
}
