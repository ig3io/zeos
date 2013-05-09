/*
 * interrupt.h - Definició de les diferents rutines de tractament d'exepcions
 */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <types.h>
#include <entry.h>

#define IDT_ENTRIES 256
#define BUFFER_SIZE 30 

extern Gate idt[IDT_ENTRIES];
extern Register idtR;

extern unsigned int zeos_ticks;

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL);
void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL);

void setIdt();

void keyboard_routine();
void clock_routine();

#endif  /* __INTERRUPT_H__ */
