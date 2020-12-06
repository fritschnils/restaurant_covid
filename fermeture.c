#ifndef SHM_H
#include "shm.h"

int main(int argc, char const *argv[])
{
    struct restaurant *m_rest;

    (void) argv;
    if (argc != 1){
        fprintf (stderr, "usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }
    // FIN TESTS ARGS --------------------------------------------------------
    
    m_rest = restaurant_map();

    if (sem_wait(&m_rest -> crit_ouvert) == -1)
        raler("sem_wait crit_ouvert", 1);

    if (!(m_rest -> ouvert))
    {
        if (sem_post(&m_rest -> crit_ouvert) == -1)
            raler("sem_post crit_ouvert", 1);
        restaurant_unmap(m_rest);
        raler("fermeture alors que restaurant fermé ./fermeture", 0);
    }

    if (sem_post(&m_rest -> crit_ouvert) == -1)
        raler("sem_post crit_ouvert", 1);

    // Annonce couvre feu
    if (sem_post(&m_rest -> couvre_feu) == -1)
        raler("sem_post crit_ouvert", 1);

    restaurant_unmap(m_rest);
    return EXIT_SUCCESS;
}

#endif