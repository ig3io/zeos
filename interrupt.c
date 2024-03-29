/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>

#include <zeos_interrupt.h>

// TODO
#include <sched.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

// Clock initialization and declaration
unsigned int zeos_ticks = 0;

char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','¡','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','ñ',
  '\0','º','\0','ç','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}


void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler(33, keyboard_handler, 0); 
  setInterruptHandler(32, clock_handler, 0); 

  setTrapHandler(0x80, system_call_handler, 3);

  set_handlers();

  set_idt_reg(&idtR);
}

void keyboard_routine()
{
  // We read the data register port   
  unsigned char input = inb(0x60);
  unsigned char is_break = input >> 7;
  unsigned char scan_code = input & 0x7F;
  // Checking bit 7. It's a make (0) or a break (1)?
  if (!is_break)
  {
    unsigned char key_char = ' ';
    if (scan_code < 128)
    {
      key_char = char_map[scan_code];
    }

    #ifdef DEBUG
    /* Keyboard debug */
      printc_xy(0, 13, 'K');
      printc_xy(1, 13, 'e');
      printc_xy(2, 13, 'y');
      printc_xy(3, 13, ':');
      printc_xy(4, 13, key_char);
      printc_xy(6,13,'1');
    /* End keyboard debug */
    #endif

    // TODO implementation dependant behaviour? a la TCP receiver: drop when
    // the buffer overflows
    if(buffer_size() < BUFFER_SIZE)
    {
      push(key_char);
    }

    #ifdef DEBUG
    debug_buffer();
    #endif
      
    if(!list_empty(&keyboardqueue))
    {
      struct task_struct * to_unblock = list_head_to_task_struct(list_first(&keyboardqueue));
      int last_size_request = to_unblock->read_pending;
      /*
      TODO there are some problems with the cycling of the buffer. This is the
      safest approach in order to avoid illegal access. We're facing a hard
      to find bug
      */
      if (last_size_request <= buffer_size() || buffer_size() >= BUFFER_SIZE/2 || buffer.end == &buffer.buffer[BUFFER_SIZE])
      {
        struct list_head * elem = &to_unblock->list;
        list_del(elem);
        list_add_tail(elem, &readyqueue);
        to_unblock->state = ST_READY;
        stats_update_blocked_to_system(&to_unblock->stats);
        // TODO: accelerates testing, but screws with mutiple readers.
        //sched_next_rr();
      }    
    }
  }
}

unsigned int times_sched = 0;

void clock_routine()
{
  zeos_show_clock();
  ++zeos_ticks;
  update_sched_data_rr();
  if (needs_sched_rr())
  {
    times_sched++;
    #ifdef DEBUG
      printc_xy(0, 8, 'S');
      printc_xy(1, 8, times_sched + 1);
    #endif
    update_current_state_rr(&readyqueue);
    sched_next_rr();
  }
}

/* PRINT THE BUFFER */
void debug_buffer()
{
    int ini = buffer.start - &buffer.buffer[0];
    int fin = buffer.end - &buffer.buffer[0];
    printc_xy(8, 13,  ini/10 + 48);
    printc_xy(9, 13,  ini%10 + 48);
    printc_xy(11, 13, fin/10 + 48);
    printc_xy(12, 13, fin%10 + 48);
    printc_xy(14, 13, buffer_size()/10 + 48);
    printc_xy(15, 13, buffer_size()%10 + 48);
    int print_column=6;
    int i = 0;
    for (i = 0; i < BUFFER_SIZE; i++)
    {
        printc_xy(print_column + i, 2 , buffer.buffer[i]);
    }
}
