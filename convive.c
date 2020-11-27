#ifndef SHM_H
#include "shm.h"

int main(int argc, char *argv[])
{
	long int val;
	char *endptr, *str;

	// Check sur nombre et longueurs des arguments
	if (argc != 3 || strlen(argv[1]) > 10 || strlen(argv[1]) < 1 
			|| strlen(argv[2]) > 10 || strlen(argv[2]) < 1)
	{
		fprintf (stderr, "usage: %s <client> <nom_chef ou taille_grp> \n"
					, argv[0]);
		return EXIT_FAILURE;
	}



	//Check de l'argument 2
	if (argv[2][0] >= 48 && argv[2][0] <= 57)
	{
		str = argv[2];
		errno = 0;
		
		val = strtol(str, &endptr, 10);

		if ((errno == ERANGE && (val == LONG_MAX
				|| val == LONG_MIN)) 
				|| (errno != 0 && val == 0))
		{
			fprintf (stderr, "usage: %s <client> <nom_chef ou taille_grp> \n"
					, argv[0]);
			perror("strtol");
			exit(EXIT_FAILURE);
		}

		// Pas d'entier || Entier non valide || Mauvaise valeur
		else if (endptr == str || *endptr != '\0' || val <= 0)
		{ 
			fprintf (stderr, "usage: %s <client> <nom_chef ou taille_grp> \n"
					, argv[0]);
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

#endif