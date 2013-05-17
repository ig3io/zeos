ZeOS
====
SO2 project


Team **JJ01**:

- Ivan Martínez Pérez
- M. Ignacio Contreras Pinilla


Custom ``.bochsrc`` file
------------------------

- It uses SDL (i.e. ``bochs-sdl`` package is installed. Newest versions
  of Ubuntu have some problems with ``bochs-x``).
- vgaromimage is not the default one

Custom `Makefile`
----------------
- `fno-stack-protector` to avoid compilation issues with large `user.c` files.
- Optional `DEBUG_FLAG` (enables debug statements -- great overhead) and `AWESOME_FEATURE` (enables non standard `sbrk` behaviour).

