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

	pos = ((int)task-(int)t)/sizeof(union task_union);

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
  // We need the first PCB
  idle_task = list_head_to_task_struct(list_first(&freequeue));
  list_del(list_first(&freequeue));
  
  idle_task->PID = 0;

  
  unsigned long *idle_stack = ((union task_union *)idle_task)->stack;

  // TODO:
  // is unsigned int the best type possible?
  // Yes, in the zeOS document they said!
  // OK!
  idle_stack[KERNEL_STACK_SIZE - 1] = (unsigned int *)&cpu_idle;
  idle_stack[KERNEL_STACK_SIZE - 2] = 0;  // Dummy value
  
  idle_task->kernel_esp = &idle_stack[KERNEL_STACK_SIZE - 2];


  // TODO: rest of stuff to initialize
}

void init_task1(void)
{
}

void inner_task_switch(union task_union *new)
{
  __asm__ __volatile__ (
    "pushl %%ebp\n\t"
    "movl %%esp, %%ebp\n\t"
    :
    :
  );
  struct task_struct * current_proc = current();
  
  __asm__ __volatile__ (
    "movl %%ebp, (%0)"
    : "=r" (current_proc->kernel_esp)
  );

  __asm__ __volatile__ (
    "movl %0, %%esp\n\t"
    "movl %%ebp, %%esp\n\t"
    "popl %%ebp\n\t"
    "ret\n\t"
    : "=r" (new->task.kernel_esp)
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
    list_add(&(task[i].task.list), &freequeue);
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


// Scheduling. Necessary for E2?

int update_sched_data_rr(void)
{
  // TODO
} 

int needs_sched_rr(void)
{
  // TODO 
}

void update_current_state_rr(struct list_head *dest)
{
  // TODO
}

void sched_next_rr(void)
{
  // TODO
}
