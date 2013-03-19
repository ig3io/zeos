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


#define WRITE_BUFFER_SIZE 4

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

  struct list_head *free_pcb = list_first(&freequeue);// take the first free PCB
/* if freequeue don't have any element, return an error(not implemented)*/
  struct task_struct *child = list_head_to_task_struct(free_pcb);
  struct task_struct *father = current();
  
  /* create a variable for store the free frames that we need for save data+Kernel pages. I create de NUM_DATA_PAG macro, but is because i don't know how much occupy data+kernel in pages, for the moment i supose 4, but we must ask at juanjo*/

////////////////// COLLECTING FREE FRAMES ////////////////////////
  int frames[NUM_PAG_DATA];
  int i;
  for (i=0;i<NUM_PAG_DATA;++i){
	frames[i] = alloc_frame();
	if(frames[i]==-1){
		while(i >=0) free_frame(frames[i--]);//FRAMES LIBERATION! like blacksXD
		 /* return not memory space error*/
	}
  }
/////////////////////////////////////////////////////////////////////

  

  
  
  return PID;
}

void sys_exit()
{
  // TODO
  // free frames...
  printc_xy(2, 4, 'E');
  printc_xy(3, 4, 'x');
  free_user_pages(current());
  sched_exit_rr();
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
  char buffer_kernel[WRITE_BUFFER_SIZE];
  int pending = size;
  int done = 0;
  int res = 0;
  while (pending > WRITE_BUFFER_SIZE)
  {
    int res_cp = copy_from_user(buffer + done, buffer_kernel, WRITE_BUFFER_SIZE);
    sys_write_console(buffer_kernel, WRITE_BUFFER_SIZE);
    if (res_cp < 0)
    {
      return res_cp;
    }
    pending -= sizeof(char) * WRITE_BUFFER_SIZE;
    done += sizeof(char) * WRITE_BUFFER_SIZE;
  }
  int res_cp = copy_from_user(buffer + done, buffer_kernel, pending);
  sys_write_console(buffer_kernel, pending);
  if (res_cp < 0)
  {
    res = -EIO;
  }
  else
  {
    done += sizeof(char) * pending;
    res = done;
  }
  return res;
}

int sys_gettime(){
	return zeos_ticks;
}
