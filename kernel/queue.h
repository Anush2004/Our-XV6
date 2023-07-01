#include "param.h"

struct qinfo {
    int size[5];
    int back[5];
    int max_ticks[5];
};

extern struct qinfo queueInfo;
extern struct proc *queuesched[5][NPROC];