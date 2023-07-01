#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(2, "Priority: lesser arguments entered\n");
        exit(1);
    }
    prior(atoi(argv[1]), atoi(argv[2]));
    exit(0);
}