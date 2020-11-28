#ifndef SHM_H
#include "shm.h"


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

	if ((fd = shm_open(NOM_RESTAURANT, O_RDWR| O_CREAT| O_EXCL, 0666)) == -1)
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
		raler("Init crit_ouvert", 1);

	if (sem_init(&m_restaurant -> serveur_dispo, 1, 0) == -1)
		raler("Init serveur_dispo", 1);

	if (sem_init(&m_restaurant -> requete_convive, 1, 0) == -1)
		raler("Init requete_convive", 1);

	if (sem_init(&m_restaurant -> reponse_serveur, 1, 0) == -1)
		raler("Init reponse_serveur", 1);

	if (sem_init(&m_restaurant -> ack_convive, 1, 0) == -1)
		raler("Init ack_convive", 1);

	if (sem_init(&m_restaurant -> police_presente, 1, 0) == -1)
		raler("Init police_presente", 1);

	if (sem_init(&m_restaurant -> couvre_feu, 1, 0) == -1)
		raler("Init couvre_feu", 1);

	m_restaurant -> req_conv.nom_convive[0] = '\0';	
	m_restaurant -> req_conv.nom_chef[0] = '\0';
	m_restaurant -> req_conv.taille_grp = -1;

	for (int i = 0; i < nb_table; i++)
		m_restaurant -> signal_depart_fin[i] = -1;

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
void destruction_restaurant(struct restaurant *restaurant)
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
		raler("Init crit_ouvert", 1);

	if (sem_destroy(&restaurant -> serveur_dispo) == -1)
		raler("Init serveur_dispo", 1);

	if (sem_destroy(&restaurant -> requete_convive) == -1)
		raler("Init requete_convive", 1);

	if (sem_destroy(&restaurant -> reponse_serveur) == -1)
		raler("Init reponse_serveur", 1);

	if (sem_destroy(&restaurant -> ack_convive) == -1)
		raler("Init ack_convive", 1);

	if (sem_destroy(&restaurant -> police_presente) == -1)
		raler("Init police_presente", 1);

	if (sem_destroy(&restaurant -> couvre_feu) == -1)
		raler("Init couvre_feu", 1);


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


int main(int argc, char *argv[])
{
	int nb_table = 0, sem_test = 1;
	long int duree_service = 0, *tailles_tables;
	char *endptr, *str;
	struct restaurant *m_rest;

	// Check sur nombre d'arguments
	if (argc < 3)
	{
		fprintf (stderr, "usage: %s <duree> <tailles_tables>..\n", argv[0]);
		return EXIT_FAILURE;
	}

	nb_table = argc - 2;



	// Check sur l'argument de durée de service
	str = argv[1];
	errno = 0;
	
	duree_service = strtol(str, &endptr, 10);

	if ((errno == ERANGE && (duree_service == LONG_MAX
			|| duree_service == LONG_MIN)) 
			|| (errno != 0 && duree_service == 0))
	{
		fprintf (stderr, "usage: %s <duree> <tailles_tables>..\n", argv[0]);
		perror("strtol");
		return EXIT_FAILURE;
	}

	// Pas d'entier trouvé || Entier non valide || Mauvaise valeur
	if (endptr == str || *endptr != '\0' || duree_service <= 0)
	{ 
		fprintf (stderr, "usage: %s <duree> <tailles_tables>..\n", argv[0]);
		return EXIT_FAILURE;
	}



	// Check sur tous les arguments de taille des tables
	// Et stockage dans un tableau de long int
	// A CHANGER !!! UTILISET UN LONG INT TEMPORAIRE AU LIEU D'UN TABLEAU !!!
	if((tailles_tables = malloc(nb_table * sizeof(long int))) == NULL)
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
			fprintf(stderr, "usage: %s <duree> <tailles_tables>..\n", argv[0]);
			perror("strtol");
			return EXIT_FAILURE;
		}

		// Pas d'entier trouvé || Entier non valide || Mauvaise valeur
		if (endptr == str || *endptr != '\0' || tailles_tables[i] <= 0 
				|| tailles_tables[i] > 6)
		{
			fprintf(stderr, "usage: %s <duree> <tailles_tables>..\n", argv[0]);
			return EXIT_FAILURE;
		}
	}
	
	print_debug(1, "creation_restaurant");
	m_rest = creation_restaurant(nb_table);
	print_debug(1, "ouverture_restaurant");
	ouverture_fermeture_restaurant(1); //ouverture
	print_debug(1, "attente");


	for(int ji = 0; ji < 10000; ji++)
	{
	/**************************************************************************
	 *						CHECK CONVIVE ATTEND							  *
	 *************************************************************************/
		print_debug(1, "convive ?");
		if (sem_getvalue(&m_rest -> serveur_dispo, &sem_test) == -1)
			raler("sem_getvalue", 1);

		if (sem_test < 1)
		{
			print_debug(1, "UN CONVIVE");

			if (sem_post(&m_rest -> serveur_dispo) == -1)
				raler("sem_post serveur_dispo", 1);
		
			if (sem_wait(&m_rest -> requete_convive) == -1)
				raler("sem_wait requete_convive", 1);
		
			// traiter requete
			if (sem_post(&m_rest -> reponse_serveur) == -1)
				raler("sem_post reponse_serveur", 1);
		
			if (sem_wait(&m_rest -> ack_convive) == -1)
				raler("sem_wait ack_convive", 1);
		
			//remettre req_conv "a zero"
		
		}

	/**************************************************************************
	 *						CHECK CONTROLE POLICE							  *
	 *************************************************************************/
		print_debug(1, "police ?");
		if (sem_getvalue(&m_rest -> police_presente, &sem_test) == -1)
			raler("sem_getvalue", 1);

		if (sem_test > 0)
		{
			print_debug(1, "LA POLICE");
			// CONTROLE POLICE
		}



	/**************************************************************************
	 *						CHECK COUVRE FEU 								  *
	 *************************************************************************/
		print_debug(1, "couvre feu ?");
		if (sem_getvalue(&m_rest -> couvre_feu, &sem_test) == -1)
			raler("sem_getvalue", 1);

		if (sem_test > 0)
		{
			print_debug(1, "COUVRE FEU");
			// ATTENDRE FIN REPAS ET REFOULE
			ouverture_fermeture_restaurant(0); //fermeture
			
			printf("%d convives servis dans %d groupes\n", 0, 0);
			//print_debug(1, "destruction restaurant");
			
			destruction_restaurant(m_rest);
			return EXIT_SUCCESS;
		}
	}//fin for


	//print_debug(1, "resturant détruit");
	return EXIT_SUCCESS;
}

#endif