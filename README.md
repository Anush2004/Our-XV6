# Our-XV6

This repository contains the 2 member project developed by Anush and Mukta for the  Operating Systems and Networks(OSN) in our second year (November 2022). It is an extension of the [MIT](https://github.com/mit-pdos)'s [xv6 Operating System for RISC-V](https://github.com/mit-pdos/xv6-riscv).

## Installation

You can follow the install instructions [here](https://pdos.csail.mit.edu/6.S081/2020/tools.html). (Skip the Athena part)


## Running the OS

```sh
$ make clean
$ make qemu SCHEDULER=[RR/PBS/FCFS/LB/MLFQ] CPUS=[N_CPU]
```

default scheduler is RR (Round-robbin).


# Modifications

Here are the modifications made to the original xv6:

## Modification 1: Syscall Tracing

We aim to intercept and record the system calls which are called by a process during its execution. We define a user program called `strace` which uses the system call `trace`.

### User Program

Running:
```sh
$ strace [mask] [command] [args]
```

Implementation:
```c
void strace(int mask, char *command, char *args[]);
```

First, we fork the current process. The parent process waits for the child process. In the child process, before we execute the command using `exec`, we run the `trace` syscall with mask as a parameter.

#### **System Call**

```c
int trace(int mask);
```
1. All syscalls pass through the function `syscall` in `syscall.c`. Hence this would be a very good place to use to implement the strace syscall.
2. There is a variable 'trace_mask' added to the proc struct, which is by default set to 0 in init_proc function. Every time a process is forked, the child inherits its parent's trace_mask.
It records which syscalls must be printed by strace.
3. We then add the `sys_trace` syscall which simply sets the mask to the value of the argument passed to it.
4. Now in `syscall`, we simply check if any of the bits in the mask have been set and if it has been set then we know we have to print it.
5. Now, the syscall itself might modify the arguments passed. Specifically, it might overwrite `a_0`. Hence we save the arguments passed to the trapframe beforehand.
6. Because there is no easy way to know how many arguments each syscall accepts we hardcode them in an array.
7. After the syscall has executed just print the required syscall traces with the saved values.

## ***Modification 2: Alarm***

### System Call

1. ```c
    int sigalarm(int interval,int handler);
    ```



2. ```c
    int sigreturn(void);
    ```

- `sigalarm` will cause the `handler` function to be called after every `interval` ticks of CPU time that the program consumes.
- `sigreturn` resets the process state to before the `handler` was called.
- Both these system calls can be tested by typing the command `alarmtest` in the terminal.

## ***Modification 3: Scheduling***

### **First Come - First Served**

We compare the creation time of each process (which is stored in proc->ctime and initialized to 0 when the process is allocated in the table).

Once we have our pick, ensure that there was at least one process there to schedule. If all is well and we have our pick, context switch to the process, thereby executing it.

Next, disable yield() in trap.c due to timer interrupts. This disables process preemption. This can be done with ifdef preprocessor macros.
  

### **Priority Based Scheduler**

Instead of time, we compare the priorities. Static priority (default to 60) can be changed (explained below) by the user. Dynamic priority is calculated and compared:

```c
dynamic_pr = max(0, min(100, static_pr - niceness + 5))
```

where niceness is defined as
```c
10*(ntime)/(ntime+rtime)
```

here ntime (nap time) and rtime (run time) are the ticks spent in sleeping state since the last call/running state in total, stored in proc->ntime and proc->rtime.

`Tie breaking` is taken care with if else loops.

#### **Set Priority**

User program:
```sh
$ setpriority [priority] [pid]
```

which calls the `set_priority` system call which sets the static priority to the given value and resets ntime to 0 and niceness to 5.

```c
int set_priority(int new_priority, int proc_id);
```

### **Lottery Based Scheduling**

This scheduler assigns a time slice to the process randomly in proportion to the number of tickets it owns.

This proportion is defined as 
```
the probability that the process runs in a given time slice is proportional to the number of tickets owned by it.
```

#### **System call**
```c
int settickets(int number)
```

1. By default, each process is assigned 1 ticket.
2. Calling this system call raises the number of tickets a process can have, thus allotting a higher proportion of CPU cycles for that process.
3. In the proc structure in `proc.h`, we include a new variable tickets to assign the number of tickets allowed to each process so that lottery based scheduling can occur.
4. Whenever the tickets are changed using `settickets()`, the proc structure's tickets variable gets updated and the CPU time is allotted accordingly.

### **Multi Level Feedback Queue** 

In this scheduling, there are 5 queues with decreasing priority and the ticks are assigned for each queue as follows:

```
priority 0: 1 timer tick
priority 1: 2 timer ticks
priority 2: 4 timer ticks
priority 3: 8 timer ticks
priority 4: 16 timer ticks
```

1. When a process is initiated, it is pushed to the end of priority 0's queue.
2. If the process uses the complete timer ticks asigned for its current ppriority queue, it is preempted and inserted at the end of the next lower priority queue.
3. The processes in the highest priority queue will always be run first.
4. The priority 4 queue uses round robin scheduling algorithm.
5. Aging: If the wait time of a process in priority queues 1-4 exceeds 64 ticks, their priority is increased and they are pushed to the next higher priority queue.
6. To implement all this, new variables `inqueue`, `curwtime`, `enteredqueue`, `usedticks`, `queuetime[5]`, `queuepos` are added to the proc structure which keep track of the processes travelling through the queues.
7. The wait time is reset to 0 whenever a process gets selected or if a change in the queue takes place because of aging.
8. **Q: If a process voluntarily relinquishes control of the CPU (eg. For doing I/O), it leaves
the queuing network, and when the process becomes ready again after the I/O, it is
inserted at the tail of the same queue, from which it is relinquished earlier. How can this be exploited by the process?**  
Due to aging conditions, not removing the process from a queue while its waiting for an I/O job to finish keeps on increasing its wait time. At one point, the process will be upgraded to a higher priority queue although its not intended to due to aging. This will not give the other processes in the lower priority which haven't been run at all to age a chance to run, leading to starvation. That's why, when a process is waiting for an I/O job, it leaves the queue and gets added to the end of the queue after the wait is done so that no unnecessary waiting times are added. Without this, even when the process doesn't need the CPU, it keeps on waiting to get allotted CPU time and ages incorrectly.


## ***Modification 4: Copy-on-write fork***

Initially, both the parent and child processes after `fork()` is executed, they share all physical pages, but to map them read-only.

So, when the child or parent executes a store instruction that needs to modify the page, the CPU raises a page-fault exception due to which the kernel maps one copy of the page read/write in the child's address space and another copy of the page read/write in the parent's address space.

After updating the page tables, the kernel resumes the faulting process at the instruction that caused the fault so that the faulting instruction will now execute without fault.

# Performance Comparison of Schedulers
1 CPU
```
SCHEDULER      WTIME      RUNTIME
RR              161         15
FCFS            140         31
LOTTERY         161         15
PBS             131         15
MLFQ            152         15
```
3 CPUs
```
SCHEDULER      WTIME      RUNTIME
RR              112         13
FCFS            37          31
LOTTERY         115         9
```
Notice that even with all the overhead from moving things around in the queue and managing multiple queues at the same time while working on just ONE cpu, MLFQ does considerably well. FCFS has the least wait time, this is mainly because the process is executed as soon as it comes into the queue and the scheduler cannot preemptively execute anything. PBS and Round Robin do reasonably well and post great results as well. The extra picking based on priority in the PBS scheduler seems to have reduced wait time considerably while taking minimum hit in runtime. This is especially good as we reduce the amount of time a process has to idle potentially.


If you have any questions regarding this project, please feel free to contact any of us:

- Anush Anand
  - GitHub: [anush](https://github.com/Anush2004)

- Mukta Chanda
  - GitHub: [mukta](https://github.com/muktachanda)


We will be glad to assist you.




