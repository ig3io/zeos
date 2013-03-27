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

int New_pid=1;

int assign_pid(){
	return ++New_pid;
}

void ret_from_fork(){
	__asm__ __volatile__(
		"popl %%eax\n"
		"xor %%eax,%%eax\n"
		:
		:
		:"ax");
}
		

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -EACCES; /*EACCES*/
  return 0;
}


int sys_ni_syscall()
{
  stats_current_user_to_system();
  stats_current_system_to_user();
  return -ENOSYS; /*ENOSYS*/
}

int sys_getpid()
{
  stats_current_user_to_system();
  stats_current_system_to_user();
  return current()->PID;
}

int sys_fork()
{
  stats_current_user_to_system();

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
      while(i >=0) free_frame(frames[i--]);
      return -ENOMEM;
    }
  }

  /////////////////////////////////////////////////////////////////////
  page_table_entry* TP_child = get_PT(&child->task);
  page_table_entry* TP_father = get_PT(&father->task);
  copy_data(father, child, sizeof(union task_union));

  allocate_DIR(&(child->task));//I'm not sure what I'm doing here?¿!?!?¿!?


  for(i=PAG_LOG_INIT_CODE_P0;i<PAG_LOG_INIT_DATA_P0;i++) //Copy the Code Pages to child proces
    set_ss_pag(TP_child,i,get_frame(TP_father,i));


  for(i=PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA;i<PAG_LOG_INIT_DATA_P0+2*NUM_PAG_DATA;i++){//Mapping the data+stack pages of child

    set_ss_pag(TP_father,i,frames[i-(PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA)]);

    copy_data((void *)((i-(NUM_PAG_DATA))*PAGE_SIZE),(void *)((i)*PAGE_SIZE),PAGE_SIZE);

    del_ss_pag(TP_father,i);
  }

  for(i=PAG_LOG_INIT_DATA_P0;i<PAG_LOG_INIT_DATA_P0+NUM_PAG_DATA;i++)//Create new Data+Stack Pages to child proces
    set_ss_pag(TP_child,i,frames[i-PAG_LOG_INIT_DATA_P0]);


  set_cr3(get_DIR(current())); //FLUSH TLB

  ////////////////STATISTICS//////////////////////
  child->task.PID = assign_pid();
  child->task.quantum = QUANTUM;
  child->task.state = ST_READY;
  ///////////////////////////////////////////////

  int ebp;


  __asm__ __volatile__(
      "mov %%ebp,%0\n\t"
      :"=g"(ebp)
      :
      :"memory");


  int des = (unsigned long*)ebp - &father->stack[0]; // Calculate the diference bettwen ebp & esp, necessary for possible values pushed in the stack

  __asm__ __volatile__(
      "mov %0,%%esi"
      :
      :"r"(des));


  child->stack[des] = ((int)child->stack[des] - (int)&father->stack[0]) + (int)&child->stack[0];
  child->stack[des-1] = (unsigned long)&ret_from_fork;
  child->stack[des-2] = (unsigned long)&child->stack[des];

  __asm__ __volatile__(
      "nop"
      :
      :);


  child->task.kernel_esp = &child->stack[des-2];

  PID = child->task.PID;
  list_add_tail(&child->task.list,&readyqueue);

  stats_current_system_to_user();
  return PID;
}

void sys_exit()
{
  stats_current_user_to_system();
  int i;
  free_user_pages(current());
  for(i = PAG_LOG_INIT_CODE_P0; i<PAG_LOG_INIT_DATA_P0; ++i) del_ss_pag(get_PT(current()),i);
  update_current_state_rr(&freequeue);
  sched_next_rr();
  stats_current_system_to_user();
}

int sys_write(int fd, char * buffer, int size)
{
  stats_current_user_to_system();

  int ch_fd = check_fd(fd, ESCRIPTURA);

  if (ch_fd < 0)
  {
    stats_current_system_to_user();
  	return ch_fd;
  }
  if (buffer == NULL)
  {
    stats_current_system_to_user();
    return -EINVAL;
  }
  if (size < 0)
  {
    stats_current_system_to_user();
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
      stats_current_system_to_user();
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

  stats_current_system_to_user();
  return res;
}

int sys_gettime(){
  stats_current_user_to_system();
  stats_current_system_to_user();
  return zeos_ticks;
}

int sys_get_stats(int pid, struct stats * st)
{
  stats_current_user_to_system();

  struct task_struct * target = NULL;
  struct list_head * it;
  
  // Only readyqueue is of interest right now. Alive process = it's inside the
  // ready queue
  list_for_each(it, &readyqueue)
  {
    struct task_struct * it_task = list_head_to_task_struct(it);
    if (it_task->PID)
    {
      target = it_task;
    }
  }

  if (target == NULL)
  {
    stats_current_system_to_user(); 
    return -ESRCH;
  }

  if (copy_to_user(&target->stats, st, sizeof(struct stats)) < 0)
  {
    stats_current_system_to_user();
    return -EFAULT;
  }

  stats_current_system_to_user();
  return 0;
}
