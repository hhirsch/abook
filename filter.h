#ifndef _FILTER_H
#define _FILTER_H

#include "database.h"

#define		FILTNAME_LEN	6

struct abook_filter {
	char filtname[FILTNAME_LEN];
	char *desc;
	int (*func) (FILE *handle);
};


int		import_database();
int             import(char filtname[FILTNAME_LEN], char *filename);

int		export_database();
int             export(char filtname[FILTNAME_LEN], char *filename);
int		fexport(char filtname[FILTNAME_LEN], FILE *handle);

void		print_filters();

#endif
