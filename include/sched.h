/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>
#include <stats.h>

#define NR_TASKS    10
#define KERNEL_STACK_SIZE   1024
#define QUANTUM		100
#define NR_SEMS     20
#define BUFFER_SIZE 40 

extern int current_quantum;

extern int page_table_refs[NR_TASKS];

struct circular_buffer {
  char buffer[BUFFER_SIZE];
  char * start;
  char * end;
};

extern struct circular_buffer buffer;

enum state_t { ST_RUN, ST_READY, ST_BLOCKED, ST_ZOMBIE };

struct task_struct {
  int PID;			/* Process ID */
  page_table_entry * dir_pages_baseAddr;
  unsigned long *kernel_esp;
  unsigned long *heap;
  unsigned long *heap_top;
  unsigned long *heap_break;
  int quantum;
  unsigned int read_pending;
  struct stats stats;
  enum state_t state;
  struct list_head list;
};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

extern union task_union task[NR_TASKS]; /* Vector de tasques */
extern struct task_struct *idle_task;

extern struct list_head blocked;
extern struct list_head keyboardqueue;
extern struct list_head freequeue;
extern struct list_head readyqueue;

struct sem_struct {
  struct task_struct * owner;
  int count;
  struct list_head list;
};


extern struct sem_struct semaphores[NR_SEMS];

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

int allocate_DIR(struct task_struct *t);
int allocate_DIR_clone(struct task_struct *t);

/* Headers for the scheduling policy */
void sched_next_rr();
void update_current_state_rr(struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();

/* Stats */
void stats_current_system_to_user();
void stats_current_user_to_system();
void stats_current_system_to_ready();
void stats_current_ready_to_system();

void stats_update_system_to_user(struct stats * st);
void stats_update_user_to_system(struct stats * st);
void stats_update_system_to_ready(struct stats * st);
void stats_update_ready_to_system(struct stats * st);
void stats_init(struct stats * st);

/* Queue gestion*/

void move_to_queue(struct list_head *queue_1, struct list_head *queue_2);

/* Buffer gestion*/
int buffer_size();
void push(char c);
char pop();
void pop_i(int size);

#endif  /* __SCHED_H__ */
