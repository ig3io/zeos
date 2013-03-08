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
  struct task_struct idle;
  idle = *list_head_to_task_struct(list_first(&freequeue)); // Uses list_first
  idle.PID=0;
  // This is the stack initialization, right? - Ignacio

  union task_union idle_stack = (union task_union *)idle;

  // TODO: is unsigned int the best type possible?
  idle_stack[KERNEL_STACK_SIZE - 1] = (unsigned int)&cpu_idle;
  idle_stack[KERNEL_STACK_SIZE - 2] = 0;  // Dummy value

/*
  __asm__ __volatile__(
      "pushl %0\n\t"
      "pushl $0x00" // value does not matter
      : // No output
      : "r" (&cpu_idle) 
      );
  // TODO ^ asm inline not tested
  idle_task = &idle;
  
  __asm__ __volatile__(
     "movl %%esp, %0\n\t"
     : // No output TODO UNTESTEEED
     : "r" (idle.kernel_esp)
     );
  */
}

void init_task1(void)
{
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
