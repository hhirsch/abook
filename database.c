
/*
 * $Id$
 *
 * by JH <jheinonen@users.sourceforge.net>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "abook.h"
#include "database.h"
#include "list.h"
#include "misc.h"
#include "options.h"
#include "filter.h"
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

static void	free_item(int i);


list_item *database = NULL;

int items = 0;

#define INITIAL_LIST_CAPACITY	30

int list_capacity = 0;

extern int first_list_item;
extern int curitem;
extern char *selected;

extern char *datafile;
extern char *rcfile;

/*
 * field definitions
 */

#include "edit.h"

/*
 * notes about adding fields:
 * 	- do not change any fields in TAB_CONTACT
 * 	- do not add fields to contact tab
 * 	- 6 fields per tab is maximum
 * 	- reorganize the field numbers in database.h
 */

struct abook_field abook_fields[ITEM_FIELDS] = {
	{"Name",	"name",		TAB_CONTACT},/* NAME */
	{"E-mails",	"email",	TAB_CONTACT},/* EMAIL */
	{"Address",	"address",	TAB_ADDRESS},/* ADDRESS */
	{"City",	"city",		TAB_ADDRESS},/* CITY */
	{"State/Province","state",	TAB_ADDRESS},/* STATE */
	{"ZIP/Postal Code","zip",	TAB_ADDRESS},/* ZIP */
	{"Country",	"country",	TAB_ADDRESS},/* COUNTRY */
	{"Home Phone",	"phone",	TAB_PHONE},/* PHONE */
	{"Work Phone",	"workphone",	TAB_PHONE},/* WORKPHONE */
	{"Fax",		"fax",		TAB_PHONE},/* FAX */
	{"Mobile",	"mobile",	TAB_PHONE},/* MOBILEPHONE */
	{"Nickname/Alias", "nick",      TAB_OTHER},/* NICK */
	{"URL",		"url",		TAB_OTHER},/* URL */
	{"Notes",	"notes",	TAB_OTHER},/* NOTES */
};


int
parse_database(FILE *in)
{
        char *line = NULL;
	char *tmp;
	int sec=0, i;
 	list_item item;

	memset(&item, 0, sizeof(item));
	
	for(;;) {
		line = getaline(in);
		if( feof(in) ) {
			if( item[NAME] && sec )
				add_item2database(item);
			else
				free_list_item(item);
			break;
		}

		if( !*line || *line == '\n' || *line == '#' ) {
			free(line);
			continue;
		} else if( *line == '[' ) {
			if( item[NAME] && sec )
				add_item2database(item);
			else
				free_list_item(item);
			memset(&item, 0, sizeof(item));
			sec = 1;
			if ( !(tmp = strchr(line, ']')))
				sec = 0; /*incorrect section lines are skipped*/
		} else if((tmp = strchr(line, '=') ) && sec ) {
			*tmp++ = '\0';
			for(i=0; i<ITEM_FIELDS; i++)
				if( !strcmp(abook_fields[i].key, line) ) {
					item[i] = strdup(tmp);
					goto next;
				}
		}
next:
		free(line);
	}

	free(line);
	return 0;
}

		

int
load_database(char *filename)
{
	FILE *in;

	if( database != NULL )
		close_database();

	if ( (in = abook_fopen(filename, "r")) == NULL )
		return -1;
	
	parse_database(in);

	if ( items == 0 )
		return 2;

	return 0;
}

int
write_database(FILE *out, struct db_enumerator e)
{
	int j;
	int i = 0;

	fprintf(out,
		"# abook addressbook file\n\n"
		"[format]\n"
		"program=" PACKAGE "\n"
		"version=" VERSION "\n"
		"\n\n"
	);

	db_enumerate_items(e) {
		fprintf(out, "[%d]\n", i);
		for(j = 0; j < ITEM_FIELDS; j++) {
			if( database[e.item][j] != NULL &&
					*database[e.item][j] )
				fprintf(out, "%s=%s\n",
					abook_fields[j].key,
					database[e.item][j]
					);
		}
		fputc('\n', out);
		i++;
	}

	return 0;
}

int
save_database()
{
	FILE *out;
	struct db_enumerator e = init_db_enumerator(ENUM_ALL);

	if( (out = abook_fopen(datafile, "w")) == NULL )
		return -1;

	if( list_is_empty() ) {
		fclose(out);
		unlink(datafile);
		return 1;
	}

	
	write_database(out, e);
	
	fclose(out);
	
	return 0;
}

static void
free_item(int item)
{
	free_list_item(database[item]);
}

void
free_list_item(list_item item)
{
	int i;

	for(i=0; i<ITEM_FIELDS; i++)
		my_free(item[i]);
}

void
close_database()
{
	int i;
	
	for(i=0; i < items; i++)
		free_item(i);

	free(database);
	free(selected);

	database = NULL;
	selected = NULL;

	items = 0;
	first_list_item = curitem = -1;
	list_capacity = 0;
}

#define _MAX_FIELD_LEN(X)	(X == EMAIL ? MAX_EMAILSTR_LEN:MAX_FIELD_LEN)

static void
validate_item(list_item item)
{
	int i;
	char *tmp;
	
	if(item[EMAIL] == NULL)
		item[EMAIL] = strdup("");

	for(i=0; i<ITEM_FIELDS; i++)
		if( item[i] && (strlen(item[i]) > _MAX_FIELD_LEN(i) ) ) {
			tmp = item[i];
			item[i][_MAX_FIELD_LEN(i)-1] = 0;
			item[i] = strdup(item[i]);
			free(tmp);
		}
}


static void
adjust_list_capacity()
{
	if(list_capacity < 1)
		list_capacity = INITIAL_LIST_CAPACITY;
	else if(items >= list_capacity)
		list_capacity *= 2;
	else if(list_capacity / 2 > items)
		list_capacity /= 2;
	else
		return;

	database = abook_realloc(database,
			sizeof(list_item) * list_capacity);
	selected = abook_realloc(selected, list_capacity);
}

int
add_item2database(list_item item)
{
	if( item[NAME] == NULL || ! *item[NAME] ) {
		free_list_item(item);
		return 1;
	}

	if( ++items > list_capacity)
		adjust_list_capacity();

	validate_item(item);

	selected[LAST_ITEM] = 0;
	itemcpy(database[LAST_ITEM], item);

	return 0;
}

void
remove_selected_items()
{
	int i, j;

	if( list_is_empty() )
		return;

	if( ! selected_items() )
		selected[ curitem ] = 1;
	
	for( j = LAST_ITEM; j >= 0; j-- ) {
		if( selected[j] ) {
			free_item(j); /* added for .4 data_s_ */
			for( i = j; i < LAST_ITEM; i++ ) {
				itemcpy(database[ i ], database[ i + 1 ]);
				selected[ i ] = selected[ i + 1 ];
			}
			items--;	
		}
	}

	if( curitem > LAST_ITEM && items > 0 )
		curitem = LAST_ITEM;


	adjust_list_capacity();

	select_none();
}

char *
get_surname(char *s)
{
	int i, a;
	int len = strlen(s);
	char *name = strdup(s);

	for( a = 0, i = len - 1; i >= 0; i--, a++ ) {
		name[a] = s[i];
		if(name[a] == ' ')
			break;
	}

	name[ a ] = 0;

	revstr(name);

	return name;
}

static int
surnamecmp(const void *i1, const void *i2)
{
	int ret;
	list_item a,b;
	char *s1, *s2;

	itemcpy(a, i1);
	itemcpy(b, i2);

	s1 = get_surname(a[NAME]);
	s2 = get_surname(b[NAME]);

	if( !(ret = safe_strcmp(s1, s2)) )
		ret = safe_strcmp(a[NAME], b[NAME]);

	free(s1);
	free(s2);

	return ret;
}

static int
namecmp(const void *i1, const void *i2)
{
	list_item a, b;

	itemcpy(a, i1);
	itemcpy(b, i2);
	
	return safe_strcmp( a[NAME], b[NAME] );
}

void
sort_database()
{
	select_none();
	
	qsort((void *)database, items, sizeof(list_item), namecmp);

	refresh_screen();
}

void
sort_surname()
{
	select_none();

	qsort((void *)database, items, sizeof(list_item), surnamecmp);

	refresh_screen();
}

int
find_item(char *str, int start, int search_fields[])
{
	int i;
	char *findstr = NULL;
	char *tmp = NULL;
	int ret = -1; /* not found */
	struct db_enumerator e = init_db_enumerator(ENUM_ALL);

	if(list_is_empty() || !is_valid_item(start))
		return -2; /* error */

	findstr = strdup(str);
	findstr = strupper(findstr);

	e.item = start - 1; /* must be "real start" - 1 */
	db_enumerate_items(e) {
		for( i = 0; search_fields[i] >= 0; i++ ) {
			tmp = safe_strdup(database[e.item][search_fields[i]]);
			if( tmp && strstr(strupper(tmp), findstr) ) {
				ret = e.item;
				goto out;
			}
			my_free(tmp);
		}
	}

out:
	free(findstr);
	free(tmp);
	return ret;
}


int
is_selected(int item)
{
	return selected[item];
}

int
is_valid_item(int item)
{
	return item <= LAST_ITEM && item >= 0;
}

int
real_db_enumerate_items(struct db_enumerator e)
{
	int item = max(0, e.item + 1);
	int i;
	
	switch(e.mode) {
#ifdef DEBUG
		case ENUM_ALL:
			break;
#endif
		case ENUM_SELECTED:
			for(i = item; i < items; i++) {
				if(is_selected(i)) {
					item = i;
					goto out;
				}
			}
			return -1;
#ifdef DEBUG
		default:
			fprintf(stderr, "real_db_enumerate_items() "
					"BUG: unknown db_enumerator mode: %d\n",
					e.mode);
			break;
#endif
	}
out:
	return (item > LAST_ITEM || item < 0) ? -1 : item;
}

struct db_enumerator
init_db_enumerator(int mode)
{
	struct db_enumerator new;

	new.item = -1; /* important - means "start from beginning" */
	new.mode = mode;

	return new;
}
