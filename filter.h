#ifndef _FILTER_H
#define _FILTER_H

#include "database.h"

#define		FILTNAME_LEN	8


struct abook_output_filter {
	char filtname[FILTNAME_LEN];
	char *desc;
	int (*func) (FILE *handle, struct db_enumerator e);
};

struct abook_output_item_filter {
	char filtname[FILTNAME_LEN];
	char *desc;
	void (*func) (FILE *handle, int item);
};

struct abook_input_filter {
	char filtname[FILTNAME_LEN];
	char *desc;
	int (*func) (FILE *handle);
};


int		import_database();
int             import_file(char filtname[FILTNAME_LEN], char *filename);

int		export_database();
int             export_file(char filtname[FILTNAME_LEN], char *filename);

struct abook_output_item_filter
		select_output_item_filter(char filtname[FILTNAME_LEN]);

void		e_write_item(FILE *out, int item, void (*func) (FILE *in, int item));

int		fexport(char filtname[FILTNAME_LEN], FILE *handle,
		int enum_mode);

void		print_filters();

#endif
