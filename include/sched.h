/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>

#define NR_TASKS      10
#define KERNEL_STACK_SIZE	1024
#define QUANTUM		500

enum state_t { ST_RUN, ST_READY, ST_BLOCKED, ST_ZOMBIE };

struct task_struct {
  int PID;			/* Process ID */
  page_table_entry * dir_pages_baseAddr;
  unsigned long *kernel_esp;
  int quantum;
  enum state_t state;
  struct list_head list;
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

extern union task_union task[NR_TASKS]; /* Vector de tasques */
extern struct task_struct *idle_task;

extern struct list_head freequeue;
extern struct list_head readyqueue;

#define KERNEL_ESP       (DWord) &task[1].stack[KERNEL_STACK_SIZE]

/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_dummy(void);

void init_sched(void);

struct task_struct * current();

void task_switch(union task_union *new);

struct task_struct *list_head_to_task_struct(struct list_head *l);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
void sched_next_rr();
void sched_exit_rr();
void update_current_state_rr(struct list_head *dest);
int needs_sched_rr();
int update_sched_data_rr();

#endif  /* __SCHED_H__ */
