/*
 * abook.c
 * by JH <jheinonen@bigfoot.com>
 *
 * Copyright (C) 1999, 2000 Jaakko Heinonen
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include "abook_curses.h"
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
#	include <locale.h>
#endif
#ifdef HAVE_TERMIOS_H
#	include <termios.h>
#else
#	ifdef HAVE_LINUX_TERMIOS_H
#		include <linux/termios.h>
#	endif
#endif
#ifdef HAVE_SYS_IOCTL_H
#	include <sys/ioctl.h>
#endif
#include "abook.h"
#include "database.h"
#include "list.h"
#include "filter.h"
#include "edit.h"
#include "misc.h"
#include "help.h"
#include "options.h"
#include "estr.h"

static void             init_abook();
static void             set_filenames();
static void		free_filenames();
static void		display_help(char **tbl);
static void             get_commands();
static void             parse_command_line(int argc, char **argv);
static void             show_usage();
static void             mutt_query(char *str);
static void             init_mutt_query();
static void             quit_mutt_query();
static void             launch_mutt();
static void		launch_lynx();
static void		win_changed(int dummy);
static void		open_datafile();
#ifdef SIGWINCH
static void		resize_abook();
#endif
static void		convert(char *srcformat, char *srcfile,
				char *dstformat, char *dstfile);

int should_resize = FALSE;
int can_resize = FALSE;

char *datafile = NULL;
char *rcfile = NULL;

WINDOW *top = NULL, *bottom = NULL;

static void
init_windows()
{
	top = newwin(LIST_TOP - 1, COLS, 0, 0);
	
	bottom = newwin(LINES - LIST_BOTTOM, COLS, LIST_BOTTOM, 0);
}

static void
free_windows()
{
	delwin(top);
	delwin(bottom);
}

static void
init_abook()
{
	set_filenames();
	init_options();

	initscr(); cbreak(); noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
#ifdef DEBUG
        fprintf(stderr, "init_abook():\n");
        fprintf(stderr, "  COLS = %d, LINES = %d\n", COLS, LINES);
#endif
	if( LINES < MIN_LINES || COLS < MIN_COLS ) {
		clear(); refresh(); endwin();
		fprintf(stderr, "Your terminal size is %dx%d\n", COLS, LINES);
		fprintf(stderr, "Terminal is too small. Minium terminal size "
				"for abook is "
				"%dx%d\n", MIN_COLS, MIN_LINES);
		exit(1);
	}
	umask(DEFAULT_UMASK);
#ifdef SIGWINCH
	signal(SIGWINCH, win_changed);
#endif
	signal(SIGINT, quit_abook);
	signal(SIGKILL, quit_abook);
	signal(SIGTERM, quit_abook);
	
	init_list();
	init_windows();

	/*
	 * this is very ugly for now
	 */
	/*if( options_get_int("datafile", "autosave") )*/

	if( load_database(datafile) == 2 ) {
		char *tmp = strconcat(getenv("HOME"),
				"/" DATAFILE, NULL);

		if( safe_strcmp(tmp, datafile) ) {
			refresh_screen();
			statusline_msg("Sorry, the specified file does "
				"not appear to be a valid abook addressbook");
			statusline_msg("Will open default addressbook...");
			free(datafile);
			datafile = tmp;
			load_database(datafile);
		} else
			free(tmp);
	}

	refresh_screen();
}

void
quit_abook()
{
	if( options_get_int("autosave") )
		save_database();
	else {
		statusline_addstr("Save database (y/N)");
		switch( getch() ) {
			case 'y':
			case 'Y':
				save_database();
			default: break;
		}
	}
	close_config();
	close_database();
	close_list();
	free_windows();
	clear();
	refresh();
	endwin();

	exit(0);
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
set_filenames()
{
	struct stat s;

	if( (stat(getenv("HOME"), &s)) == -1 || ! S_ISDIR(s.st_mode) ) {
		fprintf(stderr,"%s is not a valid HOME directory\n", getenv("HOME") );
		exit(1);
	}

	if (!datafile)
		datafile = strconcat(getenv("HOME"), "/" DATAFILE, NULL);

	rcfile = strconcat(getenv("HOME"), "/" RCFILE, NULL);

	atexit(free_filenames);
}

static void
free_filenames()
{
	my_free(rcfile);
	my_free(datafile);
}

void
headerline(char *str)
{
	werase(top);
	
	mvwhline(top, 1, 0, ACS_HLINE, COLS);
	
	mvwprintw(top, 0, 0, "%s | %s", PACKAGE " " VERSION, str);

	refresh();
	wrefresh(top);
}
		

void
refresh_screen()
{
#ifdef SIGWINCH
	if( should_resize ) {
		resize_abook();
		return;
	}
#endif
	clear();
	
	refresh_statusline();
	headerline(MAIN_HELPLINE);
	list_headerline();

	refresh_list();
}

#ifdef DEBUG
extern int curitem;
extern list_item *database;
static void
dump_item()
{
	int i;

	fprintf(stderr,"sizeof(list_item) = %d\n", sizeof(list_item));
	fprintf(stderr,"--- dumping item %d ---\n", curitem);
	
	for(i=0; i<ITEM_FIELDS; i++)
		fprintf(stderr,"%d - %d\n",
				i, (int)database[curitem][i]);

	fprintf(stderr,"--- end of dump ---\n");
				
}
#endif

extern char *selected;
extern int curitem;

static void
get_commands()
{
	int ch;

	for(;;) {
		can_resize = TRUE; /* it's safe to resize now */
		hide_cursor();
		if( should_resize )
			refresh_screen();
		ch = getch();
		show_cursor();
		can_resize = FALSE; /* it's not safe to resize anymore */
		switch( ch ) {
			case 'q': return;
			case '?': display_help(mainhelp); break;
			case 'a': add_item();		break;
			case '\r': edit_item(-1);	break;
			case KEY_DC:
			case 'd':
			case 'r': remove_items();	break;
			case 12: refresh_screen();	break;

			case 'k':
			case KEY_UP: scroll_up();	break;
			case 'j':
			case KEY_DOWN: scroll_down();	break;
			case 'K':
			case KEY_PPAGE: page_up();	break;
			case 'J':
			case KEY_NPAGE: page_down();	break;

			case 'H':
			case KEY_HOME: goto_home();	break;
			case 'E':
			case KEY_END: goto_end();	break;

			case 'w': save_database();
				  break;
			case 'l': read_database();	break;
			case 'i': import_database();	break;
			case 'e': export_database();	break;
			case 'C': clear_database();	break;

			case 'y': edit_options();	break;
			case 'o': open_datafile();	break;

			case 's': sort_database();	break;
			case 'S': sort_surname();	break;

			case '/': find(0);		break;
			case '\\': find(1);		break;

			case ' ': if(curitem >= 0) {
				   selected[curitem] = !selected[curitem];
				   print_number_of_items();
				   refresh_list();
				  }
				break;
			case '+': select_all();
				  refresh_list();
				break;
			case '-': select_none();
				  refresh_list();
				break;
			case '*': invert_selection();
				  refresh_list();
				 break;
			case 'A': move_curitem(MOVE_ITEM_UP);
				break;
			case 'Z': move_curitem(MOVE_ITEM_DOWN);
				break;

			case 'm': launch_mutt(); break;

			case 'p': print_database(); break;

			case 'u': launch_lynx(); break;
#ifdef DEBUG
			case 'D': dump_item();
#endif
		}
	}
}


static void
display_help(char **tbl)
{
	int i, j = 3;

	erase();
	headerline("help");
	refresh_statusline();
	
	for( i = 0; tbl[i] != NULL; i++) {
		mvaddstr(j++, 0, tbl[i]);
		if( ( !( (i+1) % (LINES-7) ) ) ||
				(tbl[i+1] == NULL) ) {
			refresh();
			statusline_msg("Press any key to continue...");
			erase();
			refresh_statusline();
			headerline("help");
			j = 3;
		}
	}
	refresh_screen();
}

void
display_editor_help(WINDOW *w)
{
	int i;

	werase(w);

	headerline("editor help");

	for( i = 0; editorhelp[i] != NULL; i++) {
		waddstr(w, editorhelp[i]);
		if( ( !( (i+1) % (LINES-8) ) ) ||
			(editorhelp[i+1] == NULL) ) {
			refresh();
			wrefresh(w);
			statusline_msg("Press any key to continue...");
			wclear(w);
		}
	}
}


void
statusline_msg(char *msg)
{
	clear_statusline();
	statusline_addstr(msg);
	getch();
#ifdef DEBUG
	fprintf(stderr, "statusline_msg(\"%s\")\n", msg);
#endif
	clear_statusline();
}

void
statusline_addstr(char *str)
{
	mvwaddstr(bottom, 1, 0, str);
	refresh();
	wrefresh(bottom);
}

/*
 * function statusline_getnstr
 *
 * parameters:
 *  (char *str)
 *   if n >= 0 str is a pointer which points a place where to store
 *   the string, else str is ingnored
 *  (int n)
 *   the maximum length of the string
 *   If n < 0 function will allocate needed space for the string.
 *   Value 0 is not allowed for n.
 *  (int use_filesel)
 *   if this value is nonzero the fileselector is enabled
 *
 *  returns (char *)
 *   If n < 0 a pointer to a newly allocated string is returned.
 *   If n > 0 a nonzero value is returned if user has typed a valid
 *   string. If not NULL value is returned. Never really use the
 *   _pointer_ if n > 0.
 *
 */

char *
statusline_getnstr(char *str, int n, int use_filesel)
{
	char *buf;
	int y, x;

	getyx(bottom, y, x);
	wmove(bottom, 1, x);
	
	buf = wenter_string(bottom, n,
			(use_filesel ? ESTR_USE_FILESEL:0) | ESTR_DONT_WRAP);

	if(n < 0)
		return buf;
	
	if(buf == NULL)
		str[0] = 0;
	else
		strncpy(str, buf, n);

	str[n-1] = 0;

	free(buf);

	return buf;
}

void
refresh_statusline()
{
	werase(bottom);

	mvwhline(bottom, 0, 0, ACS_HLINE, COLS);
	mvwhline(bottom, 2, 0, ACS_HLINE, COLS);

	refresh();
	wrefresh(bottom);
}
	

char *
ask_filename(char *prompt, int flags)
{
	char *buf = NULL;

	clear_statusline();
	
	statusline_addstr(prompt);
	buf = statusline_getnstr(NULL, -1, flags);

	clear_statusline();

	return buf;
}

void
clear_statusline()
{
	wmove(bottom, 1, 0);
	wclrtoeol(bottom);
	wrefresh(bottom);
	refresh();
}

static void
parse_command_line(int argc, char **argv)
{
	int i;

	for( i = 1; i < argc; i++ ) {
		if( !strcmp(argv[i], "--help") ) {
			show_usage();
			exit(1);
		} else
		if( !strcmp(argv[i], "--mutt-query") )
			mutt_query(argv[i + 1]);
		else
		if( !strcmp(argv[i], "--datafile") ) {
			if (argv[i+1]) {
				if (argv[i+1][0] != '/') {
					char *cwd = my_getcwd();
					datafile = strconcat(cwd, "/", argv[i+1], NULL);
					free(cwd);
				} else {
					datafile = strdup(argv[i+1]);
				}
				i++;
			} else {
				show_usage();
				exit(1);
			}
		} else
		if( !strcmp(argv[i], "--convert") ) {
			if( argc < 5 || argc > 6 ) {
				fprintf(stderr, "incorrect number of argumets to make conversion\n");
				fprintf(stderr, "try %s --help\n", argv[0]);
				exit(1);
			}
			if( argv[i+4] )
				convert(argv[i+1], argv[i+2],
					argv[i+3], argv[i+4]);
			else
				convert(argv[i+1], argv[i+2], argv[i+3], "-");
		} else {
			printf("option %s not recognized\n", argv[i]);
			printf("try %s --help\n", argv[0]);
			exit(1);
		}
	}
}


static void
show_usage()
{
	puts	(PACKAGE " v " VERSION "\n");
	puts	("	--help				show usage");
	puts	("	--datafile	<filename>	use an alternative addressbook file");
	puts	("	--mutt-query	<string>	make a query for mutt");
	puts	("	--convert	<inputformat> <inputfile> "
		 "<outputformat> <outputfile>");
	putchar('\n');
	puts	("available formats for --convert option:");
	print_filters();
#ifdef DEBUG
	puts	("\nWarning: this version compiled with DEBUG flag ON");
#endif
}

extern list_item *database;
extern int items;

static void
muttq_print_item(int item)
{
	char emails[MAX_EMAILS][MAX_EMAIL_LEN];
	int i;

	split_emailstr(item, emails);
	
	for(i = 0; i < (options_get_int("mutt_return_all_emails") ?
			MAX_EMAILS : 1) ; i++)
		if( *emails[i] )
			printf("%s\t%s\t%s\n", emails[i],
				database[item][NAME],
				database[item][NOTES] == NULL ? " " :
					database[item][NOTES]
				);
}

static int
mutt_query_name(char *str)
{
	int i, j;
	char *tmp;

	for(i = 0, j = 0 ; i < items; i++) {
		tmp = strdup(database[i][NAME]);
		if( strstr( strupper(tmp), strupper(str) ) != NULL ) {
			if( !j )
				putchar('\n');
			muttq_print_item(i);
			j++;
		}
		free(tmp);
	}

	return j;
}

static int
mutt_query_email(char *str)
{
	int i, j, k;
	char *tmp, emails[MAX_EMAILS][MAX_EMAIL_LEN];

	for(i = 0, j = 0; i < items; i++) {
		split_emailstr(i, emails);
		for(k = 0; k < MAX_EMAILS; k++) {
			if( *emails[k] ) {
				tmp = strdup( emails[k] );
				if( strstr( strupper(tmp), strupper(str) ) != NULL ) {
					if( !j )
						putchar('\n');
					j++;
					if( options_get_int("mutt_return_all_emails") ) {
						muttq_print_item(i);
						free(tmp);
						break;
					} else
						printf("%s\t%s\n", emails[k],
							database[i][NAME]);
				}
				free(tmp);
			}
		}
	}

	return j;
}

static void
mutt_query(char *str)
{
	int i;
	
	init_mutt_query();

	if( str == NULL || !strcasecmp(str, "all") ) {
		printf("All items\n");
		for(i = 0; i < items; i++)
			muttq_print_item(i);
	} else {
		if( !mutt_query_name(str) && !mutt_query_email(str) ) {
			printf("Not found\n");
			quit_mutt_query(1);
		}
	}

	quit_mutt_query(0);
}

static void
init_mutt_query()
{
	set_filenames();
	init_options();
	
	if( load_database(datafile) ) {
		printf("Cannot open database\n");
		quit_mutt_query(1);
		exit(1);
	}
}

static void
quit_mutt_query(int status)
{
	close_database();
	close_config();

	exit(status);
}


static void
launch_mutt()
{
	int i;
	char email[MAX_EMAIL_LEN];
	char *cmd;
	char *tmp = options_get_str("mutt_command");

	if(curitem < 0)
		return;

	cmd = strconcat(tmp, " '", NULL );

	for(i=0; i < items; i++) {
		if( ! selected[i] && i != curitem )
			continue;
		get_first_email(email, i);
		tmp = mkstr("%s \"%s\"", cmd, database[i][NAME]);
		my_free(cmd);
		if( *database[i][EMAIL] ) {
			cmd = mkstr("%s <%s>", tmp, email);
			free(tmp);
			tmp = cmd;
		}
		cmd = strconcat(tmp, " ", NULL);
		free(tmp);
	}

	tmp = mkstr("%s%c", cmd, '\'');
	free(cmd);
	cmd = tmp;
#ifdef DEBUG
	fprintf(stderr, "cmd: %s\n", cmd);
#endif
	system(cmd);	

	free(cmd);
	refresh_screen();
}

static void
launch_lynx()
{
	char *cmd = NULL;

	if(curitem < 0)
		return;

	if( database[curitem][URL] )
		cmd = mkstr("%s '%s'",
				options_get_str("www_command"),
				safe_str(database[curitem][URL]));
	else
		return;

	if ( cmd )
		system(cmd);

	free(cmd);
	refresh_screen();
}

void *
abook_malloc(size_t size)
{
	void *ptr;

	if ( (ptr = malloc(size)) == NULL ) {
		if(top) /* determinate if init_abook has been called */
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
		if(top) /* determinate if init_abook has been called */
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
	
	if( ! strchr(mode, 'r') )
		return fopen(path, mode);
	
	if ( (stat(path, &s)) == -1 )
		return NULL;
	
	return S_ISREG(s.st_mode) ? fopen(path, mode) : NULL;
}


static void
win_changed(int i)
{
	if( can_resize )
		resize_abook();
	else
		should_resize = TRUE;	
}

#ifdef SIGWINCH
static void
resize_abook()
{
#ifdef TIOCGWINSZ
	struct winsize winsz;

	ioctl (0, TIOCGWINSZ, &winsz);
#ifdef DEBUG
	if(winsz.ws_col >= MIN_COLS && winsz.ws_row >= MIN_LINES) {
		fprintf(stderr, "Warning: COLS=%d, LINES=%d\n", winsz.ws_col, winsz.ws_row);
	}
#endif
		
	if(winsz.ws_col >= MIN_COLS && winsz.ws_row >= MIN_LINES) {
#ifdef HAVE_RESIZETERM
		resizeterm(winsz.ws_row, winsz.ws_col);
#else
		COLS = winsz.ws_col;
		LINES = winsz.ws_row;
#endif
	}

	should_resize = FALSE;
	close_list(); /* we need to recreate windows */
	init_list();
	free_windows();
	init_windows();
	refresh_screen();
	refresh();
#endif /* TIOCGWINSZ */
}
#endif /* SIGWINCH */


static void
convert(char *srcformat, char *srcfile, char *dstformat, char *dstfile)
{
	int ret=0;

	if( !srcformat || !srcfile || !dstformat || !dstfile ) {
		fprintf(stderr, "too few argumets to make conversion\n");
		fprintf(stderr, "try --help\n");
	}

	strlower(srcformat);
	strlower(dstformat);

	if( !strcmp(srcformat, dstformat) ) {
		printf(	"input and output formats are the same\n"
			"exiting...\n");
		exit(1);
	}

	set_filenames();
	init_options();

	switch( import(srcformat, srcfile) ) {
		case -1:
			printf("input format %s not supported\n", srcformat);
			ret = 1;
		case 1:
			printf("cannot read file %s\n", srcfile);
			ret = 1;
	}

	if(!ret)
		switch( export(dstformat, dstfile) ) {
			case -1:
				printf("output format %s not supported\n",
						dstformat);
				ret = 1;
				break;
			case 1:
				printf("cannot write file %s\n", dstfile);
				ret = 1;
				break;
		}

	close_database();
	close_config();
	exit(ret);
}


static void
open_datafile()
{
	char *filename;

	filename = ask_filename("File to open: ", 1);

	if( !filename ) {
		refresh_screen();
		return;
	}

	if( options_get_int("autosave") )
		save_database();
	else {
		statusline_addstr("Save current database (y/N)");
		switch( getch() ) {
			case 'y':
			case 'Y':
				save_database();
			default: break;
		}
	}

	close_database();

	load_database(filename);

	if( items == 0 ) {
		statusline_msg("Sorry, that specified file appears not to be a valid abook addressbook");
		load_database(datafile);
	} else {
		free(datafile);
		datafile = strdup(filename);
	}

	refresh_screen();
	free(filename);
}
