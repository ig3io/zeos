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
  copy_data(father, child, sizeof(union task_union));

  allocate_DIR(&(child->task));

  page_table_entry* TP_child = get_PT(&child->task);
  page_table_entry* TP_father = get_PT(&father->task);


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
  //int i;
  int pos = (current()->dir_pages_baseAddr - &dir_pages[0][0] )/((sizeof(page_table_entry))); //* TOTAL_PAGES);
  page_table_refs[pos]--;	
  if(page_table_refs[pos]==0){
    free_user_pages(current());
  }
  update_current_state_rr(&freequeue);
  sched_next_rr();
  stats_current_system_to_user();
}


int sys_clone(void *(function) (void), void *stack)
{
  stats_current_user_to_system();

  int PID=-1;

  struct list_head *free_pcb = list_first(&freequeue);// take the first free PCB
  /* if freequeue don't have any element, return an error(not implemented)*/
  union task_union *child = (union task_union*)list_head_to_task_struct(free_pcb);
  union task_union *father = (union task_union*)current();
  list_del(free_pcb);
  /* create a variable for store the free frames that we need for save data+Kernel pages*/
 
  copy_data(father, child, sizeof(union task_union));

  allocate_DIR_clone(&(child->task));

  ////////////////STATISTICS//////////////////////
  child->task.PID = assign_pid();
  child->task.quantum = QUANTUM;
  child->task.state = ST_READY;
  ///////////////////////////////////////////////

  int ebp;


  __asm__ __volatile__(
      "mov %%ebp,%0\n\t"
      "nop\n\t"
      :"=g"(ebp)
      :
      :"memory");


  int des = (unsigned long*)ebp - &father->stack[0]; // Calculate the diference bettwen ebp & esp, necessary for possible values pushed in the stack

  __asm__ __volatile__(
      "mov %0,%%esi"
      :
      :"r"(&child->stack[0]));

  child->stack[des] = (unsigned long) ebp; //Same as father, so assign the ebp value

    __asm__ __volatile__(
      "mov %0,%%esi"
      :
      :"r"(child->stack[des]));
  //child->stack[des+1] =(unsigned long)  &ret_from_fork;
  child->stack[des+7] = (unsigned long) stack;
  child->stack[des+13] = (unsigned long) function;

  child->task.kernel_esp = &child->stack[des];// IDEM (we could asign the ebp variable, the same result(in theory=))

  PID = child->task.PID;
  list_add_tail(&child->task.list,&readyqueue);

  stats_current_system_to_user();
  return PID;
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
  //printc_xy(10, 10, 'J');
  stats_current_user_to_system();

  struct task_struct * target = NULL;
  struct list_head * it;
  // Only readyqueue is of interest right now
  list_for_each(it, &readyqueue)
  {
    struct task_struct * it_task = list_head_to_task_struct(it);
    if (it_task->PID == pid && it_task->state == ST_READY)
    {
      target = it_task;
    }
  }

  //printc_xy(10, 10, 'k');

  if (target == NULL)
  {
    stats_current_system_to_user(); 
    return -1;
  }

  //printc_xy(10, 10, 'L');
  if (copy_to_user(&target->stats, st, sizeof(struct stats) < 0))
  {
    stats_current_system_to_user();
    return -1; 
  }

  //printc_xy(10, 10, 'M');
  
  stats_current_system_to_user();
  return 0;
}

int sem_is_valid_number(int n_sem)
{
  if (n_sem >= NR_SEMS || n_sem < 0)
  {
    return 0;
  }
  return 1;
}

int sys_sem_init(int n_sem, unsigned int value)
{
  if (!sem_is_valid_number(n_sem))
  {
    return -1;
  }

  struct sem_struct * sem = &semaphores[n_sem];

  if (sem->owner != NULL)
  {
    return -1;
  }

  semaphores[n_sem].count = value;
  semaphores[n_sem].owner = current();
  INIT_LIST_HEAD(&semaphores[n_sem].list);
  //I think that each semaphores have his blockedqueue, so it's necesary initialize it!!(not sure XD)
  // Ignacio: It's been initialized in init_sched!
  // But okaay, since the work.pdf says it should be initialized here...
  // list head already initialized (init_sched)
  return 0;
}

int sys_sem_wait(int n_sem)
{
  if (!sem_is_valid_number(n_sem))
  {
    return -1;
  }

  struct sem_struct * sem = &semaphores[n_sem];

  if (sem->owner == NULL)
  {
    return -1;
  }

  if (sem->count > 0)
  {
    sem->count--;
  }
  else
  {

    //char * debug = "Sem: about to block a thread\n";
    //sys_write(1, debug, 29);

    list_del(&current()->list);
    list_add_tail(&current()->list, &sem->list);
    current()->state = ST_BLOCKED;
    
    //debug = "Sem: thread moved to semaphore list\n";
    //sys_write(1, debug, 36);

    // We're blocking the process, so we schedule the next one
    sched_next_rr();
  }

  // Now we check if the semaphore was destroyed
  //char * debug = "Sem: about to going back... \n";
  //sys_write(1, debug, 29);

  if (sem->owner == NULL)
  {
    //debug = "Sem: (note: it's destroyed!)\n";
    //sys_write(1, debug, 29);

    return -1;
  }

  return 0;
}

int sys_sem_signal(int n_sem)
{
  if (!sem_is_valid_number(n_sem))
  {
    return -1;
  }

  struct sem_struct * sem = &semaphores[n_sem];
  
  if (sem->owner == NULL)
  {
    return -1;
  }

  if (list_empty(&sem->list))
  {
    sem->count++;
  }
  else
  {
    //delete the proces from the blockedqueue of this semaphore and 'ready it'
    struct list_head * elem = list_first(&sem->list);
    list_del(elem);
    list_add_tail(elem, &readyqueue);
    struct task_struct * task = list_head_to_task_struct(elem); 
    task->state = ST_READY;
  }
  
  return 0;
}


int sys_sem_destroy(int n_sem)
{
  if (!sem_is_valid_number(n_sem))
  {
    return -1;
  }

  struct sem_struct * sem = &semaphores[n_sem];

  if (sem->owner==NULL || sem->owner != current())
  {
  	return -1;
  }
  
  sem->owner = NULL;

  // TODO make the processes return a -1
  while(!list_empty(&sem->list))
  {
    //delete every process from the blocked queue (and put them on the ready)
    struct list_head * elem = list_first(&sem->list);
    list_del(elem);
    list_add_tail(elem, &readyqueue);
    struct task_struct * task = list_head_to_task_struct(elem); 
    task->state = ST_READY;
  }
  
  
  return 0;
}
