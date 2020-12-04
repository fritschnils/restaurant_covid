#ifndef SHM_H
#include "shm.h"

int main(int argc, char const *argv[])
{
    int i, j, k;
    struct compte_rendu_shm *cr;
    struct restaurant *restaurant;


    (void) argv; // Pour compiler !

    if (argc != 1){
        fprintf (stderr, "usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }
    // FIN TESTS ARGS --------------------------------------------------------

    restaurant = restaurant_map();

    // Rentre si restaurant ouvert (nomalement, oui)
    if (sem_wait(&restaurant -> crit_ouvert) == -1)
        raler("sem_wait crit_ouvert", 1);

    if (!(restaurant -> ouvert))
    {
        restaurant_unmap(restaurant);
        raler("fermeture alors que restaurant fermé", 0);
    }

    if (sem_post(&restaurant -> crit_ouvert) == -1)
        raler("sem_post crit_ouvert", 1);


    printf("bonjour, controle de police\n");
    // Signale sa présence
    if (sem_post(&restaurant -> police_presente) == -1)
        raler("sem_post police_presente", 1);

    // Attend compte rendu
    if (sem_wait(&restaurant -> compte_rendu_pret) == -1)
        raler("sem_wait compte_rendu_pret", 1);


    // Affiche compte rendu
    cr = compte_rendu_map();

    for (i = 0; i < cr -> nb_table; i++)
    {
        printf("Table %d :", i);
        if (cr -> liste_elements[i].nb_conv != 0)
        {
            for (j = 0; j < cr -> liste_elements[i].nb_conv; j++)
                printf(" %s", cr -> liste_elements[i].noms[j]);
        }
        else
            printf(" (vide)");
        printf("\n");
    }

    printf("\ncahier de rappels :\n");
    k = cr -> nb_table + 1;
    for (i = cr -> nb_table; i < cr -> nb_grp + cr -> nb_table; i++)
    {
        printf("Groupe %d :", k);
        for (j = 0; j < cr -> liste_elements[i].nb_conv; j++)
            printf(" %s", cr -> liste_elements[i].noms[j]);
        printf("\n");
        k--;
    }

    // Prévient qu'il a terminé
    if (sem_post(&cr -> ack_police) == -1)
        raler("sem_post ack_police", 1);


    printf("j'ai bien controlé, je vous laisse...\n");
    restaurant_unmap(restaurant);
    compte_rendu_unmap(cr);
    return EXIT_SUCCESS;
}

#endif

