
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
#include <ctype.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "abook_curses.h"
#include "filter.h"
#include "abook.h"
#include "database.h"
#include "edit.h"
#include "gettext.h"
#include "list.h"
#include "misc.h"
#include "options.h"
#include "xmalloc.h"
#include <assert.h>

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
static int 	csv_parse_file(FILE *in);
static int 	allcsv_parse_file(FILE *in);
static int 	palmcsv_parse_file(FILE *in);

/*
 * export filter prototypes
 */

static int      ldif_export_database(FILE *out, struct db_enumerator e);
static int	html_export_database(FILE *out, struct db_enumerator e);
static int	pine_export_database(FILE *out, struct db_enumerator e);
static int	csv_export_database(FILE *out, struct db_enumerator e);
static int	allcsv_export_database(FILE *out, struct db_enumerator e);
static int	palm_export_database(FILE *out, struct db_enumerator e);
static int	gcrd_export_database(FILE *out, struct db_enumerator e);
static int	mutt_alias_export(FILE *out, struct db_enumerator e);
static int	elm_alias_export(FILE *out, struct db_enumerator e);
static int	text_export_database(FILE *out, struct db_enumerator e);
static int	spruce_export_database(FILE *out, struct db_enumerator e);

/*
 * end of function declarations
 */

struct abook_input_filter i_filters[] = {
	{ "abook", N_("abook native format"), parse_database },
	{ "ldif", N_("ldif / Netscape addressbook"), ldif_parse_file },
	{ "mutt", N_("mutt alias"), mutt_parse_file },
	{ "pine", N_("pine addressbook"), pine_parse_file },
	{ "csv", N_("comma separated values"), csv_parse_file },
	{ "allcsv", N_("comma separated values (all fields)"), allcsv_parse_file },
	{ "palmcsv", N_("Palm comma separated values"), palmcsv_parse_file },
	{ "\0", NULL, NULL }
};

struct abook_output_filter e_filters[] = {
	{ "abook", N_("abook native format"), write_database },
	{ "ldif", N_("ldif / Netscape addressbook (.4ld)"), ldif_export_database },
	{ "mutt", N_("mutt alias"), mutt_alias_export },
	{ "html", N_("html document"), html_export_database },
	{ "pine", N_("pine addressbook"), pine_export_database },
	{ "gcrd", N_("GnomeCard (VCard) addressbook"), gcrd_export_database },
	{ "csv", N_("comma separated values"), csv_export_database },
	{ "allcsv", N_("comma separated all values"), allcsv_export_database },
	{ "palmcsv", N_("Palm comma separated values"), palm_export_database},
	{ "elm", N_("elm alias"), elm_alias_export },
	{ "text", N_("plain text"), text_export_database },
	{ "spruce", N_("Spruce address book"), spruce_export_database },
	{ "\0", NULL, NULL }
};

/*
 * common functions
 */

void
print_filters()
{
	int i;

	puts(_("input:"));
	for(i=0; *i_filters[i].filtname ; i++)
		printf("\t%s\t%s\n", i_filters[i].filtname,
			gettext(i_filters[i].desc));

	putchar('\n');

	puts(_("output:"));
	for(i=0; *e_filters[i].filtname ; i++)
		printf("\t%s\t%s\n", e_filters[i].filtname,
			gettext(e_filters[i].desc));

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

	if((tmp = xstrdup(pwent->pw_gecos)) == NULL)
		return xstrdup(username);

	rtn = sscanf(pwent->pw_gecos, "%[^,]", tmp);
	if (rtn == EOF || rtn == 0) {
		free(tmp);
		return xstrdup(username);
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
	headerline(_("import database"));

	mvaddstr(3, 1, _("please select a filter"));


	for(i=0; *i_filters[i].filtname ; i++)
		mvprintw(5 + i, 6, "%c -\t%s\t%s\n", 'a' + i,
			i_filters[i].filtname,
			gettext(i_filters[i].desc));

	mvprintw(6 + i, 6, _("x -\tcancel"));
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

	filename = ask_filename(_("Filename: "));
	if(!filename) {
		refresh_screen();
		return 2;
	}

	if(i_read_file(filename, i_filters[filter].func ))
		statusline_msg(_("Error occured while opening the file"));
	else if(tmp == items)
		statusline_msg(_("File does not seem to be a valid addressbook"));

	refresh_screen();
	free(filename);

	return 0;
}



static int
i_read_file(char *filename, int (*func) (FILE *in))
{
	FILE *in;
	int ret = 0;

	if( (in = abook_fopen( filename, "r" )) == NULL )
		return 1;

	ret = (*func) (in);

	fclose(in);

	return ret;
}

int
import_file(char filtname[FILTNAME_LEN], char *filename)
{
	int i;
	int tmp = items;
	int ret = 0;

	for(i=0;; i++) {
		if(! strncasecmp(i_filters[i].filtname, filtname,
					FILTNAME_LEN) )
			break;
		if(! *i_filters[i].filtname) {
			i = -1;
			break;
		}
	}

	if(i < 0)
		return -1;

	if(!strcmp(filename, "-")) {
		struct stat s;
		if((fstat(fileno(stdin), &s)) == -1 || S_ISDIR(s.st_mode))
			ret = 1;
		else
			ret = (*i_filters[i].func) (stdin);
	} else
		ret =  i_read_file(filename, i_filters[i].func);

	if(tmp == items)
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
	headerline(_("export database"));

	mvaddstr(3, 1, _("please select a filter"));


	for(i = 0; *e_filters[i].filtname ; i++)
		mvprintw(5 + i, 6, "%c -\t%s\t%s\n", 'a' + i,
			e_filters[i].filtname,
			gettext(e_filters[i].desc));

	mvprintw(6 + i, 6, _("x -\tcancel"));
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
		filter >= number_of_output_filters() || filter < 0) {
		refresh_screen();
		return 1;
	}

	mvaddstr(5+filter, 2, "->");

	if(selected_items()) {
		/* TODO gettext: handle translated keypresses? */
		statusline_addstr(_("Export All/Selected/Cancel (A/s/c)?"));
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

	filename = ask_filename(_("Filename: "));
	if(!filename) {
		refresh_screen();
		return 2;
	}

	if( e_write_file(filename, e_filters[filter].func, enum_mode))
		statusline_msg(_("Error occured while exporting"));

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

	if((out = fopen(filename, "a")) == NULL)
		return 1;

	if(ftell(out))
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
		if(!strncasecmp(e_filters[i].filtname, filtname,
					FILTNAME_LEN))
			break;
		if(!*e_filters[i].filtname) {
			i = -1;
			break;
		}
	}

	return (e_filters[i].func) (handle, e);
}



int
export_file(char filtname[FILTNAME_LEN], char *filename)
{
	const int mode = ENUM_ALL;
	int i;
	int ret = 0;
	struct db_enumerator e = init_db_enumerator(mode);

	for(i=0;; i++) {
		if(!strncasecmp(e_filters[i].filtname, filtname,
					FILTNAME_LEN))
			break;
		if(!*e_filters[i].filtname) {
			i = -1;
			break;
		}
	}

	if(i < 0)
		return -1;

	if(!strcmp(filename, "-"))
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

#define	LDIF_ITEM_FIELDS	16

typedef char *ldif_item[LDIF_ITEM_FIELDS];

static ldif_item ldif_field_names = {
	"cn",
	"mail",
	"streetaddress",
	"streetaddress2",
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
	ADDRESS2,	/* "streetaddress2" */
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
	char *buf = NULL;
	char *ptr, *tmp;
	long pos;
	int i;

	for(i = 1;;i++) {
		char *line;

		pos = ftell(in);
		line = getaline(in);

		if(feof(in) || !line)
			break;

		if(i == 1) {
			buf = line;
			continue;
		}

		if(*line != ' ') {
			fseek(in, pos, SEEK_SET); /* fixme ! */
			free(line);
			break;
		}

		ptr = line;
		while( *ptr == ' ')
			ptr++;

		tmp = buf;
		buf = strconcat(buf, ptr, NULL);
		free(tmp);
		free(line);
	}

	if(buf && *buf == '#' ) {
		free(buf);
		return NULL;
	}

	return buf;
}

static void
ldif_add_item(ldif_item li)
{
	list_item abook_item;
	int i;

	memset(abook_item, 0, sizeof(abook_item));

	if(!li[LDIF_ITEM_FIELDS -1])
		goto bail_out;


	for(i=0; i < LDIF_ITEM_FIELDS; i++) {
		if(ldif_conv_table[i] >= 0 && li[i] && *li[i])
			abook_item[ldif_conv_table[i]] = xstrdup(li[i]);
	}

	add_item2database(abook_item);

bail_out:
	for(i=0; i < LDIF_ITEM_FIELDS; i++)
		xfree(li[i]);

}

static void
ldif_convert(ldif_item item, char *type, char *value)
{
	int i;

	if(!strcmp(type, "dn")) {
		ldif_add_item(item);
		return;
	}

	for(i=0; i < LDIF_ITEM_FIELDS; i++) {
		if(!safe_strcmp(ldif_field_names[i], type) && *value) {
			if(i == LDIF_ITEM_FIELDS - 1) /* this is a dirty hack */
				if(safe_strcmp("person", value))
					break;
			if(item[i])
				xfree(item[i]);
			item[i] = xstrdup(value);
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
		if( !(line = ldif_read_line(handle)) )
			continue;

		if(-1 == (str_parse_line(line, &type, &value, &vlen))) {
			xfree(line);
			continue; /* just skip the errors */
		}

		ldif_fix_string(value);

		ldif_convert(item, type, value);

		xfree(line);
	} while ( !feof(handle) );

	ldif_convert(item, "dn", "");

	return 0;
}

static void
ldif_fix_string(char *str)
{
	int i, j;

	for(i = 0, j = 0; j < (int)strlen(str); i++, j++)
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

#include "getname.h"

static int
mutt_read_line(FILE *in, char **alias, char **rest)
{
	char *line, *ptr, *tmp;
	size_t alias_len;

	if( !(line = ptr = getaline(in)) )
		return 1; /* error / EOF */

	while( ISSPACE(*ptr) )
		ptr++;

	if(strncmp("alias", ptr, 5)) {
		free(line);
		return 1;
	}

	ptr += 5;

	while( ISSPACE(*ptr) )
		ptr++;

	tmp = ptr;

	while( ! ISSPACE(*ptr) )
		ptr++;

	alias_len = (size_t)(ptr - tmp);

	*alias = xmalloc_inc(alias_len, 1);

	strncpy(*alias, tmp, alias_len);
	*(*alias + alias_len) = 0;

	while(ISSPACE(*ptr))
		ptr++;

	*rest = xstrdup(ptr);

	free(line);
	return 0;
}

static void
mutt_fix_quoting(char *p)
{
	char *escape = 0;

	for(; *p; p++) {
		switch(*p) {
			case '\"':
				if(escape)
					*escape = ' ';
				break;
			case '\\':
				escape = p;
				break;
			default:
				escape = 0;
		}
	}
}

static void
mutt_parse_email(list_item item)
{
	char *line = item[NAME];
	char *tmp;
	char *name, *email;
#if 0
	char *start = line;
	int i = 0;
#endif

	mutt_fix_quoting(line);
	tmp = strconcat("From: ", line, NULL);
	getname(tmp, &name, &email);
	free(tmp);

	if(name)
		item[NAME] = name;
	else
		return;
	if(email)
		item[EMAIL] = email;
	else
		return;

	/*
	 * this is completely broken
	 */
#if 0
	while( (start = strchr(start, ',')) && i++ < MAX_EMAILS - 1) {
		tmp = strconcat("From: ", ++start, NULL);
		getname(tmp, &name, &email);
		free(tmp);
		free(name);
		if(email) {
			if(*email) {
				tmp = strconcat(item[EMAIL], ",", email, NULL);
				free(item[EMAIL]);
				item[EMAIL] = tmp;
			} else {
				xfree(email);
			}
		}
	}
#endif
}

static int
mutt_parse_file(FILE *in)
{
	list_item item;

	for(;;) {
		memset(item, 0, sizeof(item));

		if(!mutt_read_line(in, &item[NICK],
				&item[NAME]) )
			mutt_parse_email(item);

		if(feof(in)) {
			free_list_item(item);
			break;
		}

		add_item2database(item);
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

		for(j = 0; j < LDIF_ITEM_FIELDS; j++) {
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

extern int extra_column;

static int
html_export_database(FILE *out, struct db_enumerator e)
{
	char tmp[MAX_EMAILSTR_LEN];

	if(items < 1)
		return 2;

	extra_column = (extra_column > 2 && extra_column < ITEM_FIELDS) ?
		extra_column : PHONE;

	html_export_write_head(out, extra_column);

	db_enumerate_items(e) {
		get_first_email(tmp, e.item);
		if (*tmp)
		    fprintf(out, "<tr>\n<td><a href=\"mailto:%s\">%s</a>\n",
			    tmp,
			    database[e.item][NAME] );
		else
		    fprintf(out, "<tr>\n<td>%s\n",
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

	fprintf(out, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
	fprintf(out, "<html>\n<head>\n <title>%s's addressbook</title>", realname );
	fprintf(out, "\n</head>\n<body>\n");
	fprintf(out, "\n<h2>%s's addressbook</h2>\n", realname );
	fprintf(out, "<br><br>\n\n");

	fprintf(out, "<table border=\"1\" align=\"center\">\n");
	fprintf(out, "\n<tr><th>Name<th>E-mail address(es)<th>%s</tr>\n\n",
			abook_fields[extra_column].name);

	free(realname);
}

static void
html_export_write_tail(FILE *out)
{
	fprintf(out, "\n</table>\n");
	fprintf(out, "\n</body>\n</html>\n");
}

/*
 * end of html export filter
 */


/*
 * pine addressbook import filter
 */

#define PINE_BUF_SIZE 2048

static void
pine_fixbuf(char *buf)
{
	int i,j;

	for(i=0,j=0; j < (int)strlen(buf); i++, j++)
		buf[i] = buf[j] == '\n' ? buf[++j] : buf[j];
}

static void
pine_convert_emails(char *s)
{
	int i;
	char *tmp;

	if(s == NULL || *s != '(')
		return;

	for(i=0; s[i]; i++ )
		s[i] = s[i+1];

	if( ( tmp = strchr(s,')')) )
		*tmp=0;

	for(i = 1; ( tmp = strchr(s, ',') ) != NULL ; i++, s = tmp + 1)
		if(i > MAX_EMAILS - 1) {
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
	char tmp[PINE_BUF_SIZE];
	int i, len, last;
	int pine_conv_table[]= {NICK, NAME, EMAIL, -1, NOTES};

	memset(&item, 0, sizeof(item));

	for(i=0, last=0; !last ; i++) {
		if( !(end = strchr(start, '\t')) )
			last=1;

		len = last ? strlen(start) : (int) (end-start);
		len = min(len, PINE_BUF_SIZE - 1);

		if(i < (int)(sizeof(pine_conv_table) / sizeof(*pine_conv_table))
				&& pine_conv_table[i] >= 0) {
			strncpy(tmp, start, len);
			tmp[len] = 0;
			if(*tmp)
				item[pine_conv_table[i]] = xstrdup(tmp);
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
	char *buf = NULL;
	char *ptr;
	int i;

	fgets(line, LINESIZE, in);

	while(!feof(in)) {
		for(i = 2;;i++) {
			buf = xrealloc(buf, i*LINESIZE);
			if(i == 2)
				strcpy(buf, line);
			fgets(line, LINESIZE, in);
			ptr=(char *)&line;
			if(*ptr != ' ' || feof(in))
				break;
			else
				while(*ptr == ' ')
					ptr++;

			strcat(buf, ptr);
		}
		if(*buf == '#') {
			xfree(buf);
			continue;
		}
		pine_fixbuf(buf);

		pine_parse_buf(buf);

		xfree(buf);
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
 * csv import filter
 */

/* FIXME
 * these files should be parsed according to a certain
 * lay out, or the default if layout is not given, at
 * the moment only default is done...
 */

#define CSV_COMMENT_CHAR	'#'
#define CSV_DUPLICATE_SEPARATOR	" "
#define CSV_TABLE_SIZE(t)	(sizeof (t) / sizeof *(t))

static int csv_conv_table[] = {
	NAME,
	EMAIL,
	PHONE,
	NOTES,
	NICK
};

static int allcsv_conv_table[] = {
	NAME,
	EMAIL,
	ADDRESS,
	ADDRESS2,
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
	NOTES,
	CUSTOM1,
	CUSTOM2,
	CUSTOM3,
	CUSTOM4,
	CUSTOM5,
};

static int palmcsv_conv_table[] = {
	NAME,		/* Last name */
	NAME,		/* First name */
	NOTES,		/* Title */
	NICK,		/* Company */
	WORKPHONE,
	PHONE,
	FAX,
	MOBILEPHONE,
	EMAIL,
	ADDRESS,
	CITY,
	STATE,
	ZIP,
	COUNTRY,
	CUSTOM1,
	CUSTOM2,
	CUSTOM3,
	CUSTOM4,
	CUSTOM5,
};

static void
csv_convert_emails(char *s)
{
	int i;
	char *tmp;

	if(s == NULL)
		return;

	for(i = 1; ( tmp = strchr(s, ',') ) != NULL ; i++, s = tmp + 1)
		if(i > MAX_EMAILS - 1) {
			*tmp = 0;
			break;
		}

}

static char *
csv_remove_quotes(char *s)
{
	char *copy, *trimmed;
	int len;

	copy = trimmed = xstrdup(s);
	strtrim(trimmed);

	len = strlen(trimmed);
	if(trimmed[len - 1] == '\"' && *trimmed == '\"') {
		if(len < 3) {
			xfree(copy);
			return NULL;
		}
		trimmed[len - 1] = 0;
		trimmed++;
		trimmed = xstrdup(trimmed);
		free(copy);
		return trimmed;
	}

	xfree(copy);
	return xstrdup(s);
}

static int
csv_field_to_item(int *table_base, size_t table_size, int field)
{
	if(field < table_size)
		return table_base[field];

	return -1;
}

static void
csv_store_item(list_item item, int i, char *s)
{
	char *newstr = NULL;

	if(!s || !*s)
		return;

	if( !(newstr = csv_remove_quotes(s)) )
		return;

	if(i >= 0) {
		if (item[i] != NULL) {
			char *oldstr = item[i];

			item[i] = strconcat(newstr, CSV_DUPLICATE_SEPARATOR,
				oldstr, NULL);
			xfree(newstr);
			xfree(oldstr);
		} else {
			item[i] = newstr;
		}
	} else {
		xfree(newstr);
	}
}

static int
csv_is_valid_quote_end(char *p)
{
	if(*p != '\"')
		return FALSE;

	for(p++; *p; p++) {
		if(*p == ',')
			return TRUE;
		else if(!ISSPACE(*p))
			return FALSE;
	}

	return TRUE;
}

static int
csv_is_valid_quote_start(char *p)
{
	for(; *p; p++) {
		if(*p == '\"')
			return TRUE;
		else if(!ISSPACE(*p))
			return FALSE;
	}

	return FALSE;
}

static void
csv_parse_line(char *line, int *table_base, size_t table_size)
{
	char *p, *start;
	int field;
	bool in_quote = FALSE;
	list_item item;

	memset(item, 0, sizeof(item));

	for(p = start = line, field = 0; *p; p++) {
		if(in_quote) {
			if(csv_is_valid_quote_end(p))
				in_quote = FALSE;
		} else {
			if ( (((p - start) / sizeof (char)) < 2 ) &&
				csv_is_valid_quote_start(p) )
				in_quote = TRUE;
		}

		if(*p == ',' && !in_quote) {
			*p = 0;
			csv_field_to_item(table_base, table_size, field);
			csv_store_item(item,
				csv_field_to_item(table_base,
							table_size, field),
				start);
			field++;
			start = p + 1;
		}
	}
	/*
	 * store last field
	 */
	csv_store_item(item, csv_field_to_item(table_base, table_size, field),
		start);

	csv_convert_emails(item[EMAIL]);
	add_item2database(item);
}

static int
csv_parse_file_common(FILE *in, int *conv_table, size_t table_size)
{
	char *line = NULL;

	while(!feof(in)) {
		line = getaline(in);

		if(line && *line && *line != CSV_COMMENT_CHAR)
			csv_parse_line(line, conv_table, table_size);

		xfree(line);
	}

	return 0;
}

static int
csv_parse_file(FILE *in)
{
	return csv_parse_file_common(in, csv_conv_table,
		CSV_TABLE_SIZE(csv_conv_table));
}

static int
allcsv_parse_file(FILE *in)
{
	return csv_parse_file_common(in, allcsv_conv_table,
		CSV_TABLE_SIZE(allcsv_conv_table));
}

static int
palmcsv_parse_file(FILE *in)
{
	return csv_parse_file_common(in, palmcsv_conv_table,
		CSV_TABLE_SIZE(palmcsv_conv_table));
}

/*
 * end of csv import filter
 */

/*
 * csv addressbook export filters
 */

#define CSV_LAST		(-1)
#define CSV_UNDEFINED		(-2)
#define CSV_SPECIAL(X)		(-3 - (X))
#define CSV_IS_SPECIAL(X)	((X) <= -3)

static int
csv_export_common(FILE *out, struct db_enumerator e,
		int fields[], void (*special_func)(FILE *, int, int))
{
	int i;

	db_enumerate_items(e) {
		for(i = 0; fields[i] != CSV_LAST; i++) {
			if(fields[i] == CSV_UNDEFINED)
				fprintf(out, "\"\"");
			else if(CSV_IS_SPECIAL(fields[i])) {
				if(special_func)
					(*special_func)(out, e.item, fields[i]);
			} else
				/*fprintf(out,(
			strchr(safe_str(database[e.item][fields[i]]), ',') ||
			strchr(safe_str(database[e.item][fields[i]]), '\"')) ?
				"\"%s\"" : "%s",
				safe_str(database[e.item][fields[i]])
				);*/
				fprintf(out, "\"%s\"",
					safe_str(database[e.item][fields[i]]));

			if(fields[i + 1] != CSV_LAST)
				fputc(',', out);
		}
		fputc('\n', out);
	}

	return 0;
}

static int
csv_export_database(FILE *out, struct db_enumerator e)
{
	int csv_export_fields[] = {
		NAME,
		EMAIL,
		PHONE,
		NOTES,
		NICK,
		CSV_LAST
	};

	csv_export_common(out, e, csv_export_fields, NULL);

	return 0;
}

static int
allcsv_export_database(FILE *out, struct db_enumerator e)
{
	/*
	 * TODO: Should get these atomatically from abook_fileds
	 *  - JH
	 */
	int allcsv_export_fields[] = {
		NAME,
		EMAIL,
		ADDRESS,
		ADDRESS2,
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
		NOTES,
		CUSTOM1,
		CUSTOM2,
		CUSTOM3,
		CUSTOM4,
		CUSTOM5,
		CSV_LAST
	};

	fprintf(out, "#");
	fprintf(out, "\"NAME\",");
	fprintf(out, "\"EMAIL\",");
	fprintf(out, "\"ADDRESS\",");
	fprintf(out, "\"ADDRESS2\",");
	fprintf(out, "\"CITY\",");
	fprintf(out, "\"STATE\",");
	fprintf(out, "\"ZIP\",");
	fprintf(out, "\"COUNTRY\",");
	fprintf(out, "\"PHONE\",");
	fprintf(out, "\"WORKPHONE\",");
	fprintf(out, "\"FAX\",");
	fprintf(out, "\"MOBILEPHONE\",");
	fprintf(out, "\"NICK\",");
	fprintf(out, "\"URL\",");
	fprintf(out, "\"NOTES\",");
	fprintf(out, "\"CUSTOM1\",");
	fprintf(out, "\"CUSTOM2\",");
	fprintf(out, "\"CUSTOM3\",");
	fprintf(out, "\"CUSTOM4\",");
	fprintf(out, "\"CUSTOM5\"\n");

	csv_export_common(out, e, allcsv_export_fields, NULL);

	return 0;
}

/*
 * palm csv
 */

#define PALM_CSV_NAME	CSV_SPECIAL(0)
#define PALM_CSV_END	CSV_SPECIAL(1)
#define PALM_CSV_CAT	CSV_SPECIAL(2)

static void
palm_split_and_write_name(FILE *out, char *name)
{
	char *p;

	assert(name);

	if ( (p = strchr(name, ' ')) ) {
		/*
		 * last name first
		 */
		fprintf(out, "\"%s\",\"" , p + 1);
		fwrite((void *)name, p - name, sizeof(char), out);
		fputc('\"', out);
	} else {
		fprintf(out, "\"%s\"", safe_str(name));
	}
}

static void
palm_csv_handle_specials(FILE *out, int item, int field)
{
	switch(field) {
		case PALM_CSV_NAME:
			palm_split_and_write_name(out, database[item][NAME]);
			break;
		case PALM_CSV_CAT:
			fprintf(out, "\"abook\"");
			break;
		case PALM_CSV_END:
			fprintf(out, "\"0\"");
			break;
		default:
			assert(0);
	}
}

static int
palm_export_database(FILE *out, struct db_enumerator e)
{
	int palm_export_fields[] = {
		PALM_CSV_NAME,		/* LASTNAME, FIRSTNAME 	*/
		CSV_UNDEFINED,		/* TITLE   		*/
		CSV_UNDEFINED, 		/* COMPANY 		*/
		WORKPHONE,		/* WORK PHONE 		*/
		PHONE,			/* HOME PHONE		*/
		FAX,			/* FAX			*/
		MOBILEPHONE, 		/* OTHER 		*/
		EMAIL,			/* EMAIL		*/
		ADDRESS,		/* ADDRESS		*/
		CITY,			/* CITY			*/
		STATE,			/* STATE		*/
		ZIP,			/* ZIP			*/
		COUNTRY,		/* COUNTRY		*/
        	NICK, 			/* DEFINED 1		*/
        	URL, 			/* DEFINED 2 		*/
		CSV_UNDEFINED,		/* DEFINED 3		*/
		CSV_UNDEFINED,		/* DEFINED 4 		*/
		NOTES,			/* NOTE			*/
		PALM_CSV_END,		/* "0" 			*/
		PALM_CSV_CAT,		/* CATEGORY 		*/
		CSV_LAST
	};

	csv_export_common(out, e, palm_export_fields, palm_csv_handle_specials);

	return 0;
}

/*
 * end of csv export filters
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
			fprintf(out, "ADR:;;%s;%s;%s;%s;%s;%s\n",
				safe_str(database[e.item][ADDRESS]),
				safe_str(database[e.item][ADDRESS2]),
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
		return xstrdup(database[i][NICK]);

	tmp = xstrdup(database[i][NAME]);

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
		xfree(alias);
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

   	if (database[i][ADDRESS2])
		fprintf(out, "\n%s", database[i][ADDRESS2]);

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

   	if (database[i][ADDRESS2])
		fprintf(out, "\n%s", database[i][ADDRESS2]);

	if (database[i][ZIP] || database[i][CITY]) {
		fputc('\n', out);

		if(database[i][ZIP]) {
			fprintf(out, "%s", database[i][ZIP]);
			if(database[i][CITY])
				fputc(' ', out);
		}

		if(database[i][CITY])
			fprintf(out, "%s", database[i][CITY]);
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
	char *style = opt_get_str(STR_ADDRESS_STYLE);

	fprintf(out,
		"-----------------------------------------\n%s's address book\n"
		"-----------------------------------------\n\n\n",
		realname);
	free(realname);

	db_enumerate_items(e) {
		fprintf(out,
			"-----------------------------------------\n\n");
		fprintf(out, "%s", database[e.item][NAME]);
		if (database[e.item][NICK] && *database[e.item][NICK])
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
		xfree(alias);
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

