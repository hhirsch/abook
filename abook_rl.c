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

void
rline_compdisp(char **matches, int n, int max_len)
{
	/*
	 * dummy
	 */
}

static void
abook_rl_init(int use_completion)
{
	rl_readline_name = RL_READLINE_NAME;
	
	rl_already_prompted = 1;
	
	rl_redisplay_function = rline_update;
	rl_completion_display_matches_hook = rline_compdisp;
	
	rl_unbind_function_in_map(rl_clear_screen, rl_get_keymap());

	if(use_completion)
		rl_bind_key('\t', rl_menu_complete);
	else
		rl_unbind_function_in_map(rl_complete, rl_get_keymap());

	clear_history();
}	

char *
abook_readline(WINDOW *w, int y, int x, char *s, int limit, int use_completion)
{
	char *ret = NULL;

	rl_win = w;
	abook_rl_init(use_completion);

	wmove(rl_win, rl_y = y, rl_x = x);
	rl_refresh();

	if(s && *s)
		add_history(s);
	
	ret = readline(NULL);

	return ret;
}
