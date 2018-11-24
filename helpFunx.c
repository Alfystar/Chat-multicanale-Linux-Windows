//
// Created by alfylinux on 06/07/18.
//

#include "helpFunx.h"
/******************************************************************/
/*                 Funzioni di appoggio e debug                   */
void printErrno (char *note, int error){
	dprintf (STDERR_FILENO, "%s\terr:%d (%s)\n", note, error, strerror (error));
}

void printAllEnv (){
	printf ("\n######################################\n");
	for (char **envp = environ; *envp != 0; envp++){
		char *thisEnv = *envp;
		printf ("%s\n", thisEnv);
	}
	printf ("######################################\n\n");
}

void freeDublePointerArr (void **argv, int size)    //l'array deve terminare NULL
{
	char (**list)[size] = argv;
	for (; *list != NULL; list++){
		free (*list);
	}
	free (*list); //ultimo blocco quello null
	free (argv); //l'inizio della lista manca da essere eliminato
}

void printDublePointeChar (char **list)     //l'array deve terminare NULL
{
	for (; *list != NULL; list++){
		printf ("%s\n", *list);
	}
	printf ("\t# End list #\n");
}

int recursive_delete (const char *dir){
	int ret = 0;
	FTS *ftsp = NULL;
	FTSENT *curr;

	// Cast needed (in C) because fts_open() takes a "char * const *", instead
	// of a "const char * const *", which is only allowed in C++. fts_open()
	// does not modify the argument.
	char *files[] = {(char *)dir, NULL};

	// FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
	//                in multithreaded programs
	// FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
	//                of the specified directory
	// FTS_XDEV     - Don't cross filesystem boundaries
	ftsp = fts_open (files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
	if (!ftsp){
		fprintf (stderr, "%s: fts_open failed: %s\n", dir, strerror (errno));
		ret = -1;
		goto finish;
	}

	while ((curr = fts_read (ftsp))){
		switch (curr->fts_info){
			case FTS_NS:
			case FTS_DNR:
			case FTS_ERR:
				fprintf (stderr, "%s: fts_read error: %s\n", curr->fts_accpath, strerror (curr->fts_errno));
				break;

			case FTS_DC:
			case FTS_DOT:
			case FTS_NSOK:
				// Not reached unless FTS_LOGICAL, FTS_SEEDOT, or FTS_NOSTAT were
				// passed to fts_open()
				break;

			case FTS_D:
				// Do nothing. Need depth-first search, so directories are deleted
				// in FTS_DP
				break;

			case FTS_DP:
			case FTS_F:
			case FTS_SL:
			case FTS_SLNONE:
			case FTS_DEFAULT:
				if (remove (curr->fts_accpath) < 0){
					fprintf (stderr, "%s: Failed to remove: %s\n", curr->fts_path, strerror (errno));
					ret = -1;
				}
				break;
		}
	}

finish:
	if (ftsp){
		fts_close (ftsp);
	}

	return ret;
}