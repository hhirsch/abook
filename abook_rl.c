/*
 * $Id$
 *
 * by JH <jheinonen@users.sourceforge.net>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "abook.h"
#include "abook_rl.h"

#define KEYPAD_HACK 1 /* enable keypad hack */
#define CBREAK_HACK 1 /* enable cbreak hack */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#if defined(HAVE_READLINE_READLINE_H)
#       include <readline/readline.h>
#elif defined(HAVE_READLINE_H)
#       include <readline.h>
#endif

#if defined(HAVE_READLINE_HISTORY_H)
#       include <readline/history.h>
#elif defined(HAVE_HISTORY_H)
#       include <history.h>
#endif

#define RL_READLINE_NAME	"Abook"

static int rl_x, rl_y;
static WINDOW *rl_win;

static bool rl_cancelled;

static void
rl_refresh()
{
	/*refresh();*/
	wrefresh(rl_win);
}

static void
rline_update()
{	
	int real_point = rl_point + rl_x;
	
	if(real_point > (COLS - 1))
		mvwaddnstr(rl_win, rl_y, rl_x,
			rl_line_buffer + (1 + real_point - COLS),
			COLS - rl_x - 1);
	else
		mvwaddnstr(rl_win, rl_y, rl_x, rl_line_buffer, rl_end);

	wclrtoeol(rl_win);
	wmove(rl_win, rl_y, min(real_point, COLS - 1));

	rl_refresh();
}

static void
rline_compdisp(char **matches, int n, int max_len)
{
	/* dummy */
}

static int
rl_cancel(int dummy1, int dummy2)
{
	rl_cancelled = TRUE;

	rl_done = 1;

	return 0;
}

static void
abook_rl_init(bool use_completion)
{
	rl_readline_name = RL_READLINE_NAME;
	
	rl_already_prompted = 1;
	rl_catch_sigwinch = 0;
	
	rl_redisplay_function = rline_update;
	rl_completion_display_matches_hook = rline_compdisp;

	rl_unbind_function_in_map(rl_clear_screen, rl_get_keymap());
	rl_unbind_function_in_map(rl_reverse_search_history, rl_get_keymap());
	rl_unbind_function_in_map(rl_re_read_init_file, rl_get_keymap());
	
	if(use_completion) {
		rl_bind_key('\t', rl_menu_complete);
	} else {
		rl_unbind_function_in_map(rl_complete, rl_get_keymap());
		rl_unbind_function_in_map(rl_menu_complete, rl_get_keymap());
	}

	rl_bind_key('g' & 31, rl_cancel); /* C-g */

	clear_history();

	rl_cancelled = FALSE;
}	

char *
abook_readline(WINDOW *w, int y, int x, char *s, int limit, bool use_completion)
{
	char *ret;

	abook_rl_init(use_completion);

	wmove(rl_win = w, rl_y = y, rl_x = x);
	rl_refresh();

	if(s && *s)
		add_history(s);
	
#ifdef KEYPAD_HACK
	keypad(w, FALSE);
#endif
#ifdef CBREAK_HACK
	nocbreak();
#endif
	ret = readline(NULL);
#ifdef CBREAK_HACK
	cbreak();
#endif
#ifdef KEYPAD_HACK
	keypad(w, TRUE);
#endif

	if(rl_cancelled && ret) {
		free(ret);
		ret = NULL;
	}

	return ret;
}

