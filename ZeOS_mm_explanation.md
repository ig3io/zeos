ZEOS MM EXPLANATION
===================

In file system.c: There are all the initializacions of the memory management system, the interesting functions ranging from line 72 to line 90. In special I focused in init_mm function.

We can found the implementation of init_mm in mm.c file. This function calls another functions that I try to explain:

Init table pages: At first time, initialize the page table of user putting to 0 all entrys, but then (well, really it's in parallell) put the bit presence of the firts PAGE_NUM_KERNELL pages to 1, and asign the same physical adress to this pages.I don't know what it's the meaning of rw bit(read/writte perhaps?). 
mm.c--lines:56 to 73
	
Init frames: This function is simple, initialize all frames of physical memory as if they were avalaible and then put the frames that the Kernel is using not avalaibles. 
mm.c--lines:195-207
