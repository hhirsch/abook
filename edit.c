
/*
 * $Id$
 *
 * by JH <jheinonen@users.sourceforge.net>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <string.h>
#include <stdlib.h>
#include "abook_curses.h"
#include "ui.h"
#include "abook.h"
#include "database.h"
#include "list.h"
#include "edit.h"
#include "misc.h"
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
editor_tab(int tab)
{
	int i;
	char *tab_names[] = {
		"/ CONTACT \\",
		"/ ADDRESS \\",
		"/  PHONE  \\",
		"/  OTHER  \\"
	};

	mvwhline(editw, TABLINE+1, 0, UI_HLINE_CHAR, EDITW_COLS);

	for(i=0; i < TABS; i++)
		mvwaddstr(editw, TABLINE, 16 * i + 3, tab_names[i]);

	mvwaddstr(editw, TABLINE+1, 16 * tab + 2, "/           \\");
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
		*tmp=0;
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
		*p=0;

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

/*
 * we have to introduce edit_undo here
 */
static void edit_undo(int item, int mode);

enum {
	BACKUP_ITEM,
	RESTORE_ITEM,
	CLEAR_UNDO
};


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

	if( (header = (char *)malloc(EDITW_COLS)) == NULL )
		return;

	get_first_email(email, item);

	if( *database[item][EMAIL] )
		snprintf(header, EDITW_COLS, "%s <%s>",
				database[item][NAME],
				email);
	else
		snprintf(header, EDITW_COLS, "%s", database[item][NAME]);

	mvwaddstr(editw, 0, (EDITW_COLS - strlen(header)) / 2,
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

		if(i==EMAIL) { /* special field */
			int k;
			char emails[MAX_EMAILS][MAX_EMAIL_LEN];
			split_emailstr(item, emails);
			getyx(editw, y, x);
			mvwaddstr(editw, y+1, FIELDS_START_X,
					"E-mail addresses:");
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
			getyx(editw, y, x); y++;
		} else
			y = FIELDS_START_Y;

		mvwprintw(editw, y, FIELDS_START_X, "%d - %s",
				j,
				abook_fields[i].name);
		mvwaddch(editw, y, TAB_COLON_POS, ':');
		mvwaddstr(editw, y, TAB_COLON_POS + 2,
				safe_str(database[item][i]));

		j++;
	}
}

static int
change_field(char *msg, char **field)
{
	int max_len = MAX_FIELD_LEN;
	char *old;

	if( !strncmp("E-mail", msg, 6) )
		max_len = MAX_EMAIL_LEN;

	old = *field;

	*field = ui_readline(msg, old, max_len - 1, 0);

	free(old);

	clear_statusline();
	refresh_statusline();

	return 0;
}

static void
change_name_field(char **field)
{
	char *tmp;

	tmp = strdup(*field);
	change_field("Name: ", field);

	if( *field == NULL || ! **field ) {
		my_free(*field);
		*field = strdup(tmp);
	}

	my_free(tmp);
}

static void
fix_email_str(char *str)
{
	for(; *str; str++ )
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
	field = strdup(emails[email_num]);

	if(change_field("E-mail: ", &field)) {
#ifdef DEBUG
		fprintf(stderr, "change_field = TRUE\n");
#endif
		return;
	}
	if(field) {
		strncpy(emails[email_num], field, MAX_EMAIL_LEN);
		fix_email_str(emails[email_num]);
	} else
		*emails[email_num] = 0;

	my_free(database[item][EMAIL]);

	for(i = 0; i < MAX_EMAILS; i++) {
		if( *emails[i] ) {
			strcat(tmp, emails[i]);
			strcat(tmp, ",");
		}
	}

	len = strlen(tmp);
	if(tmp[len -1] == ',')
		tmp[len-1] =0;

	database[item][EMAIL] = strdup(tmp);
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

	for(i=0, j=0; i< ITEM_FIELDS; i++) {
		if(abook_fields[i].tab == tab)
			j++;
		if(j==n)
			break;
	}

	if(j!=n)
		return 0;

	str = mkstr("%s: ", abook_fields[i].name);
	change_field(str, &database[item][i]);

	free(str);

	return 1;
}

static void
edit_undo(int item, int mode)
{
	int i;
	static list_item *backup = NULL;

	switch(mode) {
		case CLEAR_UNDO:
			if(backup) {
				free_list_item(backup[0]);
				my_free(backup);
			}
			break;
		case BACKUP_ITEM:
			if(backup) {
				free_list_item(backup[0]);
				my_free(backup);
			}
			backup = (list_item *)abook_malloc(sizeof(list_item));
			for(i = 0; i < ITEM_FIELDS; i++)
				backup[0][i] = safe_strdup(database[item][i]);
			break;
		case RESTORE_ITEM:
			if(backup) {
				free_list_item(database[item]);
				itemcpy(database[item], backup[0]);
				my_free(backup);
			}
			break;
	}
}

static int
edit_loop(int item)
{
	static int tab = 0; /* first tab */
	int c;

	werase(editw);
	headerline(EDITOR_HELPLINE);
	refresh_statusline();
	print_editor_header(item);
	editor_tab(tab);
	editor_print_data(tab, item);
	wmove(editw, EDITW_LINES - 1, EDITW_COLS - 1);

	refresh();
	wrefresh(editw);

	switch( (c = getch()) ) {
		case 'c': tab = TAB_CONTACT; break;
		case 'a': tab = TAB_ADDRESS; break;
		case 'p': tab = TAB_PHONE; break;
		case 'o': tab = TAB_OTHER; break;
		case KEY_LEFT: tab = tab == 0 ? MAX_TAB : tab - 1;
			       break;
		case KEY_RIGHT: tab = tab == MAX_TAB ? 0 : tab + 1;
				break;
		case '<':
		case 'k': if(is_valid_item(item-1)) item--; break;
		case '>':
		case 'j': if(is_valid_item(item+1)) item++; break;
		case 'r': roll_emails(item); break;
		case '?': display_help(HELP_EDITOR); break;
		case 'u': edit_undo(item, RESTORE_ITEM); break;
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

	while( (item = edit_loop(item)) >= 0 )
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

