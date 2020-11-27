#ifndef SHM_H
#include "shm.h"


void raler(const char* msg, int sys_error)
{
	if (sys_error)
		perror(msg);
	else
		fprintf(stderr, "Erreur : %s\n", msg);
	exit(EXIT_FAILURE);
}

/** Affiche le message passé en paramètre par msg, l'affiche se fait en 
fonction du niveau du message qui est passé en paramètre */
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

	printf("%s\n", msg);
	if(fflush(stdout) == EOF)
		raler("fflush");

	return;
}

#endif