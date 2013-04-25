/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <list.h>
#include <utils.h>

int page_table_refs[NR_TASKS] = { 0 };

union task_union task[NR_TASKS]
__attribute__((__section__(".data.task")));


#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif 

extern struct list_head blocked;

struct list_head freequeue;
struct list_head readyqueue;

struct task_struct *idle_task;

struct sem_struct semaphores[NR_SEMS];

int current_quantum = 0;

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
  return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
  return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
  int pos;
  for (pos=0;pos<NR_TASKS;pos++){
	  if(page_table_refs[pos] == 0){
      page_table_refs[pos]++;
      t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];
      return 1;
	  }
  }
  //pos = ((int)t - (int) task) /sizeof(union task_union);
  //t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

  return -1;
}

int allocate_DIR_clone(struct task_struct *t)
{
	int pos = (int)((t->dir_pages_baseAddr-&dir_pages[0][0])/sizeof(page_table_entry));
	page_table_refs[pos]++;
	return 1;
	//TODO Return -1 in error case
}

void cpu_idle(void)
{
  __asm__ __volatile__("sti": : :"memory");

  while(1)
  {
    ;
  }
}


void init_idle (void)
{
  struct list_head * list_elem = list_first(&freequeue);
  idle_task = list_head_to_task_struct(list_elem);
  list_del(list_elem);

  idle_task->PID = 0;
  idle_task->quantum = QUANTUM;
  idle_task->state = ST_READY;


  unsigned long *idle_stack = ((union task_union *)idle_task)->stack;

  idle_stack[KERNEL_STACK_SIZE - 1] = (unsigned long)&cpu_idle;
  idle_stack[KERNEL_STACK_SIZE - 2] = (unsigned long)0;  // Dummy value

  idle_task->kernel_esp = (unsigned long *)&idle_stack[KERNEL_STACK_SIZE - 2];

  stats_init(&idle_task->stats);
}

void init_task1(void)
{
  struct list_head * list_elem = list_first(&freequeue);
  list_del(list_elem);
  struct task_struct * task1_pcb = list_head_to_task_struct(list_elem);

  task1_pcb->PID = 1;
  task1_pcb->quantum=QUANTUM;
  task1_pcb->state=ST_READY;

  // It has to be added. The only process that is not in any queue at any
  // moment is the idle task
  list_add_tail(&task1_pcb->list, &readyqueue);

  set_user_pages(task1_pcb);
  set_cr3(get_DIR(task1_pcb));

  // TODO statistics
  stats_init(&task1_pcb->stats);
}

void inner_task_switch(union task_union *new)
{
  page_table_entry * new_proc_pages = get_DIR(&new->task);

	__asm__ __volatile__(
				"movl %%ebp,%0"
				:"=r"(current()->kernel_esp) : : "memory");	

  tss.esp0= (unsigned long)&(new->stack[KERNEL_STACK_SIZE]);
  if(new->task.dir_pages_baseAddr != current()-> dir_pages_baseAddr)set_cr3(new_proc_pages);

  __asm__ __volatile__ (
      "movl %0, %%esp\n\t"
      "popl %%ebp\n\t"
      "ret\n\t"
      : /* no output */
      :"r" (new->task.kernel_esp) : "memory"
      );
}

void task_switch(union task_union *new)
{
  __asm__ __volatile__ (
      "pushl %%esi\n\t"
      "pushl %%edi\n\t"
      "pushl %%ebx\n\t"
      :
      :
      );

  inner_task_switch(new);


  __asm__ __volatile__ (
      "popl %%ebx\n\t"
      "popl %%edi\n\t"
      "popl %%esi\n\t"
      :
      :
      );
}

void init_sched(){
  INIT_LIST_HEAD(&freequeue);
  INIT_LIST_HEAD(&readyqueue);

  int i = 0;
  for (i = 0; i < NR_TASKS; i++)
  {
    list_add_tail(&(task[i].task.list), &freequeue);
  }

  int j = 0;
  for (j = 0; j < NR_SEMS; j++)
  {
    INIT_LIST_HEAD(&semaphores[j].list);
    semaphores[j].count = 0;
    semaphores[j].owner = NULL;
  }

  current_quantum = QUANTUM;
}

struct task_struct* current()
{
  int ret_value;

  __asm__ __volatile__(
      "movl %%esp, %0"
      : "=g" (ret_value)
      );

  return (struct task_struct*)(ret_value&0xfffff000);
}

void print_current_quantum()
{
  printc_xy(0, 9, 'Q');
  printc_xy(1, 9, ':');
  printc_xy(2, 9, (current_quantum/100) + 48);
  printc_xy(3, 9, (current_quantum%100)/10 + 48);
  printc_xy(4, 9, (current_quantum%10)/100 + 48);
}

void update_sched_data_rr(void)
{
  current_quantum--;
  //current()->quantum--;
  print_current_quantum();
} 

int needs_sched_rr(void)
{
  if (current_quantum <= 0) {
    return 1;
  }
  else {
    return 0; 
  }
}

void update_current_state_rr(struct list_head *dest)
{

  if (dest == &freequeue)
  {
    current()->state = ST_ZOMBIE;
  }
  else if (dest == &readyqueue)
  {
    current()->state = ST_READY;
  }
  else
  {
    current()->state = ST_BLOCKED;
  }

  // Remove from current queue and move it to dest (if it's not the idle_task)
  if (current() != idle_task)
  {
    list_del(&current()->list);
    list_add_tail(&current()->list, dest);
  }
}

void sched_next_rr(void)
{
  // Quantum to default value
  //current()->quantum = QUANTUM;
  //current_quantum = QUANTUM;

  struct task_struct * next;
  
  //Check if we have proces in the readyqueue
  if (list_empty(&readyqueue))
  {
    next = idle_task;
  }
  else
  {
    struct list_head * next_list_elem;
    next_list_elem = list_first(&readyqueue);
    next = list_head_to_task_struct(next_list_elem);
    next->state = ST_RUN;
  }

  printc_xy(0, 10, 'C');
  printc_xy(1, 10, ':');
  printc_xy(2, 10, current()->PID + 48);
  printc_xy(0, 11, 'N');
  printc_xy(1, 11, ':');
  printc_xy(2, 11, next->PID + 48);

  // Restore quantum
  current_quantum = next->quantum;

  if (next != current())
  {
    // Stats updating
    stats_update_ready_to_system(&next->stats);
    stats_update_system_to_ready(&current()->stats);

    task_switch((union task_union *)next);
  }
}


/* Stats updating functions */
void stats_current_system_to_user()
{
  stats_update_system_to_user(&current()->stats);
}

void stats_current_user_to_system()
{
  stats_update_user_to_system(&current()->stats);
}

void stats_current_system_to_ready()
{
  stats_update_system_to_ready(&current()->stats);
}

void stats_current_ready_to_system()
{
  stats_update_ready_to_system(&current()->stats);
}

void stats_update_user_to_system(struct stats * st)
{
  st->user_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}

void stats_update_system_to_user(struct stats * st)
{
  st->system_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}

void stats_update_system_to_ready(struct stats * st)
{
  st->system_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}

void stats_update_ready_to_system(struct stats * st)
{
  st->ready_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
  st->total_trans++;  // READY -> RUN (sys)
}

void stats_init(struct stats * st)
{
  st->user_ticks = 0;
  st->system_ticks = 0;
  st->blocked_ticks = 0;
  st->ready_ticks = 0;
  st->elapsed_total_ticks = get_ticks();
  st->total_trans = 0;
  st->remaining_ticks = 0;
}
