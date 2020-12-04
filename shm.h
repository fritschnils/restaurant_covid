#ifndef SHM_H
#define SHM_H 1


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>

#define NOM_RESTAURANT "/leViandard"
#define NOM_COMPTE_RENDU "/compteRendu"
#define SIZE_RESTAURANT(n) sizeof(struct restaurant) + (n)*sizeof(int)
#define SIZE_COMPTE_RENDU(x) sizeof(struct compte_rendu_shm) \
                                + (x)*sizeof(struct element_shm);

struct table {
    int taille;
    int nb_conv;
    int nb_conv_attendus;
    char **liste_conv;
};

struct requete_convive {
    char nom_convive[11];       // = nom du convive
    char nom_chef[11];          // = chef de grp
    int taille_grp;             // = -1 si convive rejoint un groupe
};

struct restaurant {
    int reponse_serv;           // emplacement de la reponse du serveur
    int ouvert;                 // =1 si restaurant ouvert, 0 sinon
    size_t taille;              // taille du restaurant en memoire

    struct requete_convive req_conv;    // place pour la requete du convive

    sem_t crit_ouvert;          // init 1 : excl.mut. sur variable ouvert

    sem_t serveur_dispo;        // init 0 : indique que le serveur est dispo
    sem_t requete_ecrite;       // init 0 : indique que la requete est prete 
    sem_t reponse_serveur;      // init 0 : indique que le serveur a repondu
    sem_t ack_convive;          // init 0 : indique que le convive a bien recu
    
    sem_t police_presente;      // init 0 : indique un controle de police
    sem_t compte_rendu_pret;    // init 0 : indique que la police peut lire
    sem_t couvre_feu;           // init 0 : indique que un couvre feu proche

    sem_t fin_repas[];          // init 0 : indique fin de repas aux tables
};



// 2 structures pour stocker le cahier de rappel
struct element {
    int nb_conv;
    char noms[6][11];
    struct element *next;
};

struct cahier_rappel {
    int nb_grp;
    struct element *head;
};


// 2 structures pour le cahier de rappel et l'instantané en mémoire partagée
struct element_shm {
    int type;                   // 1 = instantané table, 0 = groupe servis
    int nb_conv;
    char noms[6][11];
};

struct compte_rendu_shm {
    int nb_grp;
    int nb_table;
    size_t taille;
    sem_t ack_police;           // init 0 : indique que la police a traité
    struct element_shm liste_elements[];
};


void raler(const char *, int);
void print_debug(int, char *);

struct restaurant *restaurant_map();
void restaurant_unmap(struct restaurant *);

struct compte_rendu_shm *compte_rendu_map();
void compte_rendu_unmap(struct compte_rendu_shm *);



#endif
