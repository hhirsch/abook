
/*
 * $Id$
 *
 * by JH <jheinonen@bigfoot.com>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#include "abook_curses.h"
#include "filter.h"
#include "abook.h"
#include "database.h"
#include "edit.h"
#include "list.h"
#include "misc.h"
#include "options.h"

extern int items;
extern list_item *database;
extern struct abook_field abook_fields[];

/*
 * function declarations
 */

/*
 * import filter prototypes
 */

static int      ldif_parse_file(FILE *handle);
static int	mutt_parse_file(FILE *in);
static int	pine_parse_file(FILE *in);

/*
 * export filter prototypes
 */

static int      ldif_export_database(FILE *out, struct db_enumerator e);
static int	html_export_database(FILE *out, struct db_enumerator e);
static int	pine_export_database(FILE *out, struct db_enumerator e);
static int	csv_export_database(FILE *out, struct db_enumerator e);
static int	gcrd_export_database(FILE *out, struct db_enumerator e);
static int	mutt_alias_export(FILE *out, struct db_enumerator e);
static int	elm_alias_export(FILE *out, struct db_enumerator e);
static int	text_export_database(FILE *out, struct db_enumerator e);
static int	spruce_export_database(FILE *out, struct db_enumerator e);

/*
 * end of function declarations
 */

struct abook_input_filter i_filters[] = {
	{ "abook", "abook native format", parse_database },
	{ "ldif", "ldif / Netscape addressbook", ldif_parse_file },
	{ "mutt", "mutt alias (beta)", mutt_parse_file },
	{ "pine", "pine addressbook", pine_parse_file },
	{ "\0", NULL, NULL }
};

struct abook_output_filter e_filters[] = {
	{ "abook", "abook native format", write_database },
	{ "ldif", "ldif / Netscape addressbook (.4ld)", ldif_export_database },
	{ "mutt", "mutt alias", mutt_alias_export },
	{ "html", "html document", html_export_database },
	{ "pine", "pine addressbook", pine_export_database },
	{ "gcrd", "GnomeCard (VCard) addressbook", gcrd_export_database },
	{ "csv", "comma separated values", csv_export_database },
	{ "elm", "elm alias", elm_alias_export },
	{ "text", "plain text", text_export_database },
	{ "spruce", "Spruce address book", spruce_export_database },
	{ "\0", NULL, NULL }
};

/*
 * common functions
 */

void
print_filters()
{
	int i;
	
	puts("input:");
	for(i=0; *i_filters[i].filtname ; i++)
		printf("\t%s\t%s\n", i_filters[i].filtname,
			i_filters[i].desc);

	putchar('\n');
	
	puts("output:");
	for(i=0; *e_filters[i].filtname ; i++)
		printf("\t%s\t%s\n", e_filters[i].filtname,
			e_filters[i].desc);

	putchar('\n');
}

static int
number_of_output_filters()
{
	int i;

	for(i=0; *e_filters[i].filtname ; i++)
		;

	return i;
}

static int
number_of_input_filters()
{
	int i;

	for(i=0; *i_filters[i].filtname ; i++)
		;

	return i;
}

static char *
get_real_name()
{
	char *username = getenv("USER");
	struct passwd *pwent;
	int rtn;
	char *tmp;

	pwent = getpwnam(username);

	if( (tmp = malloc(strlen(pwent->pw_gecos) +1)) == NULL)
		return strdup(username);

	rtn = sscanf(pwent->pw_gecos, "%[^,]", tmp);
	if (rtn == EOF || rtn == 0) {
		free(tmp);
		return strdup(username);
	} else
		return tmp;
}

/*
 * import
 */
	
static int		i_read_file(char *filename, int (*func) (FILE *in));

static void
import_screen()
{
	int i;
	
	clear();

	refresh_statusline();
	headerline("import database");

	mvaddstr(3, 1, "please select a filter");
	

	for(i=0; *i_filters[i].filtname ; i++)
		mvprintw(5 + i, 6, "%c -\t%s\t%s\n", 'a' + i,
			i_filters[i].filtname,
			i_filters[i].desc);

	mvprintw(6 + i, 6, "x -\tcancel");
}

int
import_database()
{
	int filter;
	char *filename;
	int tmp = items;

	import_screen();
	
	filter = getch() - 'a';
	if(filter == 'x' - 'a' ||
		filter >= number_of_input_filters() || filter < 0) {
		refresh_screen();
		return 1;
	}
	
	mvaddstr(5+filter, 2, "->");
	
	filename = ask_filename("Filename: ", 1);
	if( !filename ) {
		refresh_screen();
		return 2;
	}
		
	if(  i_read_file(filename, i_filters[filter].func ) )
		statusline_msg("Error occured while opening the file");
	else
	if( tmp == items )
		statusline_msg("Hmm.., file seems not to be a valid file");
	
	refresh_screen();
	free(filename);

	return 0;
}



static int
i_read_file(char *filename, int (*func) (FILE *in))
{
	FILE *in;
	int ret = 0;

	if( ( in = fopen( filename, "r" ) ) == NULL )
		return 1;

	ret = (*func) (in);

	fclose(in);

	return ret;	
}

int
import(char filtname[FILTNAME_LEN], char *filename)
{
	int i;
	int tmp = items;
	int ret = 0;

	for(i=0;; i++) {
		if( ! strncmp(i_filters[i].filtname, filtname, FILTNAME_LEN) )
			break;
		if( ! *i_filters[i].filtname ) {
			i = -1;
			break;
		}
	}

	if( i<0 )
		return -1;

	if( !strcmp(filename, "-") )
		ret = (*i_filters[i].func) (stdin);
	else
		ret =  i_read_file(filename, i_filters[i].func);
	
	if( tmp == items )
		ret = 1;
	
	return ret;
}

/*
 * export
 */

static int		e_write_file(char *filename,
		int (*func) (FILE *in, struct db_enumerator e), int mode);

static void
export_screen()
{
	int i;
	
	clear();


	refresh_statusline();
	headerline("export database");

	mvaddstr(3, 1, "please select a filter");
	

	for(i=0; *e_filters[i].filtname ; i++)
		mvprintw(5 + i, 6, "%c -\t%s\t%s\n", 'a' + i,
			e_filters[i].filtname,
			e_filters[i].desc);

	mvprintw(6 + i, 6, "x -\tcancel");
}

int
export_database()
{
	int filter;
	int enum_mode = ENUM_ALL;
	char *filename;

	export_screen();
	
	filter = getch() - 'a';
	if(filter == 'x' - 'a' ||
		filter >= number_of_output_filters(e_filters) || filter < 0) {
		refresh_screen();
		return 1;
	}
	
	mvaddstr(5+filter, 2, "->");

	if( selected_items() ) {
		statusline_addstr("Export All/Selected/Cancel (A/s/c)");
		switch( tolower(getch()) ) {
			case 's':
				enum_mode = ENUM_SELECTED;
				break;
			case 'c':
				clear_statusline();
				return 1;
		}
		clear_statusline();
	}
	
	filename = ask_filename("Filename: ", 0);
	if( !filename ) {
		refresh_screen();
		return 2;
	}
	
	if(  e_write_file(filename, e_filters[filter].func, enum_mode ) )
		statusline_msg("Error occured while exporting");
	
	refresh_screen();
	free(filename);

	return 0;
}

static int
e_write_file(char *filename, int (*func) (FILE *in, struct db_enumerator e),
		int mode)
{
	FILE *out;
	int ret = 0;
	struct db_enumerator enumerator = init_db_enumerator(mode);

	if( (out = fopen(filename, "a")) == NULL )
		return 1;

	if( ftell(out) )
		return 1;

	ret = (*func) (out, enumerator);
	
	fclose(out);
	
	return ret;
}

int
fexport(char filtname[FILTNAME_LEN], FILE *handle, int enum_mode)
{
	int i;
	struct db_enumerator e = init_db_enumerator(enum_mode);

	for(i=0;; i++) {
		if( ! strncmp(e_filters[i].filtname, filtname, FILTNAME_LEN) )
			break;
		if( ! *e_filters[i].filtname ) {
			i = -1;
			break;
		}
	}

	return (e_filters[i].func) (handle, e);
}

	

int
export(char filtname[FILTNAME_LEN], char *filename)
{
	const int mode = ENUM_ALL;
	int i;
	int ret = 0;
	struct db_enumerator e = init_db_enumerator(mode);
	
	for(i=0;; i++) {
		if( ! strncmp(e_filters[i].filtname, filtname, FILTNAME_LEN) )
			break;
		if( ! *e_filters[i].filtname ) {
			i = -1;
			break;
		}
	}

	if( i<0 )
		return -1;

	if( !strcmp(filename, "-") )
		ret = (e_filters[i].func) (stdout, e);
	else
		ret =  e_write_file(filename, e_filters[i].func, mode);

	return ret;
}

/*
 * end of common functions
 */

/*
 * ldif import
 */

#include "ldif.h"

static void	ldif_fix_string(char *str);

#ifndef LINESIZE
#	define LINESIZE 1024
#endif

#define	LDIF_ITEM_FIELDS	15

typedef char*  ldif_item[LDIF_ITEM_FIELDS];

static ldif_item ldif_field_names = {
	"cn",	
	"mail",
	"streetaddress",
        "locality",
	"st",
	"postalcode",
	"countryname",
	"homephone",
	"description",
	"homeurl",
	"facsimiletelephonenumber",
	"cellphone",
	"xmozillaanyphone",
	"xmozillanickname",
	"objectclass", /* this must be the last entry */
};

static int ldif_conv_table[LDIF_ITEM_FIELDS] = {
	NAME,		/* "cn" */
	EMAIL,		/* "mail" */
	ADDRESS,	/* "streetaddress" */
        CITY,		/* "locality" */
	STATE,		/* "st" */
	ZIP,		/* "postalcode" */
	COUNTRY,	/* "countryname" */
	PHONE,		/* "homephone" */
	NOTES,		/* "description" */
	URL,		/* "homeurl" */
	FAX,		/* "facsimiletelephonenumber" */
	MOBILEPHONE,	/* "cellphone" */
	WORKPHONE,	/* "xmozillaanyphone" */
	NICK,		/* "xmozillanickname" */
	-1,             /* "objectclass" */ /* this must be the last entry */
};


static char * 
ldif_read_line(FILE *in)
{
	char line[LINESIZE];
	char *buf=NULL;
	char *ptr, *tmp;
	long pos;
	int i;

	for(i=1;;i++) {
		pos = ftell(in);
		fgets(line, LINESIZE, in);
		
		if( feof(in) )
			break;
		
		if(i == 1) {
			buf = strdup(line);
			continue;
		}
		
		if(*line != ' ') {
			fseek(in, pos, SEEK_SET);
			break;
		}

		ptr = (char *)&line;
		while( *ptr == ' ')
			ptr++;

		tmp = buf;
		buf = strconcat(buf, ptr, NULL);
		free(tmp);
	}

	if( *buf == '#' ) {
		free(buf);
		return NULL;
	}
		
	if(buf) {
		int i,j;
		for(i=0,j=0; j < strlen(buf); i++, j++)
			buf[i] = buf[j] == '\n' ? buf[++j] : buf[j];
	}

	return buf;
}

static void
ldif_add_item(ldif_item ldif_item)
{
	list_item abook_item;
	int i;

	memset(abook_item, 0, sizeof(abook_item));
	
	if( !ldif_item[LDIF_ITEM_FIELDS -1] )
		goto bail_out;
	

	for(i=0; i < LDIF_ITEM_FIELDS; i++) {
		if(ldif_conv_table[i] >= 0 && ldif_item[i] && *ldif_item[i] )
			abook_item[ldif_conv_table[i]] = strdup(ldif_item[i]);
	}

	add_item2database(abook_item);

bail_out:
	for(i=0; i < LDIF_ITEM_FIELDS; i++)
		my_free(ldif_item[i]);

}

static void
ldif_convert(ldif_item item, char *type, char *value)
{
	int i;

	if( !strcmp(type, "dn") ) {
		ldif_add_item(item);
		return;
	}

	for(i=0; i < LDIF_ITEM_FIELDS; i++) {
		if( !safe_strcmp(ldif_field_names[i], type) && *value ) {
			if( i == LDIF_ITEM_FIELDS -1) /* this is a dirty hack */
				if( safe_strcmp("person", value))
					break;
			if(item[i])
				my_free(item[i]);
			item[i] = strdup(value);
		}
	}
}

static int
ldif_parse_file(FILE *handle)
{
	char *line = NULL;
	char *type, *value;
	int vlen;
	ldif_item item;

	memset(item, 0, sizeof(item));

	do {
		if( ! (line = ldif_read_line(handle)) )
			continue;

		if( -1 == ( str_parse_line(line, &type, &value, &vlen)) ) {
			my_free(line);
			continue; /* just skip the errors */
		}
				
		ldif_fix_string(value);

		ldif_convert(item, type, value);

		my_free(line);
	} while ( !feof(handle) );

	ldif_convert(item, "dn", "");

	return 0;
}

static void
ldif_fix_string(char *str)
{
	int i, j;

	for( i = 0, j = 0; j < strlen(str); i++, j++)
		str[i] = ( str[j] == (char)0xc3 ?
				(char) str[++j] + (char) 0x40 :
				str[j] );

	str[i] = 0;
}

/*
 * end of ldif import
 */

/*
 * mutt alias import filter
 */

enum {
	MUTT_ALIAS,
	MUTT_NAME,
	MUTT_EMAIL
};

static void
remove_newlines(char *buf)
{
	int i,j;
	
	for(i=0,j=0; j < strlen(buf); i++, j++)
		buf[i] = buf[j] == '\n' ? buf[++j] : buf[j];
}

static int
mutt_read_line(FILE *in, char **alias, char **rest)
{
	char line[LINESIZE];
	char *ptr=(char *)&line;
	char *tmp;

	fgets(line, LINESIZE, in);
	remove_newlines(line);

	while( ISSPACE(*ptr) )
		ptr++;

	if( strncmp("alias", ptr, 5) ) {
		return 1;
	}
		
	ptr += 5;

	while( ISSPACE(*ptr) )
		ptr++;

	tmp = ptr;

	while( ! ISSPACE(*ptr) )
		ptr++;

	if( (*alias = malloc(ptr-tmp+1)) == NULL)
		return 1;

	strncpy(*alias, tmp, ptr-tmp);
	*(*alias+(ptr-tmp)) = 0;

	while( ISSPACE(*ptr) )
		ptr++;

	*rest = strdup(ptr);	
	
	return 0;
}

static void
mutt_parse_email(char *mutt_item[3])
{
	char *tmp;
	int i;

	if( (tmp = strchr(mutt_item[MUTT_NAME], '<')) )
		*tmp = 0;
	else
		return;

	mutt_item[MUTT_EMAIL] = strdup(tmp+1);

	if( (tmp = strchr(mutt_item[MUTT_EMAIL], '>')) )
		*tmp = 0;

	tmp = mutt_item[MUTT_NAME];

	for(i=strlen(tmp)-1; i>0; i--)
		if(ISSPACE(tmp[i]))
			tmp[i] = 0;
		else
			break;

	mutt_item[MUTT_NAME] = strdup(tmp);

	free(tmp);
}

static void
mutt_add_mutt_item(char *mutt_item[3])
{
	list_item abook_item;

	memset(abook_item, 0, sizeof(abook_item));

	abook_item[NAME] = safe_strdup(mutt_item[MUTT_NAME]);
	abook_item[EMAIL] = safe_strdup(mutt_item[MUTT_EMAIL]);
	abook_item[NICK] = safe_strdup(mutt_item[MUTT_ALIAS]);

	add_item2database(abook_item);
}

static int
mutt_parse_file(FILE *in)
{
	char *mutt_item[3];

	for(;;) {

		memset(mutt_item, 0, sizeof(mutt_item) );
		
		if( mutt_read_line(in, &mutt_item[MUTT_ALIAS],
				&mutt_item[MUTT_NAME]) )
			continue;

		mutt_parse_email(mutt_item);

		if( feof(in) ) {
			free(mutt_item[MUTT_ALIAS]);
			free(mutt_item[MUTT_NAME]);
			free(mutt_item[MUTT_EMAIL]);
			break;
		}

		mutt_add_mutt_item(mutt_item);

		free(mutt_item[MUTT_ALIAS]);
		free(mutt_item[MUTT_NAME]);
		free(mutt_item[MUTT_EMAIL]);
	}


	return 0;
}

/*
 * end of mutt alias import filter
 */


/*
 * ldif export filter
 */

static void
ldif_fput_type_and_value(FILE *out,char *type, char *value )
{
	char *tmp;

	tmp = ldif_type_and_value(type, value, strlen(value));

	fputs(tmp, out);

	free(tmp);
}

static int
ldif_export_database(FILE *out, struct db_enumerator e)
{
	char email[MAX_EMAILSTR_LEN];

	fprintf(out, "version: 1\n");

	db_enumerate_items(e) {
		char *tmp;
		int j;
		get_first_email(email, e.item);

		tmp = mkstr("cn=%s,mail=%s", database[e.item][NAME], email);
		ldif_fput_type_and_value(out, "dn", tmp);
		free(tmp);

		for(j=0; j < LDIF_ITEM_FIELDS; j++) {
			if(ldif_conv_table[j] >= 0) {
				if(ldif_conv_table[j] == EMAIL)
					ldif_fput_type_and_value(out,
						ldif_field_names[j], email);
				else if(database[e.item][ldif_conv_table[j]])
					ldif_fput_type_and_value(out,
						ldif_field_names[j],
						database[e.item][ldif_conv_table[j]]);
			}
		}

		fprintf(out, "objectclass: top\n"
				"objectclass: person\n\n");
	}

	return 0;
}

/*
 * end of ldif export filter
 */

/*
 * html export filter
 */

static void            html_export_write_head(FILE *out, int extra_column);
static void            html_export_write_tail(FILE *out);

static int
html_export_database(FILE *out, struct db_enumerator e)
{
	char tmp[MAX_EMAILSTR_LEN];
	int extra_column = options_get_int("extra_column");

	if( items < 1 )
		return 2;

	extra_column = (extra_column > 2 && extra_column < ITEM_FIELDS) ?
		extra_column : PHONE;

	html_export_write_head(out, extra_column);

	db_enumerate_items(e) {
		get_first_email(tmp, e.item);
		fprintf(out, "<tr>\n<td><a href=\"mailto:%s\">%s</a>\n",
				tmp,
				database[e.item][NAME] );
				
		fprintf(out, "<td>%s\n<td>%s\n",
				database[e.item][EMAIL],
				safe_str(database[e.item][extra_column]) );
		fprintf(out, "</tr>\n\n");
	}

	html_export_write_tail(out);

	return 0;
}


static void
html_export_write_head(FILE *out, int extra_column)
{
	char *realname = get_real_name();

	fprintf(out, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n");
	fprintf(out, "<html>\n<head>\n	<title>%s's addressbook</title>",
			realname );
	fprintf(out, "\n</head>\n<body>\n");
	fprintf(out, "\n<h2>%s's addressbook</h2>\n", realname );
	fprintf(out, "<br><br>\n\n");

	fprintf(out, "<center><table border>\n");
	fprintf(out, "\n<tr><th>Name<th>E-mail address(es)<th>%s</tr>\n\n",
			abook_fields[extra_column].name);

	free(realname);
}

static void
html_export_write_tail(FILE *out)
{
	fprintf(out, "\n</table></center>\n");
	fprintf(out, "\n</body>\n</html>\n");
}
	
/*
 * end of html export filter
 */


/*
 * pine addressbook import filter
 */

static void
pine_fixbuf(char *buf)
{
	int i,j;

	for(i=0,j=0; j < strlen(buf); i++, j++)
		buf[i] = buf[j] == '\n' ? buf[++j] : buf[j];
}

static void
pine_convert_emails(char *s)
{
	int i;
	char *tmp;

	if( s == NULL || *s != '(' )
		return;

	for(i=0; s[i]; i++ )
		s[i] = s[i+1];

	if( ( tmp = strchr(s,')')) )
		*tmp=0;
	
	for(i=1; ( tmp = strchr(s, ',') ) != NULL ; i++, s=tmp+1 )
		if( i > 3 ) {
			*tmp = 0;
			break;	
		}

}

static void
pine_parse_buf(char *buf)
{
	list_item item;
	char *start = buf;
	char *end;
	char tmp[400];
	int i, len, last;
	int pine_conv_table[]= {NICK, NAME, EMAIL, -1, NOTES};

	memset(&item, 0, sizeof(item) );
	
	for(i=0, last=0; !last ; i++) {
		if( ! (end = strchr(start, '\t')) )
			last=1;
		
		len = last ? strlen(start) : (int) (end-start);
		len = min(len, 400-1);
	
		if(i < sizeof(pine_conv_table) / sizeof(*pine_conv_table)
				&& pine_conv_table[i] >= 0) {
			strncpy(tmp, start, len);
			tmp[len] = 0;
			item[pine_conv_table[i]] = strdup(tmp);
		}
		start = end + 1;
	}
	
	pine_convert_emails(item[EMAIL]);
	add_item2database(item);
}
		

#define LINESIZE	1024

static int
pine_parse_file(FILE *in)
{
	char line[LINESIZE];
	char *buf=NULL;
	char *ptr;
	int i;

	fgets(line, LINESIZE, in);	
	
	while(!feof(in)) {
		for(i=2;;i++) {
			buf = realloc(buf, i*LINESIZE);
			if(i==2)
				strcpy(buf, line);
			fgets(line, LINESIZE, in);
			ptr=(char *)&line;
			if(*ptr != ' ' || feof(in) )
				break;
			else
				while( *ptr == ' ') ptr++;
				
			strcat(buf, ptr);
		}
		if( *buf == '#' )
			continue;
		pine_fixbuf(buf);

		pine_parse_buf(buf);

		my_free(buf);
	}

	return 0;
}

/*
 * end of pine addressbook import filter
 */


/*
 * pine addressbook export filter
 *
 *  filter doesn't wrap the lines as it should but Pine seems to handle
 *  created files without problems - JH
 */

static int
pine_export_database(FILE *out, struct db_enumerator e)
{
	db_enumerate_items(e) {
		fprintf(out, have_multiple_emails(e.item) ?
				"%s\t%s\t(%s)\t\t%s\n" : "%s\t%s\t%s\t\t%s\n",
				safe_str(database[e.item][NICK]),
				safe_str(database[e.item][NAME]),
				safe_str(database[e.item][EMAIL]),
				safe_str(database[e.item][NOTES])
				);
	}

	return 0;
}

/*
 * end of pine addressbook export filter
 */


/*
 * csv addressbook export filter
 */

static int
csv_export_database(FILE *out, struct db_enumerator e)
{
	int j;
	int csv_export_fields[] = {
		NAME,
		EMAIL,
		PHONE,
		NOTES,
		-1
	};

	db_enumerate_items(e) {
		for(j = 0; csv_export_fields[j] >= 0; j++) {
			fprintf(out, strchr(safe_str(database[e.item][csv_export_fields[j]]), ',') ?
				"\"%s\"" : "%s",
				safe_str(database[e.item][csv_export_fields[j]])
				);
			if(csv_export_fields[j+1] >= 0)
				fputc(',', out);
		}
		fputc('\n', out);
	}
		


	return 0;
}

/*
 * end of csv export filter
 */


/*
 * GnomeCard (VCard) addressbook export filter
 */

static int
gcrd_export_database(FILE *out, struct db_enumerator e)
{
	char emails[MAX_EMAILS][MAX_EMAIL_LEN];
	int j;
	char *name;

	db_enumerate_items(e) {
		fprintf(out, "BEGIN:VCARD\nFN:%s\n",
				safe_str(database[e.item][NAME]));

		name = get_surname(database[e.item][NAME]);
	        for( j = strlen(database[e.item][NAME]) - 1; j >= 0; j-- ) {
	                if(database[e.item][NAME][j] == ' ')
	                        break;
	        } 
		fprintf(out, "N:%s;%.*s\n",
			safe_str(name),
			j,
			safe_str(database[e.item][NAME])
			); 

		free(name);

		if ( database[e.item][ADDRESS] )
			fprintf(out, "ADR:;;%s;%s;%s;%s;%s\n",
				safe_str(database[e.item][ADDRESS]),
				safe_str(database[e.item][CITY]),
				safe_str(database[e.item][STATE]),
				safe_str(database[e.item][ZIP]),
				safe_str(database[e.item][COUNTRY])
				);
		
		if (database[e.item][PHONE])
			fprintf(out, "TEL;HOME:%s\n", database[e.item][PHONE]);
		if (database[e.item][WORKPHONE])
			fprintf(out, "TEL;WORK:%s\n", database[e.item][WORKPHONE]);
		if (database[e.item][FAX])
			fprintf(out, "TEL;FAX:%s\n", database[e.item][FAX]);
		if (database[e.item][MOBILEPHONE])
			fprintf(out, "TEL;CELL:%s\n", database[e.item][MOBILEPHONE]);

		if ( database[e.item][EMAIL] ) {
			split_emailstr(e.item, emails);
			for(j=0; j < MAX_EMAILS ; j++) {
				if ( *emails[j] ) 
					fprintf(out, "EMAIL;INTERNET:%s\n",
						emails[j]);
			}
		}
		
		if ( database[e.item][NOTES] ) 
			fprintf(out, "NOTE:%s\n", database[e.item][NOTES]);
		if (database[e.item][URL])
			fprintf(out, "URL:%s\n",  database[e.item][URL]);

		fprintf(out, "END:VCARD\n\n");
		
	}

	return 0;
}

/*
 * end of GnomeCard export filter
 */


/*
 * mutt alias export filter
 */

static char *
mutt_alias_genalias(int i)
{
	char *tmp, *pos;
	
	if(database[i][NICK])
		return strdup(database[i][NICK]);

	tmp = strdup(database[i][NAME]);

	if( ( pos = strchr(tmp, ' ') ) )
		*pos = 0;

	strlower(tmp);

	return tmp;	
}

static int
mutt_alias_export(FILE *out, struct db_enumerator e)
{
	char email[MAX_EMAIL_LEN];
	char *alias = NULL;

	db_enumerate_items(e) {
		alias = mutt_alias_genalias(e.item);

		get_first_email(email, e.item);
		fprintf(out, *email ? "alias %s %s <%s>\n": "alias %s %s%s\n",
				alias,
				database[e.item][NAME],
				email);
		my_free(alias);
	}

	return 0;
}

/*
 * end of mutt alias export filter
 */


/*
 * printable export filter
 */


static void
text_write_address_us(FILE *out, int i) {
	fprintf(out, "\n%s", database[i][ADDRESS]);
	
	if (database[i][CITY])
		fprintf(out, "\n%s", database[i][CITY]);
		
	if (database[i][STATE] || database[i][ZIP]) {
		fputc('\n', out);
		
		if(database[i][STATE]) {
			fprintf(out, "%s", database[i][STATE]);
			if(database[i][ZIP])
				fputc(' ', out);
		}

		if(database[i][ZIP])
			fprintf(out, "%s", database[i][ZIP]);
	}

	if (database[i][COUNTRY])
		fprintf(out, "\n%s", database[i][COUNTRY]);
}


static void
text_write_address_uk(FILE *out, int i) {
	int j;

	for (j = ADDRESS; j <= COUNTRY; j++)
		if (database[i][j])
			fprintf(out, "\n%s", database[i][j]);
}

static void
text_write_address_eu(FILE *out, int i) {
	fprintf(out, "\n%s", database[i][ADDRESS]);
	
	if (database[i][ZIP] || database[i][CITY]) {
		fputc('\n', out);

		if(database[i][ZIP]) {
			fprintf(out, "%s", database[i][ZIP]);
			if(database[i][CITY])
				fputc(' ', out);
		}

		if(database[i][CITY])
			fprintf(out, "%s", database[i][CITY]);
		/*
		fprintf(out, "\n%s %s",
				safe_str(database[i][ZIP]),
				safe_str(database[i] [CITY]));
				*/
	}
	

	if (database[i][STATE])
		fprintf(out, "\n%s", database[i][STATE]);

	if (database[i][COUNTRY])
		fprintf(out, "\n%s", database[i][COUNTRY]);
}

static int
text_export_database(FILE * out, struct db_enumerator e)
{
	char emails[MAX_EMAILS][MAX_EMAIL_LEN];
	int j;
	char *realname = get_real_name();
	char *style = options_get_str("address_style");

	fprintf(out,
		"-----------------------------------------\n%s's address book\n"
		"-----------------------------------------\n\n\n",
		realname);
	free(realname);

	db_enumerate_items(e) {
		fprintf(out,
			"-----------------------------------------\n\n");
		fprintf(out, "%s", database[e.item][NAME]);
		if (database[e.item][NICK])
			fprintf(out, "\n(%s)", database[e.item][NICK]);
		fprintf(out, "\n");

		if (*database[e.item][EMAIL]) {
			fprintf(out, "\n");
			split_emailstr(e.item, emails);
			for (j = 0; j < MAX_EMAILS; j++)
				if (*emails[j])
					fprintf(out, "%s\n", emails[j]);
		}
		/* Print address */
		if (database[e.item][ADDRESS]) {
			if (!safe_strcmp(style, "us"))	/* US like */
				text_write_address_us(out, e.item);
			else if (!safe_strcmp(style, "uk"))	/* UK like */
				text_write_address_uk(out, e.item);
			else	/* EU like */
				text_write_address_eu(out, e.item);

			fprintf(out, "\n");
		}

		if ((database[e.item][PHONE]) ||
			(database[e.item][WORKPHONE]) ||
			(database[e.item][FAX]) ||
			(database[e.item][MOBILEPHONE])) {
			fprintf(out, "\n");
			for (j = PHONE; j <= MOBILEPHONE; j++)
				if (database[e.item][j])
					fprintf(out, "%s: %s\n",
						abook_fields[j].name,
						database[e.item][j]);
		}

		if (database[e.item][URL])
			fprintf(out, "\n%s\n", database[e.item][URL]);
		if (database[e.item][NOTES])
			fprintf(out, "\n%s\n", database[e.item][NOTES]);

		fprintf(out, "\n");
	}

	fprintf(out, "-----------------------------------------\n");

	return 0;
}

/*
 * end of printable export filter
 */

/*
 * elm alias export filter
 */

static int
elm_alias_export(FILE *out, struct db_enumerator e)
{
	char email[MAX_EMAIL_LEN];
	char *alias = NULL;

	db_enumerate_items(e) {
		alias = mutt_alias_genalias(e.item);
		get_first_email(email, e.item);
		fprintf(out, "%s = %s = %s\n",
				alias,
				database[e.item][NAME],
				email);
		my_free(alias);
	}

	return 0;
}

/*
 * end of elm alias export filter
 */


/*
 * Spruce export filter
 */

static int
spruce_export_database (FILE *out, struct db_enumerator e)
{
	char email[MAX_EMAIL_LEN];

	fprintf (out, "# This is a generated file made by abook for the Spruce e-mail client.\n\n");

	db_enumerate_items(e) {
		if(strcmp (safe_str(database[e.item][EMAIL]), "")) {
			get_first_email(email, e.item);
			fprintf(out, "# Address %d\nName: %s\nEmail: %s\nMemo: %s\n\n",
					e.item,
					database[e.item][NAME],
					email,
					safe_str(database[e.item][NOTES])
					);
		}
	}

	fprintf (out, "# End of address book file.\n");

	return 0;
}

/*
 * end of Spruce export filter
 */

