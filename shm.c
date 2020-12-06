#ifndef SHM_H
#include "shm.h"


void raler(const char* msg, int sys_error)
{
    if (sys_error)
        perror(msg);
    else
        fprintf(stderr, "Erreur : %s\n", msg);
    
    if (fflush(stderr) == EOF)
        raler("fflush", 1);


    //shm_unlink(NOM_COMPTE_RENDU);
    //shm_unlink(NOM_RESTAURANT);

    exit(EXIT_FAILURE);
}


/** Affiche un message de debugage immédiatement.

    Uniquement pour débugger. Affiche sur stdout un message pour
    savoir quand on a passé une étape. 

    \param niveau Le niveau du message de debugage.
    \param msg Le message à afficher.
    \returns Cette fonction ne renvoie rien.
*/
void print_debug(int niveau, char *msg)
{
    char *var_env_ptr;
    int var_env_value;

    if ((var_env_ptr = getenv("DEBUG_REST")) == NULL)
        return;

    if ((var_env_value = atoi(var_env_ptr)) == 0)
        return;

    if(niveau > 1 && niveau > var_env_value)
      return;

    printf("------- %s\n", msg);
    if (fflush(stdout) == EOF)
        raler("fflush", 1);

    return;
}


/** Projection en mémoire du segment de mémoire partagée.

    Le segment de nom #NOM_RESTAURANT est projeté en mémoire, et devient
    accessible en lecture/écriture via le pointeur fourni en valeur de
    retour. Le segment est partagé avec tous les autres processus qui
    le projettent également en mémoire.

    \returns L'adresse du segment existant, projeté en mémoire
*/
struct restaurant *restaurant_map()
{
    struct stat shared_file;
    struct restaurant* m_rest;
    int fd;

    if ((fd = shm_open(NOM_RESTAURANT, O_RDWR , 0666)) == -1)
        raler("ouverture segment restaurant", 1);

    if (fstat(fd, &shared_file) == -1)
        raler("fstat", 1);

    if ((m_rest = mmap(NULL, shared_file.st_size, PROT_READ | PROT_WRITE, 
        MAP_SHARED, fd, 0)) == MAP_FAILED)
        raler("mmap", 1);

    return m_rest;
}


/** Suppression de la projection en mémoire du segment de mémoire partagée.

    Après appel de cette fonction, le segment de mémoire n'est plus
    accessible. Le contenu du segment n'est pas affecté par un appel à
    cette fonction.

    \param restaurant L'adresse du début de la projection 
    \returns Cette fonction ne renvoie rien.
*/
void restaurant_unmap(struct restaurant * restaurant)
{
    if (munmap(restaurant, restaurant -> taille) == -1)
        raler("munmap", 1);
    return;
}


/** Projection en mémoire du segment de mémoire partagée.

    Le segment de nom #NOM_COMPTE_RENDU est projeté en mémoire, et devient
    accessible en lecture/écriture via le pointeur fourni en valeur de
    retour. Le segment est partagé avec tous les autres processus qui
    le projettent également en mémoire.

    \returns L'adresse du segment existant, projeté en mémoire
*/
struct compte_rendu_shm *compte_rendu_map()
{
    struct stat shared_file;
    struct compte_rendu_shm *cr;
    int fd;

    if ((fd = shm_open(NOM_COMPTE_RENDU, O_RDWR , 0666)) == -1)
        raler("ouverture segment compte_rendu_shm", 1);

    if (fstat(fd, &shared_file) == -1)
        raler("fstat", 1);

    if ((cr = mmap(NULL, shared_file.st_size, PROT_READ | PROT_WRITE, 
        MAP_SHARED, fd, 0)) == MAP_FAILED)
        raler("mmap", 1);

    return cr;
}


/** Suppression de la projection en mémoire du segment de mémoire partagée.

    Après appel de cette fonction, le segment de mémoire n'est plus
    accessible. Le contenu du segment n'est pas affecté par un appel à
    cette fonction.

    \param restaurant L'adresse du début de la projection 
    \returns Cette fonction ne renvoie rien.
*/
void compte_rendu_unmap(struct compte_rendu_shm * cr)
{
    if (munmap(cr, cr -> taille) == -1)
        raler("munmap", 1);
    return;
}


#endif