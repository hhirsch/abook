#ifndef _UI_H
#define _UI_H

#include "abook_curses.h"

enum {
	HELP_MAIN,
	HELP_EDITOR
};

int		is_ui_initialized();
void		ui_init_curses();
int		init_ui();
void		close_ui();
void		headerline(char *str);
void            refresh_screen();
int		statusline_msg(char *msg);
char		*ask_filename(char *prompt, int flags);
int		statusline_ask_boolean(char *msg, int def);
void            clear_statusline();
void		display_help(int help);
void		statusline_addstr(char *str);
char		*statusline_getnstr(char *str, int n, int use_filesel);
void		refresh_statusline();
void		get_commands();
void		ui_remove_items();
void		ui_clear_database();
void		ui_find(int next);
void		ui_print_number_of_items();
void		ui_read_database();
char		*get_surname(char *s);
void		ui_print_database();
void		ui_open_datafile();


#include "options.h" /* needed for options_get_int */

#define UI_HLINE_CHAR		options_get_int("use_ascii_only") ? \
					'-' : ACS_HLINE

#endif
