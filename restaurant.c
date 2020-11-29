#ifndef SHM_H
#include "shm.h"

void usage()
{
    fprintf(stderr, "usage: restaurant <duree> <tailles_tables>...\n");
    
    if(fflush(stderr) == EOF)
        raler("fflush", 1);
}

/** Création d'un segment de mémoire partagée contenant un restaurant.

    Créer un nouveau segment de mémoire partagée de nom #NOM_RESTAURANT 
    (qui ne doit pas déjà exister).

    Initialise le contenu du segment c'est à dire :
    	- Les variables : taille et ouvert
    	- Les sémaphores
    	- La structure de requête de convive
    	- Le tableau de communication

    \param nb_table Le nombre de tables
    \returns L'adresse du nouveau segment, projeté en mémoire
*/
struct restaurant *creation_restaurant(int nb_table)
{
	int fd;
	struct restaurant *m_restaurant;
	ssize_t octets_rest = SIZE_RESTAURANT(nb_table);

	if ((fd = shm_open(NOM_RESTAURANT
			, O_RDWR| O_CREAT| O_EXCL, 0666)) == -1)
		raler("shm_open", 1);

	if (ftruncate(fd, octets_rest) == -1)
		raler("ftruncate", 1);

	if ((m_restaurant = mmap(NULL, octets_rest, PROT_READ | PROT_WRITE, 
			MAP_SHARED, fd, 0)) == MAP_FAILED)
		raler("mmap", 1);

	// Initialiser variables de la struct restaurant 
	m_restaurant -> ouvert = 0;
	m_restaurant -> taille = octets_rest;
	
	if (sem_init(&m_restaurant -> crit_ouvert, 1, 1) == -1)
		raler("sem_init crit_ouvert", 1);

	if (sem_init(&m_restaurant -> serveur_dispo, 1, 0) == -1)
		raler("sem_init serveur_dispo", 1);

	if (sem_init(&m_restaurant -> requete_ecrite, 1, 0) == -1)
		raler("sem_init requete_ecrite", 1);

	if (sem_init(&m_restaurant -> reponse_serveur, 1, 0) == -1)
		raler("sem_init reponse_serveur", 1);

	if (sem_init(&m_restaurant -> ack_convive, 1, 0) == -1)
		raler("sem_init ack_convive", 1);

	if (sem_init(&m_restaurant -> police_presente, 1, 0) == -1)
		raler("sem_init police_presente", 1);

	if (sem_init(&m_restaurant -> couvre_feu, 1, 0) == -1)
		raler("sem_init couvre_feu", 1);

	m_restaurant -> req_conv.nom_convive[0] = '\0';	
	m_restaurant -> req_conv.nom_chef[0] = '\0';
	m_restaurant -> req_conv.taille_grp = -1;

	for (int i = 0; i < nb_table; i++)
		if (sem_init(&m_restaurant -> signal_depart_fin[i], 1, 0) == -1)
			raler("sem_init signal_depart_fin", 1);

	return m_restaurant;
}

/** Suppression du segment de mémoire partagée.

    Tous les sémaphores contenus dans le segment sont détruits à
    l'aide de sem_destroy(). La projection est ensuite supprimée.
    Enfin, le nom du segment (#NOM_RESTAURANT) est passé à shm_unlink().
    Le segment sera détruit lorsque le dernier processus qui le projette 
    dans son espace d'adressage aura appelé #stock_unmap.
    Normalement, ce sera le processus de restaurant.

    \param restaurant Un pointeur vers le début de la projection.
    \returns Cette fonction ne renvoie rien.
*/
void destruction_restaurant(struct restaurant *restaurant, int nb_table)
{
	if (sem_wait(&restaurant -> crit_ouvert) == -1)
		raler("sem_wait crit_ouvert", 1);

	if (restaurant -> ouvert)
	{
		if (sem_post(&restaurant -> crit_ouvert) == -1)
			raler("sem_wait crit_ouvert", 1);
		
		raler("destruction alors que restaurant ouvert", 0);
	}

	// Destruction des sémaphores
	if (sem_destroy(&restaurant -> crit_ouvert) == -1)
		raler("destroy crit_ouvert", 1);

	if (sem_destroy(&restaurant -> serveur_dispo) == -1)
		raler("destroy serveur_dispo", 1);

	if (sem_destroy(&restaurant -> requete_ecrite) == -1)
		raler("destroy requete_ecrite", 1);

	if (sem_destroy(&restaurant -> reponse_serveur) == -1)
		raler("destroy reponse_serveur", 1);

	if (sem_destroy(&restaurant -> ack_convive) == -1)
		raler("destroy ack_convive", 1);

	if (sem_destroy(&restaurant -> police_presente) == -1)
		raler("destroy police_presente", 1);

	if (sem_destroy(&restaurant -> couvre_feu) == -1)
		raler("destroy couvre_feu", 1);

	for (int i = 0; i < nb_table ; i++)
		if (sem_destroy(&restaurant -> signal_depart_fin[i]) == -1)
			raler("sem_destroy signal_depart_fin", 1);

	if(munmap(restaurant, restaurant -> taille) == -1)
		raler("munmap", 1);

	if(shm_unlink(NOM_RESTAURANT) == -1)
		raler("shm_unlink", 1);
}

/** Ouvre ou ferme le restaurant.

    Cette fonction ouvre ou ferme le restaurant selon le mode.
    Si mode == 1 c'est une ouverture, sinon c'est une fermeture.

    \param mode Mode d'utilisation de la fonction.
    \returns Cette fonction ne renvoie rien.
*/
void ouverture_fermeture_restaurant(int mode)
{
	struct restaurant *restaurant = restaurant_map();

	if (sem_wait(&restaurant -> crit_ouvert) == -1)
		raler("sem_wait critique", 1);

	if ((mode && restaurant -> ouvert) || ((!mode) &&  (!restaurant -> ouvert)))
	{
		if (sem_post(&restaurant -> crit_ouvert) == -1)
			raler("sem_wait crit_ouvert", 1);
		
		raler("ouverture/fermeture mais restaurant est deja ouvert/fermé", 0);
	}

	if (mode)
	{
		restaurant -> ouvert = 1;
		if (sem_post(&restaurant -> serveur_dispo) == -1)
			raler("sem_post serveur_dispo", 1);
	}

	else
		restaurant -> ouvert = 0;

	if (sem_post(&restaurant -> crit_ouvert) == -1)
		raler("sem_wait critique", 1);

	restaurant_unmap(restaurant);
}

void lancer_chrono(struct timespec chrono, int indice, int nb_conv)
{
	sem_t faux_sem;
	struct restaurant *restaurant = restaurant_map();

	if (sem_init(&faux_sem, 0, 0) == -1)
		raler("sem_init faux_sem", 1);

	if (sem_timedwait(&faux_sem, &chrono) == -1)
		if (errno != ETIMEDOUT)
			raler("timedwait", 1);

	for(int i = 0; i < nb_conv; i++)
	{
		if (sem_post(&restaurant -> signal_depart_fin[indice]) == -1)
			raler("sem_post signal_depart_fin", 1);
	}

	if (sem_destroy(&faux_sem) == -1)
		raler("sem_destroy faux_sem", 1);

	restaurant_unmap(restaurant);

	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	int nb_table = 0, test_convive = 0, test_couvrefeu = 0, test_police = 0;
	int tmp, demarrer_table, nb_groupes_servis = 0, nb_convives_servis = 0;
	int taille_tmp;
	long int duree_service = 0, *tailles_tables;
	char *endptr, *str;
	struct restaurant *m_rest;
	struct table *salle;
	struct timespec chrono;

	/**************************************************************************
	 *							CHECK DES ARGUMENTS 						  *
	 *************************************************************************/
	// Check sur nombre d'arguments
	if (argc < 3)
	{
		usage();
		return EXIT_FAILURE;
	}
	nb_table = argc - 2;

	// Durée de service
	str = argv[1];
	errno = 0;
	duree_service = strtol(str, &endptr, 10);

	if ((errno == ERANGE && (duree_service == LONG_MAX
			|| duree_service == LONG_MIN)) 
			|| (errno != 0 && duree_service == 0))
	{
		usage();
		perror("strtol");
		return EXIT_FAILURE;
	}

	// Pas d'entier trouvé || Entier non valide || Mauvaise valeur
	if (endptr == str || *endptr != '\0' || duree_service <= 0)
	{ 
		usage();
		return EXIT_FAILURE;
	}

	// Tailles des tables (puis stockage dans un tableau pour plus tard)
	if ((tailles_tables = malloc(nb_table * sizeof(long int))) == NULL)
		raler("malloc : nb tables", 0);

	for (int i = 0; i < nb_table; i++)
	{
		str = argv[i+2];
		errno = 0;

		tailles_tables[i] = strtol(str, &endptr, 10);	
		if ((errno == ERANGE && (tailles_tables[i] == LONG_MAX
				|| tailles_tables[i] == LONG_MIN)) 
				|| (errno != 0 && tailles_tables[i] == 0))
		{
			usage();
			perror("strtol");
			return EXIT_FAILURE;
		}

		if (endptr == str || *endptr != '\0' || tailles_tables[i] <= 0 
				|| tailles_tables[i] > 6)
		{
			usage();
			return EXIT_FAILURE;
		}
	}


	/**************************************************************************
	 *							INITIALISATION		 						  *
	 *************************************************************************/	

	chrono.tv_sec = 0;
	chrono.tv_nsec = 1000000 * duree_service;

	// Allocation salle
	salle = malloc(nb_table * sizeof(struct table));
	if (salle == NULL)
		raler("malloc tables", 1);

	for (int i = 0; i < nb_table; i++)
	{
		salle[i].taille = tailles_tables[i];
		salle[i].nb_conv = 0;
		salle[i].liste_conv = malloc(salle[i].taille * sizeof(char*));
		if (salle[i].liste_conv == NULL)
			raler("malloc listes noms", 1);
		for (int j = 0; j < salle[i].taille; j++)
		{
			salle[i].liste_conv[j] = malloc(11 * sizeof(char));
			if (salle[i].liste_conv[j]== NULL)
				raler("malloc noms", 1); 
		}
	}

	// Création restaurant puis ouverture
	m_rest = creation_restaurant(nb_table);
	ouverture_fermeture_restaurant(1);




	/**************************************************************************
	 *							DEBUT TRAVAILLE 		 					  *
	 *************************************************************************/
	while (!test_couvrefeu)
	{
		/**********************************************************************
		 *						CHECK CONVIVE ATTEND						  *
		 *********************************************************************/
		print_debug(1, "convive ?");
		if (sem_getvalue(&m_rest -> serveur_dispo, &test_convive) == -1)
			raler("sem_getvalue", 1);

		if (test_convive < 1)
		{
			print_debug(1, "UN CONVIVE");

			if (sem_post(&m_rest -> serveur_dispo) == -1)
				raler("sem_post serveur_dispo", 1);
		
			if (sem_wait(&m_rest -> requete_ecrite) == -1)
				raler("sem_wait requete_ecrite", 1);

			printf("nom convive : %s\nnom chef : %s\ntaille groupe : %d\n"
					, m_rest -> req_conv.nom_convive
					, m_rest -> req_conv.nom_chef
					, m_rest -> req_conv.taille_grp);
			
			tmp = -1;
			taille_tmp = tailles_tables[0];
			demarrer_table = 0;
			// si c'est un chef cherche la table qui va bien
			if (m_rest -> req_conv.taille_grp != -1)
			{
				printf("C'EST LE CHEF\n");
				for (int i = 0; i < nb_table; i++)
				{
					if (tailles_tables[i] >= m_rest -> req_conv.taille_grp
							&& tailles_tables[i] <= taille_tmp
							&& salle[i].nb_conv == 0)
						tmp = i;
					printf("taille table : %ld, taile groupe : %d, nb_conv a la table : %d\n", tailles_tables[i], m_rest -> req_conv.taille_grp, salle[i].nb_conv);

				}

				if (tmp != -1 /*&& tmp <= nb_table*/)
				{
					strncpy(salle[tmp].liste_conv[0]
							, m_rest -> req_conv.nom_convive, 10);
					salle[tmp].liste_conv[0][10] = '\0'; // sécurité
					salle[tmp].nb_conv ++;
					//ECRIRE AUSSI DANS LE CAHIER DE RAPPEL
				}
			}

			// si c'est pas un chef, cherche la table du chef
			else
			{
				printf("C'EST PAS LE CHEF\n");
				for (int i = 0; i < nb_table; i++)
				{
					if (strncmp(salle[i].liste_conv[0]
							, m_rest -> req_conv.nom_convive, 10) == 0)
					{
						if (salle[i].nb_conv == salle[i].taille)
							tmp = -1;
						else
							tmp = i;
						break;
					}
				}

				if (tmp != -1 /*&& tmp <= nb_table*/)
				{

					strncpy(salle[tmp].liste_conv[salle[tmp].nb_conv]
							, m_rest -> req_conv.nom_convive, 10);
					salle[tmp].liste_conv[salle[tmp].nb_conv][10] = '\0';
					salle[tmp].nb_conv ++;
					//ECRIRE AUSSI DANS LE CAHIER DE RAPPEL
				}
			}
			printf("JE RENVOIE %d AU CLIENT\n", tmp);

			// pas de table -> refouler
			if (/*tmp > nb_table || */tmp == -1)
				m_rest -> reponse_serv = -1;
			// sinon -> donner indice de la table
			else
			{
				m_rest -> reponse_serv = tmp; 
				if (salle[tmp].nb_conv == m_rest -> req_conv.taille_grp)
					demarrer_table = 1;
			}

			// donne réponse au convive
			if (sem_post(&m_rest -> reponse_serveur) == -1)
				raler("sem_post reponse_serveur", 1);


			// attend accusé de reception du convive
			if (sem_wait(&m_rest -> ack_convive) == -1)
				raler("sem_wait ack_convive", 1);
			

			if (demarrer_table)
			printf("JE DEMARRE TABLE\n");
			{
 				switch(fork())
 				{
 					case -1 :
 						raler("fork", 1);
 						break;
 					case 0 :
 						lancer_chrono(chrono, tmp, salle[tmp].nb_conv);
 						break;
 				}
 				nb_groupes_servis++;
 				nb_convives_servis += salle[tmp].nb_conv;
 			}		
		}

		/**********************************************************************
		 *						CHECK CONTROLE POLICE						  *
		 *********************************************************************/
		print_debug(1, "police ?");
		if (sem_getvalue(&m_rest -> police_presente, &test_police) == -1)
			raler("sem_getvalue", 1);

		if (test_police > 0)
		{
			print_debug(1, "LA POLICE");
			// CONTROLE POLICE
		}


		/**********************************************************************
		 *						CHECK COUVRE FEU 							  *
		 *********************************************************************/
		print_debug(1, "couvre feu ?");
		if (sem_trywait(&m_rest -> couvre_feu) == -1)
		{
			if(errno != EAGAIN)
				raler("sem_trywait", 1);
		}
		else
		{
			print_debug(1, "COUVRE FEU");
			// ATTENDRE FIN REPAS ET REFOULE
			ouverture_fermeture_restaurant(0); //fermeture
			
			printf("%d convives servis dans %d groupes\n", 0, 0);
			//print_debug(1, "destruction restaurant");
			

			destruction_restaurant(m_rest, nb_table);
			return EXIT_SUCCESS;
		}
	
	}//fin while


	//print_debug(1, "resturant détruit");

	return EXIT_SUCCESS;
}

#endif