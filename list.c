
/*
 * $Id$
 *
 * by JH <jheinonen@users.sourceforge.net>
 *
 * Copyright (C) Jaakko Heinonen
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "abook.h"
#include "ui.h"
#include "database.h"
#include "edit.h"
#include "list.h"
#include "misc.h"
#include "options.h"

#define MIN_EXTRA_COLUMN	ADDRESS /* 2 */
#define MAX_EXTRA_COLUMN	LAST_FIELD

int curitem = -1;
int first_list_item = -1;
char *selected = NULL;

int extra_column = -1;
int extra_alternative = -1;

extern int items;
extern list_item *database;
extern struct abook_field abook_fields[];

WINDOW *list = NULL;

static int
init_extra_field(char *option_name)
{
	int i, ret = -1;
	char *option_str;

	assert(option_name != NULL);
	
	option_str = options_get_str(option_name);

	if(option_str && *option_str) {
		for(i = 0; i < ITEM_FIELDS; i++) {
			if(!strcasecmp(option_str, abook_fields[i].key)) {
				ret = i;
				break;
			}
		}
		if(ret < MIN_EXTRA_COLUMN || ret > MAX_EXTRA_COLUMN) {
			ret = -1;
		}
	}

	return ret;
}

void
init_list()
{
	list = newwin(LIST_LINES, LIST_COLS, LIST_TOP, 0);
	scrollok(list, TRUE);

	/*
	 * init extra_column and extra alternative
	 */

	extra_column = init_extra_field("extra_column");
	extra_alternative = init_extra_field("extra_alternative");
}

void
close_list()
{
	delwin(list);
	list = NULL;
}

void
refresh_list()
{
	int i, line;
	
	werase(list);

	ui_print_number_of_items();
	
	if( items < 1 ) {
		refresh();
		wrefresh(list);
		return;
	}
	
	if(curitem < 0)
		curitem = 0;

	if(first_list_item < 0)
		first_list_item = 0;

	if(curitem < first_list_item)
		first_list_item = curitem;
	else if(curitem > LAST_LIST_ITEM)
		first_list_item = max(curitem - LIST_LINES + 1, 0);

        for( line = 0, i = first_list_item ; i <= LAST_LIST_ITEM && i < items;
			line++, i++ ) {

		print_list_line(i, line, i == curitem);
        }

        wrefresh(list);
}

void
print_list_line(int i, int line, int highlight)
{
	int extra = extra_column;
	char tmp[MAX_EMAILSTR_LEN];
	int real_emaillen = (extra_column > 0 || extra_alternative > 0) ?
		EMAILLEN : COLS - EMAILPOS;

	scrollok(list, FALSE);
	if(highlight)
		highlight_line(list, line);

	if( selected[i] )
		mvwaddch(list, line, 0, '*' );
	
	mvwaddnstr(list, line, NAMEPOS, database[i][NAME], NAMELEN);
	if( options_get_int( "show_all_emails"  ) )
		mvwaddnstr(list, line, EMAILPOS, database[i][EMAIL],
				real_emaillen);
	else {
		get_first_email(tmp, i);
		mvwaddnstr(list, line, EMAILPOS, tmp, real_emaillen);
	}

	if(extra < 0 || !database[i][extra])
		extra = extra_alternative;
	if(extra >= 0)
		mvwaddnstr(list, line, EXTRAPOS,
				safe_str(database[i][extra]),
				EXTRALEN);

	scrollok(list, TRUE);
	if(highlight_line)
		wstandend(list);
}
	

void
list_headerline()
{
	attrset(A_BOLD);
	mvaddstr(2, NAMEPOS, abook_fields[NAME].name);
	mvaddstr(2, EMAILPOS, abook_fields[EMAIL].name);
	if(extra_column > 0)
		mvaddnstr(2, EXTRAPOS, abook_fields[extra_column].name,
				COLS-EXTRAPOS);
	attrset(A_NORMAL);
}

void
scroll_up()
{
	if( curitem < 1 )
		return;

	curitem--;

	refresh_list();
}

void
scroll_down()
{
	if( curitem > items - 2 )
		return;

	curitem++;

	refresh_list();
}


void
page_up()
{
	if( curitem < 1 )
		return;
	
	curitem = curitem == first_list_item ?
		((curitem -= LIST_LINES) < 0 ? 0 : curitem) : first_list_item;
	
	refresh_list();
}

void
page_down()
{
	if( curitem > items - 2 )
		return;

	curitem = curitem == LAST_LIST_ITEM ?
		((curitem += LIST_LINES) > LAST_ITEM ? LAST_ITEM : curitem) :
		min(LAST_LIST_ITEM, LAST_ITEM);

	refresh_list();
}


void
select_none()
{
        memset( selected, 0, items );
}

void
select_all()
{
        memset( selected, 1, items );
}

void
move_curitem(int direction)
{
        list_item tmp;

        if( curitem < 0 || curitem > LAST_ITEM )
                return;

        itemcpy(tmp, database[curitem]);

        switch(direction) {
                case MOVE_ITEM_UP:
                        if( curitem < 1 )
                                return;
                        itemcpy(database[curitem], database[curitem - 1]);
                        itemcpy(database[curitem-1], tmp);
                        scroll_up();
                        break;

                case MOVE_ITEM_DOWN:
                        if( curitem >= LAST_ITEM )
                                return;
                        itemcpy(database[curitem], database[curitem + 1]);
                        itemcpy(database[curitem+1], tmp);
                        scroll_down();
                        break;
        }
}

void
goto_home()
{
	if(items > 0)
		curitem = 0;
	
	refresh_list();
}

void
goto_end()
{
	if(items > 0)
		curitem = LAST_ITEM;

	refresh_list();
}


void
highlight_line(WINDOW *win, int line)
{
	wstandout(win);
	
#ifdef mvwchgat
	mvwchgat(win, line, 0, -1,  A_STANDOUT, 0, NULL);
#else
	/*
	 * buggy function: FIXME
	 */
	scrollok(win, FALSE);
	{
		int i;
		wmove(win, line, 0);
		for(i = 0; i < COLS; i++)
			waddch(win, ' ');
	/*wattrset(win, 0);*/
	}
	scrollok(win, TRUE);
#endif
}

int
selected_items()
{
	int i, n = 0;

	for(i = 0; i < items; i++)
		if(selected[i])
			n++;

	return n;
}

void
invert_selection()
{
	int i;

	if( items < 1 )
		return;

	for(i = 0; i < items; i++)
		selected[i] = !selected[i];
}

int
list_current_item()
{
	return curitem;
}

int
list_is_empty()
{
	return items < 1;
}

	
