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

#define NOM_RESTAURANT "/leViandard"
#define SIZE_RESTAURANT(n) sizeof(struct restaurant) + (n)*sizeof(int)

struct table {
	int taille;
	int nb_conv;
	int nb_conv_attendus;
	char **liste_conv;
};

struct requete_convive {
	char nom_convive[11];		// = nom du convive
	char nom_chef[11];			// = chef de grp
	int taille_grp;				// = -1 si convive rejoint un groupe
};

struct restaurant {
	int reponse_serv;			// emplacement de la reponse du serveur
	int ouvert;					// =1 si restaurant ouvert, 0 sinon
	size_t taille;				// taille du restaurant en memoire

	struct requete_convive req_conv;	// place pour la requete du convive

	sem_t crit_ouvert;			// init 1 : excl.mut. sur variable ouvert

	sem_t serveur_dispo;		// init 0 : indique que le serveur est dispo
	sem_t requete_ecrite;		// init 0 : indique que la requete est prete 
	sem_t reponse_serveur;		// init 0 : indique que le serveur a repondu
	sem_t ack_convive;			// init 0 : indique que le convive a bien recu
	
	sem_t police_presente;		// init 0 : indique un controle de police
	sem_t couvre_feu;			// init 0 : indique que un couvre feu proche


	sem_t signal_depart_fin[];	// init 0 : indique fin de repas aux tables
};

void raler(const char *, int);
void print_debug(int, char *);

struct restaurant *restaurant_map();
void restaurant_unmap(struct restaurant *);



#endif