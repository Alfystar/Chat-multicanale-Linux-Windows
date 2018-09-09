//
// Created by alfylinux on 06/07/18.
//

#include "helpFunx.h"
/******************************************************************/
/*                 Funzioni di appoggio e debug                   */
void printErrno(char *note, int error) {
	dprintf(2, "%s\terr:%d (%s)\n", note, error, strerror(error));
}

void printAllEnv() {
	printf("\n######################################\n");
	for (char **envp = environ; *envp != 0; envp++) {
		char *thisEnv = *envp;
		printf("%s\n", thisEnv);
	}
	printf("######################################\n\n");
}

void freeDublePointerArr(void **argv, int size)    //l'array deve terminare NULL
{
	char (**list)[size] = argv;
	for (; *list != NULL; list++) {
		free(*list);
	}
	free(*list); //ultimo blocco quello null
	free(argv); //l'inizio della lista manca da essere eliminato
}

void printDublePointeChar(char **list)     //l'array deve terminare NULL
{
	for (; *list != NULL; list++) {
		printf("%s\n", *list);
	}
	printf("\t# End list #\n");
}