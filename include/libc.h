/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>

int write(int fd, char *buffer, int size);

int gettime();

void itoa(int a, char *b);

int strlen(char *a);

int getpid();

int fork();

int clone(void *(function) (void), void *stack);

void exit();

int get_stats(int pid, struct stats * st);

int perror(char * msg);

int sem_init(int n_sem, unsigned intvalue);

int sem_wait(int n_sem);

int sem_signal(int n_sem);

int sem_destroy(int n_sem);

#endif  /* __LIBC_H__ */
