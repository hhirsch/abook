#ifndef _DATABASE_H
#define _DATABASE_H


#define MAX_EMAILS		4
#define MAX_EMAIL_LEN		80
#define MAX_EMAILSTR_LEN	(MAX_EMAILS*MAX_EMAIL_LEN + MAX_EMAILS + 1)
#define MAX_FIELD_LEN		81

enum {
	NAME,
	EMAIL,
	ADDRESS,
	CITY,
	STATE,
	ZIP,
	COUNTRY,
	PHONE,
	WORKPHONE,
	FAX,
	MOBILEPHONE,
	NICK,
	URL,
	NOTES
};

#define LAST_FIELD		NOTES

#define ITEM_FIELDS		(LAST_FIELD+1)

typedef char * list_item[ITEM_FIELDS];

struct abook_field {
	char *name;
	char *key;
	int tab;
};

int		parse_database(FILE *in);
int		write_database(FILE *out);
int		load_database(char *filename);
int		save_database();
void		close_database();
int		add_item2database(list_item item);
void		free_list_item(list_item item);
void		remove_selected_items();
void		sort_surname();
void		sort_database();
char		*get_surname(char *s);
int		find_item(char *str, int start);
int		is_selected(int item);


#define LAST_ITEM	(items - 1)

#define itemcpy(dest, src)	memmove(dest, src, sizeof(list_item))

#define split_emailstr(item, emails) do {\
	int _i,_j,_k,len; \
	memset(&emails, 0, sizeof(emails) ); \
	len = strlen(database[item][EMAIL]); \
	for( _i=0,_j=0, _k=0; _i < len && _j < 4; _i++ ) { \
		if( database[item][EMAIL][_i] ==',' ) { \
			_j++; \
			_k = 0; \
		} else \
			if( _k < MAX_EMAIL_LEN -1 ) \
				emails[_j][_k++] = database[item][EMAIL][_i]; \
	} \
} while(0)

#define have_multiple_emails(item) \
	strchr(database[item][EMAIL], ',')

#endif
