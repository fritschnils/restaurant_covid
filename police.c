#ifndef SHM_H
#include "shm.h"

int main(int argc, char const *argv[])
{
    (void) argv;
    if (argc != 1){
        fprintf (stderr, "usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }
    // FIN TESTS ARGS --------------------------------------------------------




    return EXIT_SUCCESS;
}

#endif

