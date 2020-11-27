#ifndef SHM_H
#include "shm.h"

struct restaurant *creation_restaurant(int nb_tables)
{
	int fd;
	struct restaurant *m_restaurant;
	ssize_t octets_rest = sizeof(struct restaurant) 
							+ nb_tables * sizeof(struct table);

	if ((fd = shm_open(NOM_RESTAURANT, O_RDWR0 O_CREAT| O_EXCL, 0666)) == -1)
		raler("shm_open", 1)

	if (ftruncate(fd, octets_rest) == -1)
		raler("ftruncate", 1);

	if ((m_restaurant = mmap(NULL, octets_rest, PROT_READ | PROT_WRITE, 
			MAP_SHARED, fd, 0)) == MAP_FAILED)
		raler("mmap");

// Initialiser variables de la struct restaurant 
//  			A FAIRE


// Initialiser variables de toutes les struct table 
	// for (int i=0; i<nstation; i++)
	// {
	// 	if (sem_init(&m_restaurant->stations[i].prise_en_charge, 1, 1) == -1)
	// 		raler("Init prise en charge");

	// 	if (sem_init(&m_restaurant->stations[i].depose, 1, 2) == -1)
	// 		raler("Init depose");

	// 	if (sem_init(&m_restaurant->stations[i].taxi_dispo, 1, 0) == -1)
	// 		raler("Init taxi dispo");

	// 	if (sem_init(&m_restaurant->stations[i].client_vide, 1, 0) == -1)
	// 		raler("Init taxi dispo");

	// 	m_restaurant->stations[i].seg_taxi = NULL;

	// 	if (sem_init(&m_restaurant->stations[i].critique_taxi, 1, 1) == -1)
	// 		raler("Init taxi dispo");

	// 	if (sem_init(&m_restaurant->stations[i].critique_client, 1, 1) == -1)
	// 		raler("Init taxi dispo");

	// 	m_restaurant->stations[i].cpt_taxi = 0;
	// 	m_restaurant->stations[i].cpt_client = 0;
	// }

	return m_restaurant;
}

int main(int argc, char *argv[])
{
	int nb_tables = 0;
	long int duree_service = 0, *tailles_tables;
	char *endptr, *str;

	// Check sur nombre d'arguments
	if (argc < 3)
	{
		fprintf (stderr, "usage: %s <duree> <tailles_tables>..\n", argv[0]);
		return EXIT_FAILURE;
	}

	nb_tables = argc - 2;



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
	if((tailles_tables = malloc(nb_tables * sizeof(long int))) == NULL)
		raler("malloc : nb tables", 0);

	for (int i = 0; i < nb_tables; i++)
	{
		str = argv[i+2];
		errno = 0;

		tailles_tables[i] = strtol(str, &endptr, 10);	
		if ((errno == ERANGE && (tailles_tables[i] == LONG_MAX
				|| tailles_tables[i] == LONG_MIN)) 
				|| (errno != 0 && tailles_tables[i] == 0))
		{
			fprintf (stderr, "usage: %s <duree> <tailles_tables>..\n", argv[0]);
			perror("strtol");
			return EXIT_FAILURE;
		}

		// Pas d'entier trouvé || Entier non valide || Mauvaise valeur
		if (endptr == str || *endptr != '\0' || tailles_tables[i] <= 0 
				|| tailles_tables[i] > 6)
		{
			fprintf (stderr, "usage: %s <duree> <tailles_tables>..\n", argv[0]);
			return EXIT_FAILURE;
		}
	}
	

	return EXIT_SUCCESS;
}

#endif