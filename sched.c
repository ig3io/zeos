/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <list.h>
#include <utils.h>

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

  pos = ((int)t - (int) task) /sizeof(union task_union);
  t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

  return 1;
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
  
  idle_task->stats.user_ticks = 0;
  idle_task->stats.system_ticks = 0;
  idle_task->stats.blocked_ticks = 0;
  idle_task->stats.ready_ticks = 0;
  idle_task->stats.elapsed_total_ticks = get_ticks();
  idle_task->stats.total_trans = 0;
  idle_task->stats.remaining_ticks = 0;
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
  task1_pcb->stats.user_ticks = 0;
  task1_pcb->stats.system_ticks = 0;
  task1_pcb->stats.blocked_ticks = 0;
  task1_pcb->stats.ready_ticks = 0;
  task1_pcb->stats.elapsed_total_ticks = get_ticks();
  task1_pcb->stats.total_trans = 0;
  task1_pcb->stats.remaining_ticks = 0;
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


int update_sched_data_rr(void)
{
  current()->quantum--;
  
  // Enclosing function is called at each clock interrupt.
  // Thus only triggered while on user time
  current()->stats.user_ticks = get_ticks() - current()->stats.elapsed_total_ticks;
  // current()->stats.system_ticks++;
  //current()->stats.elapsed_total_ticks++;
  current()->stats.remaining_ticks = current()->quantum;
 
  struct list_head * it_ready;
  list_for_each(it_ready, &readyqueue)
  {
    struct task_struct * task_ready = list_head_to_task_struct(it_ready);
    struct stats stats_ready = task_ready->stats;
    if (task_ready != current()) {
      stats_ready.ready_ticks = get_ticks() - stats_ready.elapsed_total_ticks;
    }
  }

  // There are no more queues of interest, right now

  printc_xy(0, 9, 'Q');
  printc_xy(1, 9, ':');
  printc_xy(2, 9, (current()->quantum/100) + 48);
  printc_xy(3, 9, (current()->quantum%100)/10 + 48);
  printc_xy(4, 9, (current()->quantum%10)/100 + 48);

} 

int needs_sched_rr(void)
{
  if (current()->quantum <= 0) {
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

  if (current() != idle_task) {
    list_del(&current()->list);
    list_add_tail(&current()->list, dest);
  }
  
}


// Same as sched_next_rr, but it assumes the current process will not be
// returned to (it goes to freequeue, no readyqueue)
/*void sched_exit_rr(void)
{
  struct task_struct * next;

  update_current_state_rr(&freequeue);

  if (list_empty(&readyqueue))
  {
    printc_xy(0, 0, 'A');
    next = idle_task;
  }
  else
  {
    printc_xy(0, 0, 'B');
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

  if (next != current())
  {
    printc_xy(0, 0, 'G');
    task_switch((union task_union *)next);
    printc_xy(0, 0, 'F');
  }
}*/

void sched_next_rr(void)
{

  // Quantum to default value
  current()->quantum = QUANTUM;
  int state = current()-> state;

	switch (state){
			case ST_ZOMBIE:
					list_add_tail(&current()->list,&freequeue);
					break;
			case ST_READY:
					list_add_tail(&current()->list,&readyqueue);
					break;
			case ST_BLOCKED:
					//In this case, what it do??
					break;
			default:
					break;
	}

  struct task_struct * next;

 
  // Unless you've called exit(), the current process will always
  // have the option to continue execution 
  
  // TODO unnecessary? redundant?
  if (list_empty(&readyqueue))
  {
    printc_xy(0, 0, 'A');
    next = idle_task;
  }
  else
  {
    printc_xy(0, 0, 'B');
    struct list_head * next_list_elem;
    next_list_elem = list_first(&readyqueue);
    next = list_head_to_task_struct(next_list_elem);
    next->state = ST_RUN;
    //list_del(next_list_elem);
  }

  printc_xy(0, 10, 'C');
  printc_xy(1, 10, ':');
  printc_xy(2, 10, current()->PID + 48);
  printc_xy(0, 11, 'N');
  printc_xy(1, 11, ':');
  printc_xy(2, 11, next->PID + 48);

  if (next != current())
  {
    //printc_xy(0, 0, 'F');
    task_switch((union task_union *)next);
    //printc_xy(0, 0, 'G');
  }
}
