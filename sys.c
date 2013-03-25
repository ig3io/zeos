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

// TODO: ret_from_fork
#include <entry.h>

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
	printc_xy(7, 9, 'F');
	printc_xy(8, 9, 'O');
	printc_xy(9, 9, 'R');
	printc_xy(10, 9, 'K');
	
  int PID=-1;

  struct list_head *free_pcb = list_first(&freequeue);// take the first free PCB
/* if freequeue don't have any element, return an error(not implemented)*/
  union task_union *child = (union task_union*)list_head_to_task_struct(free_pcb);
  union task_union *father = (union task_union*)current();
  list_del(free_pcb);
  /* create a variable for store the free frames that we need for save data+Kernel pages.*/

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
page_table_entry* TP_child = get_PT(&child->task);
page_table_entry* TP_father = get_PT(&father->task);
copy_data(father, child, sizeof(struct task_struct));

//int child_dir = allocate_DIR(child);//I'm not sure what I'm doing here?¿!?!?¿!?


for(i=PAG_LOG_INIT_CODE_P0;i<PAG_LOG_INIT_DATA_P0;++i) //Copy the Code Pages to child proces
	set_ss_pag(TP_child,i,get_frame(TP_father,i));

for(i=PAG_LOG_INIT_DATA_P0;i<PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA;++i)//Create new Data+Stack Pages to child proces
	set_ss_pag(TP_child,i,frames[i-PAG_LOG_INIT_CODE_P0]);

for(i=PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA;i<PAG_LOG_INIT_DATA_P0+2*NUM_PAG_DATA;++i){//Mapping the data+stack pages of child
	//printc_xy(i-FRAME_INIT_CODE_P0-NUM_PAG_DATA,10,'A');
	set_ss_pag(TP_father,i,frames[i-(PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA)]);
	//printc_xy(i-FRAME_INIT_CODE_P0-NUM_PAG_DATA,11,'A');
	copy_data((unsigned long *)((i-(NUM_PAG_DATA))*PAGE_SIZE),(unsigned long *)((i)*PAGE_SIZE),PAGE_SIZE);
	//printc_xy(i-FRAME_INIT_CODE_P0-NUM_PAG_DATA,12,'A');
	del_ss_pag(TP_father,i);
	//printc_xy(i-FRAME_INIT_CODE_P0-NUM_PAG_DATA,13,'A');
}
printc_xy(14, 9, 'K');
set_cr3(get_DIR(&father->task)); //FLUSH TLB

////////////////STATISTICS//////////////////////
child->task.PID = current()->PID+1;
child->task.quantum = QUANTUM;
child->task.state = ST_READY;
///////////////////////////////////////////////

int ebp;
int hijo = &child->stack[0];
int padre = &father->stack[0];

__asm__ __volatile__(
	"mov %%ebp,%0\n\t"
	"mov %1,%%ecx\n\t"
	"mov %2,%%edx\n\t"
	:"=g"(ebp)
	:"r" (hijo), "r" (padre));

printc_xy(15, 9, 'K');

int des = ebp - (int)&father->stack[0]; // Calculate the diference bettwen ebp & esp, necessary for possible values pushed in the stack

child->stack[des] = ((int)child->stack[des] - (int)&father->stack[0]) + (int)&child->stack[0];
child->stack[des-1] = (unsigned long)&ret_from_fork;
child->stack[des-2] = (unsigned long)&child->stack[des];


//child->task.kernel_esp = &child->stack[elem];
child->task.kernel_esp  = (unsigned long *)ebp;
	
PID = child->task.PID;
list_add_tail(&child->task.list,&readyqueue);
  return PID;
}

void sys_exit()
{
  // TODOt
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
