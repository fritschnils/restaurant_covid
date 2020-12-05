#ifndef SHM_H
#include "shm.h"

void usage()
{
    fprintf(stderr, "usage: convive <client> <nom_chef ou taille_grp> \n");
    
    if(fflush(stderr) == EOF)
        raler("fflush", 1);
}

void ecrit_requete(struct requete_convive *conv, struct requete_convive *dest)
{
    strncpy(dest -> nom_convive, conv -> nom_convive, 11);
    
    if(conv -> taille_grp == -1)
    {
        strncpy(dest -> nom_chef, conv -> nom_chef, 11);
        dest -> taille_grp = -1;    
    }
    else
    {
        dest -> nom_chef[0] = '\0';
        dest -> taille_grp = conv -> taille_grp;
    }
}

int main(int argc, char *argv[])
{
    long int val;
    int rep;
    char *endptr, *str;
    struct restaurant *m_rest;
    struct requete_convive s_conv;

    if (argc != 3 || strlen(argv[1]) > 10 || strlen(argv[1]) < 1 
            || strlen(argv[2]) > 10 || strlen(argv[2]) < 1)
    {
        usage();
        return EXIT_FAILURE;
    }

    if (argv[2][0] >= 48 && argv[2][0] <= 57)
    {
        str = argv[2];
        errno = 0;

        val = strtol(str, &endptr, 10);

        if ((errno == ERANGE && (val == LONG_MAX
                || val == LONG_MIN)) 
                || (errno != 0 && val == 0))
        {
            usage();
            perror("strtol");
            return EXIT_FAILURE;
        }

        else if (endptr == str || *endptr != '\0' || val <= 0)
        {
            usage();
            return EXIT_FAILURE;
        }

        s_conv.nom_chef[0] = '\0';
        s_conv.taille_grp = val;
    }
    else
    {
        strncpy(s_conv.nom_chef, argv[2], 11);
        s_conv.taille_grp = -1; 
    }
    
    strncpy(s_conv.nom_convive, argv[1], 11);



    // FIN TESTS ARGS --------------------------------------------------------

    m_rest = restaurant_map();

    if (sem_wait(&m_rest -> crit_ouvert) == -1)
        raler("sem_wait crit_ouvert", 1);

    if (!(m_rest -> ouvert))
    {
        if (sem_post(&m_rest -> crit_ouvert) == -1)
            raler("sem_post crit_ouvert", 1);
        restaurant_unmap(m_rest);
        return EXIT_SUCCESS;
    }

    if (sem_post(&m_rest -> crit_ouvert) == -1)
        raler("sem_post crit_ouvert", 1);

    // Attend serveur 
    if (sem_wait(&m_rest -> serveur_dispo) == -1)
        raler("sem_wait crit_ouvert", 1);

    // Ecrit requete puis prévient serveur
    ecrit_requete(&s_conv, &m_rest -> req_conv);
    
    if (sem_post(&m_rest -> requete_ecrite) == -1)
        raler("sem_post requete_ecrite", 1);

    // Attend reponse serveur
    if (sem_wait(&m_rest -> reponse_serveur) == -1)
        raler("sem_wait reponse_serveur", 1);

    // Traite reponse : s'installe / repart et envoi un ack
    printf("Bonjour %s vous avez la table %d\n"
            , m_rest -> req_conv.nom_convive, m_rest -> reponse_serv);
    rep = m_rest -> reponse_serv;

    if (sem_post(&m_rest -> ack_convive) == -1)
        raler("sem_post ack_convive", 1);
    
    if (rep == -1)
        return EXIT_SUCCESS;

    // Attend fin du repas
    if (sem_wait(&m_rest -> fin_repas[rep]) == -1)
        raler("sem_wait reponse_serveur", 1);

    restaurant_unmap(m_rest);

    return EXIT_SUCCESS;
}

#endif