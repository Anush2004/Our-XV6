#include "user/user.h"

void strace(int mask, char* command, char** args)
{
    int pid = fork();

    if(pid == 0)
    {
        trace(mask);
        exec(command, args);
        exit(0);
    }
    else 
    {
        wait(0);
    }
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        fprintf(2, "strace: did not enter all the arguments needed...\n");
        exit(1);
    }
    else 
    {
        strace(atoi(argv[1]), argv[2], argv + 2);
        exit(0);
    }
}