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
int counter_printer=0;
int counter_printer2 = 5;

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
  if (permissions!=ESCRIPTURA && permissions!=LECTURA) return -EACCES; /*EACCES*/
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

  // TODO return an error if freequeue is empty
  if (list_empty(&freequeue))
  {
    return -1;
  }

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

  int n_frames = ((unsigned long) (current()->heap)/PAGE_SIZE)-HEAPSTART;
  if((unsigned long)current()->heap % PAGE_SIZE !=0) ++n_frames;
  if(n_frames > 0){
    ///////////////// COLLECTING FREE FRAMES ////////////////////////
    int heap_frames[n_frames];
    int i;
    for (i=0;i<n_frames;++i){
      heap_frames[i] = alloc_frame();
      if(frames[i]==-1){
       while(i >=0) free_frame(frames[i--]);
        return -ENOMEM;
      }
    }
    ///////////////////////////////////////////////////////////////////

    //////////Copiando heap//////////////////////////////////////////////////

    for(i=0;i<n_frames;++i){
      set_ss_pag(TP_child,i+HEAPSTART,heap_frames[i]);
      set_ss_pag(TP_father,i+HEAPSTART,heap_frames[i]);
      copy_data((void *) ((HEAPSTART+i) * PAGE_SIZE), (void *)((HEAPSTART+i+n_frames)*PAGE_SIZE),PAGE_SIZE);
      del_ss_pag(TP_father,HEAPSTART+i+n_frames);
    }

  }

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
  int pos = (current()->dir_pages_baseAddr - &dir_pages[0][0] )/((sizeof(page_table_entry))* TOTAL_PAGES); // TODO
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

  // TODO refine return code
  if (list_empty(&freequeue))
  {
    return -1;
  }

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

  unsigned long ebp;


  __asm__ __volatile__(
      "mov %%ebp,%0\n\t"
      "nop\n\t"
      :"=g"(ebp)
      :
      :"memory");

  // Calculate the diference bettwen ebp & esp, necessary for possible values pushed in the stack
  //int des = (int)((unsigned long *)ebp - &father->stack[0])/(sizeof(unsigned long));
  int des = (int)((unsigned long *)ebp - &father->stack[0]);

  __asm__ __volatile__(
      "mov %0,%%esi"
      :
      :"r"(&child->stack[0]));

  child->stack[des] = (unsigned long) ebp; //Same as father, so assign the ebp value

    __asm__ __volatile__(
      "mov %0,%%esi"
      :
      :"r"(child->stack[des]));
  
  
  //// child->stack[des+1] =(unsigned long)  &ret_from_fork;
  child->stack[des + 7] = (unsigned long) stack;
  child->stack[des + 13] = (unsigned long) function;
  child->stack[des+16] = (unsigned long) stack;
  

  child->stack[des] = (unsigned long)ebp;
  ////child->stack[des + 1] = (unsigned long)&ret_from_fork;
  //child->stack[des - 2] = (unsigned long)stack;
  //child->stack[des - 1] = (unsigned long)function;

  child->task.kernel_esp = &child->stack[des];// IDEM (we could asign the ebp variable, the same result(in theory=))

  PID = child->task.PID;
  list_add_tail(&child->task.list,&readyqueue);

  stats_current_system_to_user();
  return PID;
}

void *sys_sbrk(int increment)
{

  if(current()->heap + increment < HEAPSTART*PAGE_SIZE)
  {

    #if AWESOME_FEATURE
    while(current()->heap_top != HEAPSTART * PAGE_SIZE)
    {
        page_table_entry* actual= get_PT(current());
        free_frame(get_frame(actual,PH_PAGE((int)current()->heap_top)));
        del_ss_pag(actual,PH_PAGE((int)current()->heap_top));
        current()->heap_top -= PAGE_SIZE;
        current()->heap_break -= PAGE_SIZE;
        counter_printer=1;
        #ifdef DEBUG
        printc_xy(4,20,counter_printer+48);
        #endif

    }
    current()->heap = HEAPSTART * PAGE_SIZE;
    #endif

    return -ENOMEM;
  } 

  int heap_inicial = current()->heap;
  current()-> heap += increment;//incrementamos el heap
  int heap_actual = current()->heap;

  /*CASO BASE - HEAP no inilicializado*/
  if(current()->heap_break == (HEAPSTART*PAGE_SIZE)){
    int frame = alloc_frame();
    if(frame==-1){
        return -ENOMEM;
    }
    set_ss_pag(get_PT(current()),PH_PAGE(heap_inicial),frame);
    current()->heap_break += PAGE_SIZE;

    #if DEBUG
    ++counter_printer;
    printc_xy(4,20,counter_printer+48);
    #endif
  }
  /////////////////////////////////////////////////////////////////

  if(increment>0)
  {

    //COLLECT FRAMES///////////////////
    int n_frames=0;
    while(!(heap_actual>=current()->heap_top && heap_actual<=current()->heap_break)){
        ++n_frames;
        current()->heap_top +=PAGE_SIZE;
        current()->heap_break +=PAGE_SIZE;
    }//Calculate number of necessary frames

    if(heap_actual == current()->heap_break){
      ++n_frames;
      current()->heap_top +=PAGE_SIZE;
      current()->heap_break +=PAGE_SIZE;
    }//if the actual heap it's equal that the heap_break take one more frame

    #if DEBUG
    counter_printer += n_frames;
    printc_xy(4,20,counter_printer+48);
    #endif

    int frames[n_frames];
    int i;
    for (i=0;i<n_frames;++i){
      frames[i] = alloc_frame();
      if(frames[i]==-1){
      while(i >=0) free_frame(frames[i--]);
      return -ENOMEM;
      }
    }
    ////////////////////////////////////
    for(i=0;i<n_frames;++i){
      set_ss_pag(get_PT(current()),i+PH_PAGE(heap_inicial),frames[i]);
    }
  }

  if(increment<0){

    while(!(heap_actual>=current()->heap_top && heap_actual<=current()->heap_break)){
        page_table_entry* actual= get_PT(current());
        free_frame(get_frame(actual,PH_PAGE((int)current()->heap_top)));
        del_ss_pag(actual,PH_PAGE((int)current()->heap_top));
        current()->heap_top -= PAGE_SIZE;
        current()->heap_break -= PAGE_SIZE;

        #if DEBUG
        --counter_printer;
        printc_xy(4,20,counter_printer+48);
        #endif
    }
  }

  return (void*) heap_actual;
}

int sys_write(int fd, char * buffer, int size)
{
  stats_current_user_to_system();

  int ch_fd = check_fd(fd, LECTURA);

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

int sys_read_keyboard(char * buf, int count)
{

  current()->read_pending = count;

  if (!list_empty(&keyboardqueue))
  {
    struct list_head * elem = &current()->list;
    list_del(elem);
    list_add_tail(elem, &keyboardqueue);
    sched_next_rr();
  }

  int current_read = 0;
  unsigned int * current_count = &current()->read_pending;
  
  while (*current_count > 0)
  {
  /*
    if (buffer_size() == BUFFER_SIZE)
    {
      #ifdef DEBUG
      printc_xy(14,21,'E');
      #endif
      if (buffer.start > buffer.end)
      {
        int len_a = &buffer.buffer[BUFFER_SIZE] - buffer.start;
        if (copy_to_user(buffer.start, buf + current_read, len_a) < 0)
        {
          return -1;
        }
        pop_i(len_a);
        *current_count -= len_a;
        current_read += len_a;
        char * start = &buffer.buffer[0];
        int len_b = buffer.end - start;
        if (copy_to_user(start, buf + len_a + current_read, len_b) < 0)
        {
          return -1;
        }
        pop_i(len_b);
        *current_count -= len_b;
        current_read += len_b;
      }
      else
      {
        #ifdef DEBUG
        printc_xy(14,22,'E');
        #endif
        if (copy_to_user(buffer.start, buf + current_read, buffer_size()) < 0)
        {
          return -1;
        }
        pop_i(buffer_size());
        *current_count -= buffer_size();
        current_read += buffer_size();
      }
      struct list_head * elem = &current()->list;
      list_del(elem);
      list_add(elem, &keyboardqueue);
      sched_next_rr(); //TODO we should preserve the queueing politic
    }
    else
    if (buffer_size() >= count)*/
    {
      // TODO error detection
      if (buffer.start > buffer.end)
      {
        int len_a = &buffer.buffer[BUFFER_SIZE] - buffer.start;
        if (copy_to_user(buffer.start, buf + current_read, len_a) < 0)
        {
          return -1;
        }
        pop_i(len_a);
        *current_count -= len_a;
        current_read += len_a;

        //int len_b = *current_count;
        int len_b = buffer.end - &buffer.buffer[0];
        char * start = &buffer.buffer[0];
        if (copy_to_user(start, buf + len_a + current_read, len_b) < 0)
        {
          return -1;
        }
        pop_i(len_b);
        *current_count -= len_b;
        current_read += len_b;
      }
      else
      {
        int size = buffer_size();
        if (copy_to_user(buffer.start, buf + current_read, size) < 0)
        {
          return -1;
        }
        pop_i(size);
        *current_count -= size;
        current_read += size;
      }
    }

    
    //else
    if (*current_count > 0)
    {
      struct list_head * elem = &current()->list;
      list_del(elem);
      list_add_tail(elem, &keyboardqueue);
      sched_next_rr();
    }

    #ifdef DEBUG
    printc_xy(2, 22, *current_count/10 + 48);
    printc_xy(3, 22, *current_count%10 + 48);
    printc_xy(5, 22, current_read/10 + 48);
    printc_xy(6, 22, current_read%10 + 48);
    #endif
  }
  return current_read;
}

int sys_read(int fd, char * buf,int count)
{
  stats_current_user_to_system();
  #ifdef DEBUG
  printc_xy(15,13,'1');
  //Debug_buffer();
  #endif

  int ch_fd =check_fd(fd, ESCRIPTURA);

  if (ch_fd < 0)
  {
    stats_current_system_to_user();
    return ch_fd;
  }
  if (count < 0)
  {
    stats_current_system_to_user();
    return -EINVAL;
  }

  return sys_read_keyboard(buf, count);
}


int sys_gettime(){
  stats_current_user_to_system();
  stats_current_system_to_user();
  return zeos_ticks;
}

int sys_get_stats(int pid, struct stats * st)
{
  stats_current_user_to_system();

  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats)))
  {
    return -EACCES;
  }

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

  if (target == NULL)
  {
    stats_current_system_to_user(); 
    return -1;
  }

  if (copy_to_user(&target->stats, st, sizeof(struct stats) < 0))
  {
    stats_current_system_to_user();
    return -1; 
  }
  
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

