
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
#include <signal.h>
#include <ctype.h>
#include "abook.h"
#include "ui.h"
#include "edit.h"
#include "database.h"
#include "list.h"
#include "misc.h"
#include "options.h"
#include "filter.h"
#include "xmalloc.h"
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#ifdef HAVE_TERMIOS_H
#	include <termios.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#	include <sys/ioctl.h>
#endif

#include "abook_rl.h"

/*
 * external variables
 */

extern int items, curitem;
extern char *datafile;

extern bool alternative_datafile;

/*
 * internal variables
 */

static bool ui_initialized = FALSE;

static bool should_resize = FALSE;
static bool can_resize = FALSE;

static WINDOW *top = NULL, *bottom = NULL;


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


#ifdef SIGWINCH
static void
resize_abook()
{
#ifdef TIOCGWINSZ
	struct winsize winsz;

	ioctl(0, TIOCGWINSZ, &winsz);
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


static void
win_changed(int i)
{
	if(can_resize)
		resize_abook();
	else
		should_resize = TRUE;
}
#endif /* SIGWINCH */


int
is_ui_initialized()
{
	return ui_initialized;
}

void
ui_init_curses()
{
	if(!is_ui_initialized())
		initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
}

int
init_ui()
{
	ui_init_curses();
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
		return 1;
	}

	init_list();
	init_windows();

	ui_initialized = TRUE;

#ifdef SIGWINCH
	signal(SIGWINCH, win_changed);
#endif

	return 0;
}

void
close_ui()
{
	close_list();
	free_windows();
	clear();
	refresh();
	endwin();

	ui_initialized = FALSE;
}


void
headerline(char *str)
{
	werase(top);

	mvwhline(top, 1, 0, UI_HLINE_CHAR, COLS);

	mvwprintw(top, 0, 0, "%s | %s", PACKAGE " " VERSION, str);

	refresh();
	wrefresh(top);
}


void
refresh_screen()
{
#ifdef SIGWINCH
	if(should_resize) {
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


int
statusline_msg(char *msg)
{
	int c;

	clear_statusline();
	statusline_addstr(msg);
	c = getch();
#ifdef DEBUG
	fprintf(stderr, "statusline_msg(\"%s\")\n", msg);
#endif
	clear_statusline();

	return c;
}

void
statusline_addstr(char *str)
{
	mvwaddstr(bottom, 1, 0, str);
	refresh();
	wrefresh(bottom);
}

char *
ui_readline(char *prompt, char *s, int limit, bool use_completion)
{
	int y, x;
	char *ret;

	mvwaddstr(bottom, 1, 0, prompt);

	getyx(bottom, y, x);

	ret = abook_readline(bottom, y, x, s, limit, use_completion);

	if(ret)
		strtrim(ret);

	return ret;
}

int
statusline_ask_boolean(char *msg, int def)
{
	int ret;
	char *msg2 = strconcat(msg,  def ? " (Y/n)?" : " (y/N)?", NULL);

	statusline_addstr(msg2);

	free(msg2);

	switch(tolower(getch())) {
		case 'n':
		case 'N':
			ret = FALSE;
			break;
		case 'y':
		case 'Y':
			ret = TRUE;
			break;
		default:
			ret = def;
			break;
	}

	clear_statusline();

	return ret;
}

void
refresh_statusline()
{
	werase(bottom);

	mvwhline(bottom, 0, 0, UI_HLINE_CHAR, COLS);

	refresh();
	wrefresh(bottom);
}

char *
ask_filename(char *prompt)
{
	char *buf = NULL;

	clear_statusline();

	buf = ui_readline(prompt, NULL, -1, 1);

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

/*
 * help
 */

#include "help.h"

void
display_help(int help)
{
	int i;
	char **tbl;
	WINDOW *helpw;

	switch(help) {
		case HELP_MAIN:
			tbl = mainhelp;
			break;
		case HELP_EDITOR:
			tbl = editorhelp;
			break;
		default:return;
	}

	helpw = newwin(LINES - 5, COLS - 6, 2, 3);
	erase();
	headerline("help");

	for(i = 0; tbl[i] != NULL; i++) {
		waddstr(helpw, tbl[i]);
		if( (!((i + 1) % (LINES - 8))) ||
			(tbl[i + 1] == NULL) ) {
			refresh();
			wrefresh(helpw);
			refresh_statusline();
			if(statusline_msg("Press any key to continue...")
					== 'q')
				break;
			wclear(helpw);
		}
	}

	clear_statusline();
	delwin(helpw);
}

/*
 * end of help
 */

extern char *selected;
extern int curitem;

void
get_commands()
{
	int ch;

	for(;;) {
		can_resize = TRUE; /* it's safe to resize now */
		if(!opt_get_bool(BOOL_SHOW_CURSOR))
			hide_cursor();
		if(should_resize)
			refresh_screen();
		ch = getch();
		if(!opt_get_bool(BOOL_SHOW_CURSOR))
			show_cursor();
		can_resize = FALSE; /* it's not safe to resize anymore */
		switch(ch) {
			case 'q': return;
			case 'Q': quit_abook(QUIT_DONTSAVE);	break;
			case 'P': print_stderr(selected_items() ?
						  -1 : list_current_item());
				  return;
			case '?':
				  display_help(HELP_MAIN);
				  refresh_screen();
				  break;
			case 'a': add_item();		break;
			case '\r': edit_item(-1);	break;
			case KEY_DC:
			case 'd':
			case 'r': ui_remove_items();	break;
			case 'D': duplicate_item();	break;
			case 12: refresh_screen();	break;

			case 'k':
			case KEY_UP: scroll_up();	break;
			case 'j':
			case KEY_DOWN: scroll_down();	break;
			case 'K':
			case KEY_PPAGE: page_up();	break;
			case 'J':
			case KEY_NPAGE: page_down();	break;

			case 'g':
			case KEY_HOME: goto_home();	break;
			case 'G':
			case KEY_END: goto_end();	break;

			case 'w': save_database();
				  break;
			case 'l': ui_read_database();	break;
			case 'i': import_database();	break;
			case 'e': export_database();	break;
			case 'C': ui_clear_database();	break;

			case 'o': ui_open_datafile();	break;

			case 's': sort_by_field(NAME);	break;
			case 'S': sort_surname();	break;
			case 'F': sort_by_field(-1);	break;

			case '/': ui_find(0);		break;
			case '\\': ui_find(1);		break;

			case ' ': if(curitem >= 0) {
				   selected[curitem] = !selected[curitem];
				   ui_print_number_of_items();
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

			case 'm': launch_mutt(selected_items() ?
						  -1 : list_current_item());
				  refresh_screen();
				  break;

			case 'p': ui_print_database(); break;

			case 'u': launch_wwwbrowser(list_current_item());
				  refresh_screen();
				  break;
		}
	}
}

void
ui_remove_items()
{
	if(list_is_empty())
		return;

	if(statusline_ask_boolean("Remove selected item(s)", TRUE))
		remove_selected_items();

	clear_statusline();
	refresh_list();
}

void
ui_clear_database()
{
	if(statusline_ask_boolean("Clear WHOLE database", FALSE)) {
		close_database();
		refresh_list();
	}
}

void
ui_find(int next)
{
	int item = -1;
	static char findstr[MAX_FIELD_LEN];
	int search_fields[] = {NAME, EMAIL, NICK, -1};

	clear_statusline();

	if(next) {
		if(!*findstr)
			return;
	} else {
		char *s;
		s = ui_readline("/", findstr, MAX_FIELD_LEN - 1, 0);
		strncpy(findstr, s, MAX_FIELD_LEN);
		refresh_screen();
	}

	if( (item = find_item(findstr, curitem + !!next, search_fields)) < 0 &&
			(item = find_item(findstr, 0, search_fields)) >= 0)
		statusline_addstr("Search hit bottom, continuing at top");

	if(item >= 0) {
		curitem = item;
		refresh_list();
	}
}


void
ui_print_number_of_items()
{
	char *str = mkstr("     " "|%3d/%3d", selected_items(), items);

	mvaddstr(0, COLS-strlen(str), str);

	free(str);
}

void
ui_read_database()
{
	if(items > 0)
		if(!statusline_ask_boolean("Your current data will be lost - "
				"Press 'y' to continue", FALSE))
			return;

	load_database(datafile);
	refresh_list();
}


void
ui_print_database()
{
	FILE *handle;
	char *command = opt_get_str(STR_PRINT_COMMAND);
	int mode;

	if(list_is_empty())
		return;

	statusline_addstr("Print All/Selected/Cancel (a/s/C)?");

	switch(tolower(getch())) {
		case 'a':
			mode = ENUM_ALL;
			break;
		case 's':
			if( !selected_items() ) {
				statusline_msg("No selected items");
				return;
			}
			mode = ENUM_SELECTED;
			break;
		default:
			clear_statusline();
			return;
	}

	clear_statusline();

	if( ! *command || (handle = popen(command, "w")) == NULL)
		return;

	fexport("text", handle, mode);

	pclose(handle);
}


void
ui_open_datafile()
{
	char *filename;

	filename = ask_filename("File to open: ");

	if(!filename || ! *filename) {
		free(filename);
		refresh_screen();
		return;
	}

	if(opt_get_bool(BOOL_AUTOSAVE))
		save_database();
	else if(statusline_ask_boolean("Save current database", FALSE))
		save_database();

	close_database();

	load_database(filename);

	if(items == 0) {
		statusline_msg("Sorry, that specified file appears not to be a valid abook addressbook");
		load_database(datafile);
	} else {
		free(datafile);
		datafile = xstrdup(filename);
	}

	refresh_screen();
	free(filename);

	alternative_datafile = TRUE;
}
