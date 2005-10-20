
/*
 * $Id$
 *
 * by JH <jheinonen@users.sourceforge.net>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "abook_curses.h"
#include "ui.h"
#include "abook.h"
#include "database.h"
#include "gettext.h"
#include "list.h"
#include "edit.h"
#include "misc.h"
#include "xmalloc.h"
#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

/*
 * some extern variables
 */

extern struct abook_field abook_fields[];

extern list_item *database;
extern int curitem;
extern int items;

WINDOW *editw;

static void
editor_tab(const int tab)
{
	int i, j;
	int x_pos = 2; /* current x pos */
	static char *tab_names[] = {
		N_("CONTACT"),
		N_("ADDRESS"),
		N_(" PHONE "),
		N_(" OTHER "),
		N_("CUSTOM ")
	};

	mvwhline(editw, TABLINE + 1, 0, UI_HLINE_CHAR, EDITW_COLS);

	for(i = 0; i < TABS; i++) {
		int width = strwidth(gettext(tab_names[i])) + 5;

		if(x_pos + width + 1 > EDITW_COLS) {
			statusline_msg(_("Tab name too wide for screen"));
			break;
		}

		mvwaddch(editw,  TABLINE + 1, x_pos,  UI_TEE_CHAR);
		mvwaddch(editw,  TABLINE + 1, x_pos + width - 2, UI_TEE_CHAR);

		mvwaddch(editw,  TABLINE, x_pos,  UI_ULCORNER_CHAR);
		mvwaddch(editw,  TABLINE, x_pos + 1,  UI_LBOXLINE_CHAR);
		mvwaddstr(editw, TABLINE, x_pos + 2,  gettext(tab_names[i]));
		mvwaddch(editw,  TABLINE, x_pos + width - 3, UI_RBOXLINE_CHAR);
		mvwaddch(editw,  TABLINE, x_pos + width - 2, UI_URCORNER_CHAR);

		if(i == tab) {
			mvwaddch(editw,  TABLINE + 1, x_pos, UI_LRCORNER_CHAR);
			for(j = 0; j < width - 3; j++)
				mvwaddstr(editw,
					TABLINE + 1, x_pos + j + 1, " ");
			mvwaddch(editw,  TABLINE + 1, x_pos + width - 2,
				UI_LLCORNER_CHAR);
		}
		x_pos += width;
	}
}

void
get_first_email(char *str, int item)
{
	char *tmp;

	if(database[item][EMAIL] == NULL) {
		*str = 0;
		return;
	}

	strncpy(str, database[item][EMAIL], MAX_EMAIL_LEN);
	if( (tmp = strchr(str, ',')) )
		*tmp = 0;
	else
		str[MAX_EMAIL_LEN-1] = 0;
}

static void
roll_emails(int item)
{
	char tmp[MAX_EMAILSTR_LEN];
	char *p;

	strcpy(tmp, database[item][EMAIL]);

	if( !(p = strchr(tmp, ',')) )
		return;
	else
		*p = 0;

	strcpy(database[item][EMAIL], p+1);
	strcat(database[item][EMAIL], ",");
	strcat(database[item][EMAIL], tmp);
}

static void
init_editor()
{
	clear();
	editw = newwin(EDITW_LINES, EDITW_COLS, EDITW_TOP, EDITW_X);

	refresh_statusline();
}

enum {
	BACKUP_ITEM,
	RESTORE_ITEM,
	CLEAR_UNDO
};

static int
edit_undo(int item, int mode)
{
	int i;
	static list_item *backup = NULL;
	static int backed_up_item = -1;

	switch(mode) {
		case CLEAR_UNDO:
			if(backup) {
				free_list_item(backup[0]);
				xfree(backup);
			}
			break;
		case BACKUP_ITEM:
			if(backup) {
				free_list_item(backup[0]);
				xfree(backup);
			}
			backup = xmalloc(sizeof(list_item));
			for(i = 0; i < ITEM_FIELDS; i++)
				if(database[item][i] == NULL)
					backup[0][i] = NULL;
				else
					backup[0][i] =
						xstrdup(database[item][i]);
			backed_up_item = item;
			break;
		case RESTORE_ITEM:
			if(backup) {
				free_list_item(database[backed_up_item]);
				itemcpy(database[backed_up_item], backup[0]);
				xfree(backup);
				return backed_up_item;
			}
			break;
		default:
			assert(0);
	}

	return item;
}


static void
close_editor()
{
	edit_undo(-1, CLEAR_UNDO);
	delwin(editw);
	refresh_screen();
}

static void
print_editor_header(int item)
{
	char *header;
	char email[MAX_EMAIL_LEN];

	if((header = xmalloc(EDITW_COLS)) == NULL)
		return;

	get_first_email(email, item);

	if(*database[item][EMAIL])
		snprintf(header, EDITW_COLS, "%s <%s>",
				database[item][NAME],
				email);
	else
		snprintf(header, EDITW_COLS, "%s", database[item][NAME]);

	mvwaddstr(editw, 0, (EDITW_COLS - strwidth(header)) / 2,
			header);

	free(header);
}

static void
editor_print_data(int tab, int item)
{
	int i, j;
	int y, x;

	for(i = 0, j = 1; i < ITEM_FIELDS; i++) {
		if(abook_fields[i].tab != tab)
			continue;

		if(i == EMAIL) { /* special field */
			int k;
			char emails[MAX_EMAILS][MAX_EMAIL_LEN];
			split_emailstr(item, emails);
			getyx(editw, y, x);
			mvwaddstr(editw, y+1, FIELDS_START_X,
					_("E-mail addresses:"));
			for(k = 0; k < MAX_EMAILS; k++) {
				getyx(editw, y, x);
				mvwprintw(editw, y+1, FIELDS_START_X,
				"%c -", '2' + k);
				mvwprintw(editw, y +1, TAB_COLON_POS,
						": %s", emails[k]);
			}
			continue;
		}

		if(j > 1) {
			getyx(editw, y, x);
			y++;
		} else
			y = FIELDS_START_Y;

		mvwprintw(editw, y, FIELDS_START_X, "%d - %s",
				j,
				gettext(abook_fields[i].name));
		mvwaddch(editw, y, TAB_COLON_POS, ':');
		mvwaddstr(editw, y, TAB_COLON_POS + 2,
				safe_str(database[item][i]));

		j++;
	}
}

/*
 * function: change_field
 *
 * parameters:
 *  (char *msg)
 *   message to display as a prompt
 *  (char **field)
 *   a pointer to a pointer which will point a new string. if the latter
 *   pointer != NULL it will be freed (if user doesn't cancel)
 *
 * returns (int)
 *  a nonzero value if user has cancelled and zero if user has typed a
 *  valid string
 */

static int
change_field(char *msg, char **field)
{
	int max_len = MAX_FIELD_LEN;
	char *old;
	int ret = 0;

	if(!strncmp("E-mail", msg, 6))
		max_len = MAX_EMAIL_LEN;

	old = *field;

	*field = ui_readline(msg, old, max_len - 1, 0);

	if(*field) {
		xfree(old);
		if(!**field)
			xfree(*field);
	} else {
		*field = old;
		ret = 1;
	}

	clear_statusline();
	refresh_statusline();

	return ret;
}

static void
change_name_field(char **field)
{
	char *tmp;

	tmp = xstrdup(*field);
	change_field("Name: ", field);

	if(*field == NULL || ! **field) {
		xfree(*field);
		*field = xstrdup(tmp);
	}

	xfree(tmp);
}

static void
fix_email_str(char *str)
{
	for(; *str; str++)
		*str = *str == ',' ? '_' : *str;
}

static void
edit_emails(char c, int item)
{
	char *field;
	char emails[MAX_EMAILS][MAX_EMAIL_LEN];
	char tmp[MAX_EMAILSTR_LEN] = "";
	int i, len;
	int email_num = c - '2';

	split_emailstr(item, emails);
	field = xstrdup(emails[email_num]);

	if(change_field("E-mail: ", &field))
		return; /* user cancelled ( C-g ) */

	if(field) {
		strncpy(emails[email_num], field, MAX_EMAIL_LEN);
		fix_email_str(emails[email_num]);
	} else
		*emails[email_num] = 0;

	xfree(database[item][EMAIL]);

	for(i = 0; i < MAX_EMAILS; i++) {
		if( *emails[i] ) {
			strcat(tmp, emails[i]);
			strcat(tmp, ",");
		}
	}

	len = strlen(tmp);
	if(tmp[len -1] == ',')
		tmp[len-1] =0;

	database[item][EMAIL] = xstrdup(tmp);
}

static int
edit_field(int tab, char c, int item)
{
	int i, j;
	int n = c - '1' + 1;
	char *str;

	if(n < 1 || n > MAX_TAB_FIELDS)
		return 0;

	edit_undo(item, BACKUP_ITEM);

	if(tab == TAB_CONTACT) {
		switch(c) {
			case '1': change_name_field(&database[item][NAME]);
				  break;
			case '2':
			case '3':
			case '4':
			case '5': edit_emails(c, item); break;
			default: return 0;
		}
		return 1;
	}

	for(i = 0, j = 0; i< ITEM_FIELDS; i++) {
		if(abook_fields[i].tab == tab)
			j++;
		if(j==n)
			break;
	}

	if(j != n)
		return 0;

	str = strdup_printf("%s: ", gettext(abook_fields[i].name));
	change_field(str, &database[item][i]);

	free(str);

	return 1;
}

static int
edit_loop(int item)
{
	static int tab = 0; /* first tab */
	int c;

	werase(editw);
	headerline(gettext(EDITOR_HELPLINE));
	refresh_statusline();
	print_editor_header(item);
	editor_tab(tab);
	editor_print_data(tab, item);
	wmove(editw, EDITW_LINES - 1, EDITW_COLS - 1);

	refresh();
	wrefresh(editw);

	switch((c = getch())) {
		case 'c': tab = TAB_CONTACT; break;
		case 'a': tab = TAB_ADDRESS; break;
		case 'p': tab = TAB_PHONE; break;
		case 'o': tab = TAB_OTHER; break;
		case 'C': tab = TAB_CUSTOM; break;
		case 'h':
		case KEY_LEFT: tab = tab == 0 ? MAX_TAB : tab - 1;
			       break;
		case 'l':
		case KEY_RIGHT: tab = tab == MAX_TAB ? 0 : tab + 1;
				break;
		case KEY_UP:
		case '<':
		case 'k': if(is_valid_item(item-1)) item--; break;
		case KEY_DOWN:
		case '>':
		case 'j': if(is_valid_item(item + 1)) item++; break;
		case 'r': roll_emails(item); break;
		case '?': display_help(HELP_EDITOR); break;
		case 'u': item = edit_undo(item, RESTORE_ITEM); break;
		case 'm': launch_mutt(item); clearok(stdscr, 1); break;
		case 'v': launch_wwwbrowser(item); clearok(stdscr, 1); break;
		case 12 : clearok(stdscr, 1); break; /* ^L (refresh screen) */
		default:  return edit_field(tab, c, item) ? item : -1;
	}

	return item;
}

void
edit_item(int item)
{
	if( item < 0 ) {
		if( curitem < 0 )
			return;
		else
			item = curitem;
	}

	init_editor();

	while((item = edit_loop(item)) >= 0)
		curitem = item; /* hmm, this is not very clean way to go */

	close_editor();
}

void
add_item()
{
	char *field = NULL;
	list_item item;

	change_field("Name: ", &field);

	if( field == NULL )
		return;

	memset(item, 0, sizeof(item));

	item[NAME] = field;

	add_item2database(item);

	curitem = LAST_ITEM;

	edit_item(LAST_ITEM);
}

