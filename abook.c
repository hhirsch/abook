/*
 * $Id$
 *
 * by JH <jheinonen@users.sourceforge.net>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
#	include <locale.h>
#endif
#include "abook.h"
#include "ui.h"
#include "database.h"
#include "list.h"
#include "filter.h"
#include "edit.h"
#include "misc.h"
#include "options.h"
#include "getname.h"
#include "getopt.h"
#include <assert.h>

static void             init_abook();
static void		quit_abook_sig(int i);
static void             set_filenames();
static void             parse_command_line(int argc, char **argv);
static void             show_usage();
static void             mutt_query(char *str);
static void             init_mutt_query();
static void		convert(char *srcformat, char *srcfile,
				char *dstformat, char *dstfile);
static void		add_email(int);

char *datafile = NULL;
char *rcfile = NULL;

bool alternative_datafile = FALSE;
bool alternative_rcfile = FALSE;

static int
datafile_writeable()
{
	FILE *f;

	assert(datafile != NULL);

	if( (f = fopen(datafile, "a")) == NULL)
		return FALSE;

	fclose(f);

	return TRUE;
}

static void
check_abook_directory()
{
	struct stat s;
	char *dir;

	assert(!is_ui_initialized());

	if(alternative_datafile)
		return;

	dir = strconcat(getenv("HOME"), "/" DIR_IN_HOME, NULL);
	assert(dir != NULL);

	if(stat(dir, &s) == -1) {
		if(errno != ENOENT) {
			perror(dir);
                        free(dir);
                        exit(1);
		}
		if(mkdir(dir, 0700) == -1) {
			printf("Cannot create directory %s\n", dir);
			perror(dir);
			free(dir);
			exit(1);
		}
	} else if(!S_ISDIR(s.st_mode)) {
		printf("%s is not a directory\n", dir);
		free(dir);
		exit(1);
	}

	free(dir);
}

static void
init_abook()
{
	set_filenames();
	check_abook_directory();
	init_opts();
	if(load_opts(rcfile) > 0) {
		printf("Press enter to continue...\n");
		fgetc(stdin);
	}

	signal(SIGKILL, quit_abook_sig);
	signal(SIGTERM, quit_abook_sig);

	if( init_ui() )
		exit(1);

	umask(DEFAULT_UMASK);

	if(!datafile_writeable()) {
		char *s = mkstr("File %s is not writeable", datafile);
		refresh_screen();
		statusline_msg(s);
		free(s);
		if(load_database(datafile) || !statusline_ask_boolean(
					"If you continue all changes will "
				"be lost. Do you want to continue?", FALSE)) {
			free_opts();
			/*close_database();*/
			close_ui();
			exit(1);
		}
	} else
		load_database(datafile);

	refresh_screen();
}

void
quit_abook()
{
	if( opt_get_bool(BOOL_AUTOSAVE) )
		save_database();
	else if( statusline_ask_boolean("Save database", TRUE) )
		save_database();

	free_opts();
	close_database();

	close_ui();

	exit(0);
}

static void
quit_abook_sig(int i)
{
	quit_abook();
}

int
main(int argc, char **argv)
{
#if defined(HAVE_SETLOCALE) && defined(HAVE_LOCALE_H)
	setlocale(LC_ALL, "" );
#endif

	parse_command_line(argc, argv);

	init_abook();

	get_commands();	

	quit_abook();

	return 0;
}

static void
free_filenames()
{
	my_free(rcfile);
	my_free(datafile);
}


static void
set_filenames()
{
	struct stat s;

	if( (stat(getenv("HOME"), &s)) == -1 || ! S_ISDIR(s.st_mode) ) {
		fprintf(stderr,"%s is not a valid HOME directory\n", getenv("HOME") );
		exit(1);
	}

	if(!datafile)
		datafile = strconcat(getenv("HOME"), "/" DIR_IN_HOME "/"
				DATAFILE, NULL);

	if(!rcfile)
		rcfile = strconcat(getenv("HOME"), "/" DIR_IN_HOME "/"
				RCFILE, NULL);

	atexit(free_filenames);
}

/*
 * CLI
 */

enum {
	MODE_CONT,
	MODE_ADD_EMAIL,
	MODE_ADD_EMAIL_QUIET,
	MODE_QUERY,
	MODE_CONVERT
};

static void
change_mode(int *current, int mode)
{
	if(*current != MODE_CONT) {
		fprintf(stderr, "Cannot combine options --mutt-query, "
				"--convert, "
				"--add-email or --add-email-quiet\n");
		exit(1);
	}

	*current = mode;
}

void
set_filename(char **var, char *path)
{
	char *cwd;

	assert(var != NULL);
	assert(*var == NULL); /* or else we probably leak memory */
	assert(path != NULL);

	if(*path == '/') {
		*var = strdup(path);
		return;
	}

	cwd = my_getcwd();

	*var = strconcat(cwd, "/", path, NULL);

	free(cwd);
}

#define set_convert_var(X) do { if(mode != MODE_CONVERT) {\
	fprintf(stderr, "please use option --%s after --convert option\n",\
			long_options[option_index].name);\
		exit(1);\
	} else\
		X = optarg;\
	} while(0)

static void
parse_command_line(int argc, char **argv)
{
	int mode = MODE_CONT;
	char *query_string = NULL;
	char *informat = "abook",
		*outformat = "text",
		*infile = "-",
		*outfile = "-";
	int c;

	for(;;) {
		int option_index = 0;
		enum {
			OPT_ADD_EMAIL,
			OPT_ADD_EMAIL_QUIET,
			OPT_MUTT_QUERY,
			OPT_CONVERT,
			OPT_INFORMAT,
			OPT_OUTFORMAT,
			OPT_INFILE,
			OPT_OUTFILE,
			OPT_FORMATS
		};
		static struct option long_options[] = {
			{ "help", 0, 0, 'h' },
			{ "add-email", 0, 0, OPT_ADD_EMAIL },
			{ "add-email-quiet", 0, 0, OPT_ADD_EMAIL_QUIET },
			{ "datafile", 1, 0, 'f' },
			{ "mutt-query", 1, 0, OPT_MUTT_QUERY },
			{ "config", 1, 0, 'C' },
			{ "convert", 0, 0, OPT_CONVERT },
			{ "informat", 1, 0, OPT_INFORMAT },
			{ "outformat", 1, 0, OPT_OUTFORMAT },
			{ "infile", 1, 0, OPT_INFILE },
			{ "outfile", 1, 0, OPT_OUTFILE },
			{ "formats", 0, 0, OPT_FORMATS },
			{ 0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "hC:",
				long_options, &option_index);

		if(c == -1)
			break;

		switch(c) {
			case 'h':
				show_usage();
				exit(1);
			case OPT_ADD_EMAIL:
				change_mode(&mode, MODE_ADD_EMAIL);
				break;
			case OPT_ADD_EMAIL_QUIET:
				change_mode(&mode, MODE_ADD_EMAIL_QUIET);
				break;
			case 'f':
				set_filename(&datafile, optarg);
				alternative_datafile = TRUE;
				break;
			case OPT_MUTT_QUERY:
				query_string = optarg;
				change_mode(&mode, MODE_QUERY);
				break;
			case 'C':
				set_filename(&rcfile, optarg);
				alternative_rcfile = TRUE;
				break;
			case OPT_CONVERT:
				change_mode(&mode, MODE_CONVERT);
				break;
			case OPT_INFORMAT:
				set_convert_var(informat);
				break;
			case OPT_OUTFORMAT:
				set_convert_var(outformat);
				break;
			case OPT_INFILE:
				set_convert_var(infile);
				break;
			case OPT_OUTFILE:
				set_convert_var(outfile);
				break;
			case OPT_FORMATS:
				print_filters();
				exit(0);
			default:
				exit(1);
		}
	}

	if (optind < argc) {
		fprintf(stderr, "%s: unrecognized arguments on command line\n",
				argv[0]);
		exit(1);
	}

	switch(mode) {
		case MODE_ADD_EMAIL:
			add_email(0);
		case MODE_ADD_EMAIL_QUIET:
			add_email(1);
		case MODE_QUERY:
			mutt_query(query_string);
		case MODE_CONVERT:
			convert(informat, infile, outformat, outfile);
	}
}


static void
show_usage()
{
	puts	(PACKAGE " v " VERSION "\n");
	puts	("     -h	--help				show usage");
	puts	("     -C	--config	<file>		use an alternative configuration file");   
	puts	("	--datafile	<file>		use an alternative addressbook file");
	puts	("	--mutt-query	<string>	make a query for mutt");
	puts	("	--add-email			"
			"read an e-mail message from stdin and\n"
		"					"
		"add the sender to the addressbook");
	puts	("	--add-email-quiet		"
		"same as --add-email but doesn't\n"
		"					confirm adding");
	putchar('\n');
	puts	("	--convert			convert address book files");
	puts	("	options to use with --convert:");
	puts	("	--informat	<format>	format for input file");
	puts	("					(default: abook)");
	puts	("	--infile	<file>		source file");
	puts	("					(default: stdin)");
	puts	("	--outformat	<format>	format for output file");
	puts	("					(default: text)");
	puts	("	--outfile	<file>		destination file");
	puts	("					(default: stdout)");
	puts	("	--formats			list available formats");
}

/*
 * end of CLI
 */

extern list_item *database;

static void
quit_mutt_query(int status)
{
	close_database();
	free_opts();

	exit(status);
}

static void
muttq_print_item(FILE *file, int item)
{
	char emails[MAX_EMAILS][MAX_EMAIL_LEN];
	int i;

	split_emailstr(item, emails);

	for(i = 0; i < (opt_get_bool(BOOL_MUTT_RETURN_ALL_EMAILS) ?
			MAX_EMAILS : 1) ; i++)
		if( *emails[i] )
			fprintf(file, "%s\t%s\t%s\n", emails[i],
				database[item][NAME],
				database[item][NOTES] == NULL ? " " :
					database[item][NOTES]
				);
}

static void
mutt_query(char *str)
{
	init_mutt_query();

	if( str == NULL || !strcasecmp(str, "all") ) {
		struct db_enumerator e = init_db_enumerator(ENUM_ALL);
		printf("All items\n");
		db_enumerate_items(e)
			muttq_print_item(stdout, e.item);
	} else {
		int search_fields[] = {NAME, EMAIL, NICK, -1};
		int i;
		if( (i = find_item(str, 0, search_fields)) < 0 ) {
			printf("Not found\n");
			quit_mutt_query(1);
		}
		putchar('\n');
		while(i >= 0) {
			muttq_print_item(stdout, i);
			i = find_item(str, i+1, search_fields);
		}
	}

	quit_mutt_query(0);
}

static void
init_mutt_query()
{
	set_filenames();
	init_opts();
	load_opts(rcfile);

	if( load_database(datafile) ) {
		printf("Cannot open database\n");
		quit_mutt_query(1);
		exit(1);
	}
}


static char *
make_mailstr(int item)
{
	char email[MAX_EMAIL_LEN];
	char *ret;
	char *name = mkstr("\"%s\"", database[item][NAME]);

	get_first_email(email, item);

	ret = *database[item][EMAIL] ?
		mkstr("%s <%s>", name, email) :
		strdup(name);

	free(name);

	return ret;
}

void
print_stderr(int item)
{
	fprintf (stderr, "%c", '\n');

	if( is_valid_item(item) )
		muttq_print_item(stderr, item);
	else {
		struct db_enumerator e = init_db_enumerator(ENUM_SELECTED);
		db_enumerate_items(e) {
			muttq_print_item(stderr, e.item);
		}
	}

}

void
launch_mutt(int item)
{
	char *cmd = NULL, *mailstr = NULL;
	char *mutt_command = opt_get_str(STR_MUTT_COMMAND);

	if(mutt_command == NULL || !*mutt_command)
		return;

	if( is_valid_item(item) )
		mailstr = make_mailstr(item);
	else {
		struct db_enumerator e = init_db_enumerator(ENUM_SELECTED);
		char *tmp = NULL;
		db_enumerate_items(e) {
			tmp = mailstr;
			mailstr = tmp ?
				strconcat(tmp, ",", make_mailstr(e.item), NULL):
				strconcat(make_mailstr(e.item), NULL);
			free(tmp);
		}
	}

	cmd = strconcat(mutt_command, " \'", mailstr,
				"\'", NULL);
	free(mailstr);
#ifdef DEBUG
	fprintf(stderr, "cmd: %s\n", cmd);
#endif
	system(cmd);	
	free(cmd);

	/*
	 * we need to make sure that curses settings are correct
	 */
	ui_init_curses();
}

void
launch_wwwbrowser(int item)
{
	char *cmd = NULL;

	if( !is_valid_item(item) )
		return;

	if( database[item][URL] )
		cmd = mkstr("%s '%s'",
				opt_get_str(STR_WWW_COMMAND),
				safe_str(database[item][URL]));
	else
		return;

	if ( cmd )
		system(cmd);

	free(cmd);

	/*
	 * we need to make sure that curses settings are correct
	 */
	ui_init_curses();
}

void *
abook_malloc(size_t size)
{
	void *ptr;

	if ( (ptr = malloc(size)) == NULL ) {
		if( is_ui_initialized() )
			quit_abook();
		perror("malloc() failed");
		exit(1);
	}

	return ptr;
}

void *
abook_realloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);

	if( size == 0 )
		return NULL;

	if( ptr == NULL ) {
		if( is_ui_initialized() )
			quit_abook();
		perror("realloc() failed");
		exit(1);
	}

	return ptr;
}

FILE *
abook_fopen (const char *path, const char *mode)
{	
	struct stat s;
	bool stat_ok;

	stat_ok = (stat(path, &s) != -1);
	
	if(strchr(mode, 'r'))
		return (stat_ok && S_ISREG(s.st_mode)) ?
			fopen(path, mode) : NULL;
	else
		return (stat_ok && S_ISDIR(s.st_mode)) ?
			NULL : fopen(path, mode);
}

static void
convert(char *srcformat, char *srcfile, char *dstformat, char *dstfile)
{
	int ret=0;

	if( !srcformat || !srcfile || !dstformat || !dstfile ) {
		fprintf(stderr, "too few argumets to make conversion\n");
		fprintf(stderr, "try --help\n");
	}

#ifndef DEBUG
	if( !strcasecmp(srcformat, dstformat) ) {
		printf(	"input and output formats are the same\n"
			"exiting...\n");
		exit(1);
	}
#endif

	set_filenames();
	init_opts();
	load_opts(rcfile);

	switch( import_file(srcformat, srcfile) ) {
		case -1:
			fprintf(stderr,
				"input format %s not supported\n", srcformat);
			ret = 1;
			break;
		case 1:
			fprintf(stderr, "cannot read file %s\n", srcfile);
			ret = 1;
			break;
	}

	if(!ret)
		switch( export_file(dstformat, dstfile) ) {
			case -1:
				fprintf(stderr,
					"output format %s not supported\n",
					dstformat);
				ret = 1;
				break;
			case 1:
				fprintf(stderr,
					"cannot write file %s\n", dstfile);
				ret = 1;
				break;
		}

	close_database();
	free_opts();
	exit(ret);
}

/*
 * --add-email handling
 */

static int add_email_count = 0;

static void
quit_add_email()
{
	if(add_email_count > 0) {
		if(save_database() < 0) {
			fprintf(stderr, "cannot open %s\n", datafile);
			exit(1);
		}
		printf("%d item(s) added to %s\n", add_email_count, datafile);
	} else {
		puts("Valid sender address not found");
	}

	exit(0);
}

static void
quit_add_email_sig(int signal)
{
	quit_add_email();
}

static void
init_add_email()
{
	set_filenames();
	atexit(free_filenames);
	init_opts();
	load_opts(rcfile);
	atexit(free_opts);

	/*
	 * we don't actually care if loading fails or not
	 */
	load_database(datafile);

	atexit(close_database);

	signal(SIGINT, quit_add_email_sig);
}

static int
add_email_add_item(int quiet, char *name, char *email)
{
	list_item item;

	if(opt_get_bool(BOOL_ADD_EMAIL_PREVENT_DUPLICATES)) {
		int search_fields[] = { EMAIL, -1 };
		if(find_item(email, 0, search_fields) >= 0) {
			if(!quiet)
				printf("Address %s already in addressbook\n",
						email);
			return 0;
		}
	}
					
	if(!quiet) {
		FILE *in = fopen("/dev/tty", "r");
		char c;
		if(!in) {
			fprintf(stderr, "cannot open /dev/tty\n"
				"you may want to use --add-email-quiet\n");
			exit(1);
		}
		printf("Add ``%s <%s>'' to %s ? (y/n)\n",
				name,
				email,
				datafile
		);
		do {
			c = fgetc(in);
			if(c == 'n' || c == 'N') {
				fclose(in);
				return 0;
			}
		} while(c != 'y' && c != 'Y');
		fclose(in);
	}

	memset(item, 0, sizeof(item));
	item[NAME] = strdup(name);
	item[EMAIL] = strdup(email);
	add_item2database(item);

	return 1;
}

static void
add_email(int quiet)
{
	char *line;
	char *name = NULL, *email = NULL;
	struct stat s;

	if( (fstat(fileno(stdin), &s)) == -1 || S_ISDIR(s.st_mode)) {
		fprintf(stderr, "stdin is a directory or cannot stat stdin\n");
		exit(1);
	}

	init_add_email();

	do {
		line = getaline(stdin);
		if(line && !strncasecmp("From:", line, 5) ) {
			getname(line, &name, &email);
			add_email_count += add_email_add_item(quiet,
					name, email);
			my_free(name);
			my_free(email);
		}
		my_free(line);
	} while( !feof(stdin) );

	quit_add_email();
}

/*
 * end of --add-email handling
 */
