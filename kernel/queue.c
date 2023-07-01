#ifdef MLFQ
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "proc.h"
#include "queue.h"

struct proc *queuesched[5][NPROC];
struct qinfo queueInfo;

void init_queue()
{
    queueInfo.max_ticks[0] = 1;
    queueInfo.max_ticks[1] = 2;
    queueInfo.max_ticks[2] = 4;
    queueInfo.max_ticks[3] = 8;
    queueInfo.max_ticks[4] = 16;

    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < NPROC; j++)
        {
            queuesched[i][j] = 0;
        }
        queueInfo.size[i] = 0;
        queueInfo.back[i] = 0;
    }
}

// Pushes pointer to process in proc table into queue.
void push_back(struct proc *p, int q_pos)
{
    if (q_pos < 0 || q_pos >= 5)
        panic("MLFQ: Out of bounds queue position requested");
    if (queueInfo.back[q_pos] >= NPROC)
        panic("MLFQ: Out of static process info memory");
    queuesched[q_pos][queueInfo.back[q_pos]++] = p;
    queueInfo.size[q_pos]++;
    p->queuepos = q_pos;
    p->curwtime = 0;
    p->enteredqueue = ticks;
    p->usedticks = 0;
    p->inqueue = 1;
}

// Pops pointer to process from front of queue.
struct proc *
pop_front(int q_pos)
{
    if (q_pos < 0 || q_pos >= 5)
        panic("MLFQ: Out of bounds queue position requested");
    if (queueInfo.size[q_pos] <= 0)
        panic("MLFQ: Attempt to pop empty queue");

    struct proc *retval = queuesched[q_pos][0];
    queuesched[q_pos][0] = 0;
    queueInfo.back[q_pos]--;
    queueInfo.size[q_pos]--;

    for (int i = 1; i < NPROC; i++)
    {
        queuesched[q_pos][i - 1] = queuesched[q_pos][i];
        if (queuesched[q_pos][i] == 0)
            break;
    }

    retval->inqueue = 0;
    return retval;
}

void removequeue(struct proc *p, int qpos)
{
    int found = -1;
    for (int i = 0; i < NPROC; i++)
        if (queuesched[qpos][i] == p)
            found = i;
    if (found == -1)
        return;

    queuesched[qpos][found] = 0;
    for (int i = found + 1; i < NPROC; i++)
    {
        queuesched[qpos][i - 1] = queuesched[qpos][i];
        if (queuesched[qpos][i] == 0)
            break;
    }
}
#endif
