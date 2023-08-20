# multithreading_library
Implementing user level multithreading library

This repository contains code for one-one and many-one model of multithreading library.
Thread functions included in repository listed below:

1)thread_init();
2)thread_create();
3)thread_exit();
4)thread_join();
5)thread_kill();

Every thread has some time to do the work. Time is passed in thread_init() function as an argument.
After the time is over alarm signal (SIGVTALRM) is delievered and context switching is done.
For context switching getcontext,makecontext, setcontext, swapcontext theses system calls are used.

User has to call thread_init() function before using any function of thread library.

How to use this library?

1)Compile thread.cpp and create object file.
```bash
g++ -c thread.cpp
```
This will create thread.o object file.

2)Import threading library into your cpp file in which you want to use this threading library.
```c
#include "thread.h"
```

3)Compile this file and create object file.
For example:
```bash
g++ -c test.cpp
```
This will create test.o file.

4)Linking object files.
```bash
g++ -o try thread.o test.o 
```

5)Run "try" file.
```bash
./try
```
