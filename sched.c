/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include<list.h>

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

  pos = ((int)t-(int)task)/sizeof(union task_union);

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

void cpu_dummy(void)
{
  printc_xy(0, 0, 'Z');
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

  // TODO:
  // Is unsigned int the best type possible? Yes, in the zeOS document they said!
  idle_stack[KERNEL_STACK_SIZE - 1] = (unsigned int *)&cpu_idle;
  idle_stack[KERNEL_STACK_SIZE - 2] = 0;  // Dummy value

  idle_task->kernel_esp = (unsigned int *)&idle_stack[KERNEL_STACK_SIZE - 2];

  // TODO: rest of stuff to initialize
}

void init_dummy (void)
{
  struct list_head * list_elem = list_first(&freequeue);
  struct task_struct * dummy_task = list_head_to_task_struct(list_elem);
  list_del(list_elem);

  dummy_task->PID = 2;
  dummy_task->quantum = QUANTUM;
  dummy_task->state = ST_READY;

  // TODO
  list_add_tail(&dummy_task->list, &readyqueue);

  unsigned long *dummy_stack = ((union task_union *)dummy_task)->stack;

  // TODO:
  // Is unsigned int the best type possible? Yes, in the zeOS document they said!
  dummy_stack[KERNEL_STACK_SIZE - 1] = (unsigned int *)&cpu_dummy;
  dummy_stack[KERNEL_STACK_SIZE - 2] = 0;  // Dummy value

  dummy_task->kernel_esp = (unsigned int *)&dummy_stack[KERNEL_STACK_SIZE - 2];

  // TODO: rest of stuff to initialize
}

void init_task1(void)
{
  struct list_head * list_elem = list_first(&freequeue);
  struct task_struct * task1_pcb = list_head_to_task_struct(list_elem);
  list_del(list_elem);

  task1_pcb->PID = 1;
  task1_pcb->quantum=QUANTUM;
  task1_pcb->state=ST_READY;

  // TODO
  //list_add_tail(&task1_pcb->list, &readyqueue);

  set_user_pages(task1_pcb);
  set_cr3(get_DIR(task1_pcb));
}

void inner_task_switch(union task_union *new)
{
  page_table_entry * new_proc_pages = get_DIR(&new->task);
  // current_proc_pages should be saved somewhere??
  // No. It can be obtainer through get_DIR

  tss.esp0= &(new->stack[KERNEL_STACK_SIZE]);
  set_cr3(new_proc_pages);


 // __asm__ __volatile__ (
 //     "pushl %%ebp\n\t"
 //     "movl %%esp, %%ebp\n\t"
 //     :
 //     :
 //     );
  struct task_struct * current_proc = current();

//  __asm__ __volatile__ (
//      "movl %%ebp, (%0)"
//      : /* no output */
//      : "r" (&current_proc->kernel_esp)
//      );

  __asm__ __volatile__ (
      //"movl %0, %%esp\n\t"
      //"movl %%ebp, %%esp\n\t"
      "movl %%ebp, (%0)\n\t"
      "movl %1, %%esp\n\t"
      "popl %%ebp\n\t"
      "ret\n\t"
      : /* no output */
      :"r" (&current_proc->kernel_esp), "r" (new->task.kernel_esp)
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

unsigned int count = 0;

void sched_next_rr(void)
{
  count++;

  // Quantum to default value
  current()->quantum = QUANTUM;
  
  struct task_struct * next;

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
    printc_xy(0, 0, 'C');
    update_current_state_rr(&readyqueue);
    printc_xy(0, 0, 'D');
    printc_xy(0, 0, 'E');
    
    printc_xy(0, 0, 'F');
    next->state = ST_RUN;
    list_del(&next->list);
    printc_xy(0, 0, 'G');
    task_switch((union task_union *)next);
  }
}
