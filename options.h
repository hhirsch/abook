#ifndef _OPTIONS_H
#define _OPTIONS_H

#if 0
typedef int bool;
#else
#	include <abook_curses.h> /* bool */
#endif

/*
 * bool options
 */


enum bool_opts {
	BOOL_AUTOSAVE,
	BOOL_SHOW_ALL_EMAILS,
	BOOL_MUTT_RETURN_ALL_EMAILS,
	BOOL_USE_ASCII_ONLY,
	BOOL_MAX
};

/*
 * int options
 */

enum int_opts {
	INT_EMAILPOS,
	INT_EXTRAPOS,
	INT_MAX
};

/*
 * string options
 */

enum str_opts {
	STR_EXTRA_COLUMN,
	STR_EXTRA_ALTERNATIVE,
	STR_MUTT_COMMAND,
	STR_PRINT_COMMAND,
	STR_WWW_COMMAND,
	STR_ADDRESS_STYLE,
	STR_MAX
};


int		opt_get_int(enum int_opts opt);
bool		opt_get_bool(enum bool_opts opt);
char *		opt_get_str(enum str_opts opt);
void		init_opts();
void		free_opts();
int		load_opts(char *filename);

#endif
